"""Contract and backward-compatibility checks without Windows / hardware."""
import json
import io
import asyncio
import datetime
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import Mock, patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import companion_core
import discovery
import media_provider


class CompanionCoreTests(unittest.TestCase):
    def test_worker_drops_late_callbacks_after_stop(self):
        callback=Mock(side_effect=RuntimeError("Signal source has been deleted"))
        worker=companion_core.CompanionWorker(companion_core.Settings(),"test",callback)
        worker.stop()
        worker._emit({"type":"telemetry","data":{}})
        callback.assert_not_called()
    def test_media_artwork_key_stable_and_absent_when_offline(self):
        media = {"available": True, "title": "Test", "artist": "Cat", "album": "Desk"}
        key = companion_core.media_artwork_id(media)
        self.assertEqual(len(key), 16)
        self.assertEqual(key, companion_core.media_artwork_id(dict(media)))
        self.assertNotEqual(key, companion_core.media_artwork_id(dict(media, album="Other")))
        self.assertEqual(companion_core.media_artwork_id(dict(media, available=False)), "")
        self.assertEqual(companion_core.wire_media(media)["artwork_id"], key)

    def test_artwork_upload_is_bounded_and_key_protected(self):
        pixels = b"\x00\xf8" * 4096  # solid red, standard RGB565 little-endian
        with patch.object(companion_core._http, "open", return_value=io.BytesIO(b'{"ok":true}')) as post:
            result = companion_core.post_artwork("192.168.0.107", "private-test", "a" * 16, pixels)
        self.assertTrue(result["ok"])
        request = post.call_args.args[0]
        self.assertTrue(request.full_url.endswith("/api/v1/artwork"))
        self.assertIn(pixels, request.data)
        headers = {key.lower(): value for key, value in request.header_items()}
        self.assertEqual(headers["x-meo-key"], "private-test")
        self.assertEqual(headers["x-meo-art-id"], "a" * 16)
        self.assertIn("multipart/form-data", headers["content-type"])
        with self.assertRaises(ValueError):
            companion_core.post_artwork("192.168.0.107", "key", "a" * 16, b"bad")

    def test_media_utf8_bytes_bounded(self):
        data = companion_core.wire_media({"available": True, "playing": True,
            "title": "Mèo" * 100, "artist": "Á" * 200,
            "position_ms": 2500, "duration_ms": 80000})
        self.assertLessEqual(len(data["title"].encode()), 64)
        self.assertLessEqual(len(data["artist"].encode()), 48)
        self.assertEqual(data["schema"], 1)
        self.assertTrue(data["available"])
        self.assertTrue(data["title"].endswith("Mèo") or len(data["title"]) > 0)
        self.assertLessEqual(len(json.dumps(data, ensure_ascii=False).encode()), 512)

    def test_missing_media_never_fabricates_song(self):
        no = media_provider.unavailable()
        self.assertFalse(no["available"])
        self.assertEqual(no["title"], "")
        self.assertFalse(no["playing"])

    def test_media_playing_enum_4_maps_correctly(self):
        from types import SimpleNamespace
        class Session:
            async def try_get_media_properties_async(self):
                return SimpleNamespace(title="Test", artist="Artist", album_title="Demo")
            def get_playback_info(self):
                return SimpleNamespace(playback_status=4)
            def get_timeline_properties(self):
                return SimpleNamespace(start_time=datetime.timedelta(),
                    position=datetime.timedelta(seconds=15), end_time=datetime.timedelta(seconds=120))
        async def fake_session():
            return Session()
        with patch.object(media_provider, "_get_session", side_effect=fake_session):
            status = asyncio.run(media_provider._snapshot())
        self.assertTrue(status["playing"])
        self.assertEqual(status["position_ms"], 15000)
        self.assertEqual(status["duration_ms"], 120000)

    def test_settings_no_pairing_secret_written(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "settings.json"
            companion_core.write_settings(companion_core.Settings(host="192.168.0.107"), path)
            self.assertNotIn("key", path.read_text())
            self.assertEqual(companion_core.read_settings(path).host, "192.168.0.107")

    def test_bad_settings_ignored(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "settings.json"
            path.write_text('{"interval":-1,"host":"broken"}')
            self.assertEqual(companion_core.read_settings(path), companion_core.Settings())

    def test_worker_ack_deduplicates(self):
        agent = companion_core.CompanionWorker(companion_core.Settings(host="192.168.0.107"), "test")
        calls = []
        def command(host, endpoint, key, payload=None, timeout=1.3):
            calls.append(endpoint)
            if endpoint.endswith("/commands"):
                return {"command": {"seq": 42, "action": "play_pause"}}
            return {"ok": True}
        with patch.object(companion_core, "json_request", side_effect=command), \
             patch.object(media_provider, "control", return_value=True) as ctl:
            agent._process_commands("192.168.0.107")
            agent._process_commands("192.168.0.107")
            ctl.assert_called_once_with("play_pause")
        self.assertEqual(sum(s.endswith("/ack") for s in calls), 1)

    def test_paused_media_is_refreshed_and_reboot_resets_volatile_session(self):
        agent = companion_core.CompanionWorker(companion_core.Settings(host="192.168.0.107"), "test")
        agent._last_seq = 42
        agent._last_artwork_id = "old-cover"
        agent.reset_device_session()
        count = [0]

        def collect(*_):
            count[0] += 1
            if count[0] == 3:
                agent._stop.set()
            return {"cpu_percent": 5, "ram_percent": 50}

        song = {"available": True, "playing": False, "title": "Paused song"}
        with patch.object(companion_core, "collect_real", side_effect=collect), \
             patch.object(companion_core, "send_json", return_value={"ok": True}), \
             patch.object(companion_core, "NvidiaGpu"), \
             patch.object(companion_core.psutil, "cpu_percent"), \
             patch.object(companion_core.time, "monotonic", side_effect=[100, 100, 102, 102, 104]), \
             patch.object(agent._stop, "wait"), \
             patch.object(agent, "_process_commands"), \
             patch.object(agent, "_poll_focus"), patch.object(agent, "_poll_motion"), \
             patch.object(media_provider, "snapshot", return_value=song), \
             patch.object(media_provider, "artwork_rgb565", return_value=None), \
             patch.object(companion_core, "json_request", return_value={"ok": True}) as send:
            agent._loop()
        self.assertEqual(agent._last_seq, 0)
        self.assertNotEqual(agent._last_artwork_id, "old-cover")
        self.assertEqual(sum(call.args[1] == "/api/v1/media" for call in send.call_args_list), 2)

    def test_motion_status_poll_is_optional_and_non_mutating(self):
        updates = []
        agent = companion_core.CompanionWorker(
            companion_core.Settings(host="192.168.0.107"), "test", updates.append)
        with patch.object(companion_core, "json_request", return_value={"version": "1.6.0"}) as read:
            agent._poll_motion("192.168.0.107")
        read.assert_called_once_with("192.168.0.107", "/api/v1/status", timeout=0.9)
        self.assertEqual(updates, [])
        state = {"motion_state": "music", "motion_style": 2,
                 "motion_budget_limited": False, "cat_motion_frames": 3}
        with patch.object(companion_core, "json_request", return_value=state):
            agent._poll_motion("192.168.0.107")
        self.assertEqual(updates[0]["type"], "motion_status")
        self.assertEqual(updates[0]["data"]["motion_style"], 2)


class DiscoveryTests(unittest.TestCase):
    def test_normalize_host(self):
        self.assertEqual(discovery.normalize_host(" http://192.168.0.107/ "), "192.168.0.107")
        self.assertEqual(discovery.normalize_host("MEODESK.local"), "meodesk.local")

    def test_disallow_bad_hosts_and_https(self):
        for value in ("", "http://foo:8080", "https://host", "localhost/a", "abc@foo", "8.8.8.8"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                discovery.normalize_host(value)

    def test_only_recognized_device_passes(self):
        import io
        class Response(io.BytesIO):
            status = 200
            headers = {"Content-Length": "30"}
        response = Response(b'{"device":"MEO DESK","version":"0.8.0"}')
        with patch.object(discovery._opener, "open", return_value=response):
            info = discovery.probe("192.168.0.107")
        self.assertEqual(info["version"], "0.8.0")

    def test_discover_prioritizes_old_compatible_device(self):
        with patch.object(discovery, "probe", side_effect=lambda host, timeout=0.7:
                          {"host": host, "version": "0.7.1", "name": "MEO DESK"}
                          if host == "192.168.0.107" else None):
            found = discovery.discover("192.168.0.107")
            self.assertEqual(len(found), 1)
            self.assertEqual(found[0]["version"], "0.7.1")


if __name__ == "__main__":
    unittest.main()
