"""Focused checks for music isolation, retry timing and one running agent."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import Mock, patch
import uuid

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import companion_core
from connection_state import RetryBackoff
from media_monitor import MediaMonitor
import media_provider


def session(name, status=4):
    item = Mock(source_app_user_model_id=name)
    item.get_playback_info.return_value = SimpleNamespace(playback_status=status)
    return item


class DesktopServiceTests(unittest.TestCase):
    def test_retry_is_bounded_and_recovers_after_success(self):
        retry = RetryBackoff(initial=2, maximum=8)
        self.assertTrue(retry.ready(0))
        self.assertEqual(retry.failed(0), 2)
        self.assertFalse(retry.ready(1))
        self.assertEqual(retry.failed(2), 4)
        self.assertEqual(retry.failed(6), 8)
        self.assertEqual(retry.failed(14), 8)
        retry.reset()
        self.assertTrue(retry.ready(14))
        self.assertEqual(retry.failed(14), 2)

    def test_session_recovers_spotify_opened_before_app(self):
        spotify = session("Spotify.exe")
        stale = session("browser", 5)
        manager = Mock()
        manager.get_current_session.return_value = None
        manager.get_sessions.return_value = [stale, spotify]
        self.assertIs(media_provider._select_session(manager), spotify)
        manager.get_current_session.return_value = stale
        self.assertIs(media_provider._select_session(manager), spotify)
        browser = session("browser")
        manager.get_current_session.return_value = browser
        self.assertIs(media_provider._select_session(manager), browser)
        browser.get_playback_info.side_effect = OSError("session closed")
        self.assertIs(media_provider._select_session(manager), spotify)

    def test_missing_bindings_and_provider_failure_have_distinct_states(self):
        async def missing():
            raise ImportError("optional provider")
        async def broken():
            raise OSError("provider failed")
        for reason, callback in (("missing_provider", missing), ("provider_error", broken)):
            with patch.object(media_provider, "_snapshot", side_effect=callback):
                result = media_provider.snapshot()
                self.assertFalse(result["available"])
                self.assertEqual(result["reason"], reason)
                self.assertEqual(result["title"], "")

    def test_local_music_heartbeat_cover_cache_and_stop(self):
        updates = []
        monitor = MediaMonitor(updates.append)
        song = {"available": True, "playing": False, "title": "Nhạc tiếng Việt"}
        pixels = b"\x00\xf8" * 4096
        with patch.object(media_provider, "snapshot", return_value=song), \
             patch.object(media_provider, "artwork_rgb565", return_value=pixels) as art:
            monitor._sample()
            monitor._sample()
            self.assertEqual(sum(e["type"] == "media" for e in updates), 2)
            art.assert_called_once_with(song)
            cached, cover = monitor.snapshot()
            self.assertEqual(cover, pixels)
            cached["title"] = "changed externally"
            self.assertEqual(monitor.snapshot()[0]["title"], song["title"])
            updates.clear()
            monitor.stop()
            monitor._sample()
            self.assertEqual(updates, [])
        monitor = MediaMonitor(updates.append)
        with patch.object(media_provider, "snapshot", return_value=song), \
             patch.object(media_provider, "artwork_rgb565", return_value=pixels):
            monitor._sample()
        with patch.object(media_provider, "snapshot", return_value=media_provider.unavailable()):
            monitor._sample()
        self.assertIsNone(monitor.snapshot()[1])
        self.assertFalse(monitor.snapshot()[0]["available"])

    def test_network_outage_does_not_stop_local_music_or_spin_retries(self):
        updates, attempts, clock = [], [], [0.0]
        monitor = MediaMonitor(updates.append)
        worker = companion_core.CompanionWorker(companion_core.Settings(host="192.168.0.107"),
                                                 "test", media_source=monitor.snapshot)
        def send(*args, **kwargs):
            attempts.append(clock[0])
            raise OSError("offline")
        def wait(seconds):
            monitor._sample()
            clock[0] += seconds
            if clock[0] >= 4:
                worker._stop.set()
        with patch.object(companion_core, "NvidiaGpu"), \
             patch.object(companion_core.psutil, "cpu_percent"), \
             patch.object(companion_core, "collect_real", return_value={}), \
             patch.object(companion_core, "send_json", side_effect=send), \
             patch.object(companion_core.time, "monotonic", side_effect=lambda: clock[0]), \
             patch.object(worker._stop, "wait", side_effect=wait), \
             patch.object(media_provider, "snapshot", return_value={"available": True, "title": "Music"}), \
             patch.object(media_provider, "artwork_rgb565", return_value=None):
            worker._loop()
        self.assertEqual(attempts, [0, 1, 3])
        self.assertEqual(sum(e["type"] == "media" for e in updates), 4)

    def test_network_uses_music_cache_and_rejects_wrong_device(self):
        song = {"available": True, "title": "Cached song", "playing": False}
        worker = companion_core.CompanionWorker(companion_core.Settings(host="192.168.0.107"),
                                                 "test", media_source=lambda: (song, b"\0" * 8192))
        with patch.object(companion_core, "NvidiaGpu"), \
             patch.object(companion_core.psutil, "cpu_percent"), \
             patch.object(companion_core, "collect_real", return_value={}), \
             patch.object(companion_core, "send_json", return_value={"ok": True}) as send, \
             patch.object(companion_core, "json_request", return_value={"ok": True}) as request, \
             patch.object(companion_core, "post_artwork", return_value={"ok": True}), \
             patch.object(worker, "_process_commands"), patch.object(worker, "_poll_focus"), \
             patch.object(worker, "_poll_motion"), \
             patch.object(worker._stop, "wait", side_effect=lambda _: worker._stop.set()), \
             patch.object(media_provider, "snapshot") as provider:
            worker._loop()
            provider.assert_not_called()
            self.assertTrue(any(call.args[1] == "/api/v1/media" for call in request.call_args_list))
            send.assert_called_once()
        worker = companion_core.CompanionWorker(
            companion_core.Settings(host="192.168.0.107", device_id="expected"), "test")
        with patch.object(companion_core, "NvidiaGpu"), \
             patch.object(companion_core.psutil, "cpu_percent"), \
             patch.object(companion_core, "json_request", return_value={"device_id": "other"}), \
             patch.object(companion_core, "send_json") as send, \
             patch.object(worker._stop, "wait", side_effect=lambda _: worker._stop.set()):
            worker._loop()
            send.assert_not_called()

    def test_cover_arriving_after_metadata_is_sent_on_next_heartbeat(self):
        clock = [0.0]
        song = {"available": True, "title": "Track"}
        pixels = b"\0" * 8192
        source = Mock(side_effect=[(song, None), (song, pixels)])
        worker = companion_core.CompanionWorker(companion_core.Settings(host="192.168.0.107"),
                                                 "test", media_source=source)
        def wait(_):
            clock[0] += 2
            if clock[0] >= 4:
                worker._stop.set()
        with patch.object(companion_core, "NvidiaGpu"), \
             patch.object(companion_core.psutil, "cpu_percent"), \
             patch.object(companion_core, "collect_real", return_value={}), \
             patch.object(companion_core, "send_json", return_value={"ok": True}), \
             patch.object(companion_core, "json_request", return_value={"ok": True}), \
             patch.object(companion_core, "post_artwork", return_value={"ok": True}) as art, \
             patch.object(companion_core.time, "monotonic", side_effect=lambda: clock[0]), \
             patch.object(worker, "_process_commands"), patch.object(worker, "_poll_focus"), \
             patch.object(worker, "_poll_motion"), patch.object(worker._stop, "wait", side_effect=wait):
            worker._loop()
        art.assert_called_once()
        self.assertEqual(art.call_args.args[3], pixels)


@unittest.skipUnless(importlib.util.find_spec("PySide6"), "Qt optional")
class SingleInstanceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
        from PySide6.QtWidgets import QApplication
        cls.app = QApplication.instance() or QApplication([])

    def test_preview_and_live_can_coexist_but_live_cannot_duplicate(self):
        from single_instance import SingleInstance
        app = self.app
        with tempfile.TemporaryDirectory() as folder, \
             patch("single_instance.Path.home", return_value=Path(folder)):
            live = SingleInstance(directory=folder)
            preview = SingleInstance(preview=True, directory=folder)
            duplicate = SingleInstance(directory=folder)
            try:
                self.assertTrue(live.acquire())
                self.assertTrue(preview.acquire())
                self.assertFalse(duplicate.acquire())
                app.processEvents()
            finally:
                live.release()
                preview.release()

    def test_second_process_activates_existing_and_crash_is_recoverable(self):
        from single_instance import SingleInstance
        agent_path = str(Path(__file__).resolve().parents[1])
        with tempfile.TemporaryDirectory() as folder:
            name = "mimi-test-" + uuid.uuid4().hex
            code = f'''
import sys, os
sys.path.insert(0, {agent_path!r})
from PySide6.QtCore import QCoreApplication, QTimer
from single_instance import SingleInstance
app=QCoreApplication([])
owner=SingleInstance(name={name!r},directory={folder!r})
assert owner.acquire()
owner.activated.connect(lambda: (print("activated",flush=True),app.quit()))
QTimer.singleShot(4000,app.quit)
print("ready",flush=True)
app.exec()
os._exit(0)
'''
            child = subprocess.Popen([sys.executable, "-u", "-c", code], stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE, text=True)
            try:
                self.assertEqual(child.stdout.readline().strip(), "ready")
                second = SingleInstance(name=name, directory=folder)
                self.assertFalse(second.acquire())
                output, errors = child.communicate(timeout=6)
                self.assertEqual(child.returncode, 0, errors)
                self.assertIn("activated", output)
                # The first process exited without unlocking: stale lock/endpoint recovery.
                recovered = SingleInstance(name=name, directory=folder)
                try:
                    self.assertTrue(recovered.acquire())
                finally:
                    recovered.release()
            finally:
                if child.poll() is None:
                    child.kill()
                    child.communicate()


if __name__ == "__main__":
    unittest.main()
