"""Read-only hardware evidence recorder: failures must not become measurements."""
import csv
import io
import json
import sys
import tempfile
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch
from urllib.error import HTTPError, URLError

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import device_diagnostics as diagnostics


def status(**extra):
    return {"device": "MEO DESK", "device_id": "abc", "version": "1.7.0",
            "uptime_s": 100, "cat_fps": 24.6, "fps_target": 25,
            "internal_ram_kb": 180, "psram_free_kb": 6000, "pc_connected": True,
            "demo": False, **extra}


def record(payload=None, **data):
    return {"utc": "2026-11-20T10:00:00+00:00", "elapsed_s": 0,
            "data": {"status": status() if payload is None else payload, **data}, "errors": {}}


class DiagnosticsTests(unittest.TestCase):
    def test_lab_metrics_preserve_coverage_and_never_invent_missing_values(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        raw = status(theme=1, lab_enabled=True, lab_valid=True, lab_window_ms=5000,
                     lab_cpu_frame_ms=2.5, lab_dma_wait_ms=0.0,
                     lab_payload_bytes_s=1152000, lab_payload_coverage="cat_dma+buffered_card+album_only")
        clean = diagnostics.sanitized("status", raw)
        row = diagnostics.flatten(record(clean))
        self.assertEqual(row["lab_cpu_frame_ms"], 2.5)
        self.assertEqual(row["lab_dma_wait_ms"], 0.0)
        self.assertEqual(row["lab_payload_coverage"], "cat_dma+buffered_card+album_only")
        self.assertEqual(row["theme"], 1)
        self.assertNotIn("lab_bridge_wall_ms", row)

    def test_unknown_and_private_fields_are_not_saved(self):
        reader = diagnostics.DeviceReader("192.168.0.107")
        payload = status(wifi_ssid="PRIVATE", pc_name="PRIVATE", title="PRIVATE",
                         settings={"key": "PRIVATE"}, pairing_key="PRIVATE",
                         internal_dma_kb={"password": "PRIVATE"})
        replies = {"status": payload, "performance": {"fps": [24.6]},
                   "ota": {"ready": True, "message": "PRIVATE"},
                   "catpack": {"source": "custom", "secret": "PRIVATE"}}
        with patch.object(reader, "read", side_effect=lambda name: replies[name]):
            sample = reader.sample()
        self.assertNotIn("PRIVATE", json.dumps(sample))
        self.assertIsNone(sample["data"]["status"]["internal_dma_kb"])

    def test_real_device_fps_with_demo_pc_is_separate_from_simulation(self):
        real_demo = record(status(demo=True))
        fake = record(status(device="MEO DESK / MOCK", version="1.7.0-mock", cat_fps=30))
        summary = diagnostics.Summary("meodesk.local", 60, 5)
        self.assertEqual(summary.add(real_demo)["source"], "device_reported")
        self.assertEqual(summary.add(fake)["source"], "simulated")
        self.assertEqual(summary.value["pc_mode_counts"], {"demo": 1, "real": 1})
        buckets = summary.value["metrics_by_source"]
        self.assertEqual(buckets["device_reported"]["cat_fps"]["min"], 24.6)
        self.assertEqual(buckets["simulated"]["cat_fps"]["min"], 30)
        self.assertEqual(summary.value["hardware_acceptance"], "not_assessed")

    def test_missing_metrics_stay_missing_and_zero_is_preserved(self):
        summary = diagnostics.Summary("meodesk.local", 60, 5)
        row = summary.add(record({"device": "MEO DESK", "cat_fps": 0, "fps_target": 25}))
        self.assertEqual(row["cat_fps"], 0)
        self.assertNotIn("psram_free_kb", row)
        self.assertEqual(row["pc_mode"], "unknown")
        summary.add(record(status(cat_fps=None, internal_ram_kb=float("nan"))))
        metrics = summary.value["metrics_by_source"]["device_reported"]
        self.assertEqual(metrics["cat_fps"]["count"], 1)
        self.assertNotIn("internal_ram_kb", metrics)

    def test_optional_old_firmware_endpoints_are_probed_once_then_retried_on_upgrade(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        version = ["1.1.0"]

        def reply(name):
            if name == "status":
                return status(version=version[0])
            raise HTTPError("http://meodesk.local/", 404, "private text", {}, None)

        with patch.object(reader, "read", side_effect=reply) as read:
            first = reader.sample()
            second = reader.sample()
            self.assertEqual(read.call_count, 5)
            self.assertEqual(second["data"]["status"]["version"], "1.1.0")
            self.assertEqual(first["errors"]["ota"]["code"], "unsupported")
            version[0] = "1.7.0"
            reader.sample()
            self.assertEqual(read.call_count, 9)

    def test_outage_is_recorded_and_recovery_resumes_optional_reads(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        with patch.object(reader, "read", side_effect=URLError("PRIVATE")) as read:
            failed = reader.sample()
        self.assertEqual(read.call_count, 1)
        self.assertEqual(failed["errors"]["status"]["code"], "connection_error")
        self.assertNotIn("PRIVATE", json.dumps(failed))
        summary = diagnostics.Summary(reader.host, 60, 5)
        self.assertEqual(summary.add(dict(utc="test", elapsed_s=0, **failed))["source"], "unavailable")
        with patch.object(reader, "read", side_effect=lambda name: status() if name == "status" else {}):
            summary.add(dict(utc="test", elapsed_s=5, **reader.sample()))
        self.assertEqual(summary.value["status_errors"], 1)
        self.assertEqual(summary.value["status_samples"], 1)

    def test_non_device_response_does_not_trigger_other_requests(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        with patch.object(reader, "read", return_value={"device": "other"}) as read:
            result = reader.sample()
        self.assertEqual(read.call_count, 1)
        self.assertEqual(result["data"], {})
        self.assertEqual(result["errors"]["status"]["code"], "invalid_response")

    def test_uptime_drop_is_observation_not_proof_of_reboot(self):
        summary = diagnostics.Summary("meodesk.local", 60, 5)
        for uptime in (100, 110, 2, 7):
            summary.add(record(status(uptime_s=uptime)))
        summary.add(record(status(device_id="different", uptime_s=1)))
        self.assertEqual(summary.value["uptime_decreases"], 1)

    def test_response_size_and_json_object_are_bounded(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        for body in (b"x" * 32769, b"[]", b"not json"):
            with self.subTest(body=body[:20]), \
                    patch.object(reader.opener, "open", return_value=io.BytesIO(body)), \
                    self.assertRaises(ValueError):
                reader.read("status")

    def test_http_transport_uses_only_get_and_does_not_follow_redirect(self):
        requests = []

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *_):
                pass

            def do_GET(self):
                requests.append((self.command, self.path, dict(self.headers)))
                if self.path == "/api/v1/performance":
                    self.send_response(302)
                    self.send_header("Location", "/redirect-target")
                    self.end_headers()
                    return
                self.send_response(200)
                self.end_headers()
                self.wfile.write(json.dumps(status() if self.path == "/api/status" else {}).encode())

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        thread = threading.Thread(target=server.serve_forever, daemon=True)
        thread.start()
        try:
            reader = diagnostics.DeviceReader("127.0.0.1")
            # Test-only ephemeral server port; CLI intentionally uses device port 80.
            reader.host = f"127.0.0.1:{server.server_port}"
            result = reader.sample()
        finally:
            server.shutdown()
            server.server_close()
            thread.join(timeout=2)
        self.assertEqual(result["errors"]["performance"]["http_status"], 302)
        self.assertEqual([item[1] for item in requests], list(diagnostics.ENDPOINTS.values()))
        self.assertTrue(all(item[0] == "GET" and "X-Meo-Key" not in item[2] for item in requests))

    def test_interruption_saves_flushed_evidence_and_summary_without_overwriting(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        clock = [0.0]

        def wait(_):
            clock[0] += 2
            raise KeyboardInterrupt

        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "session"
            with patch.object(reader, "sample", return_value={"data": {"status": status()}, "errors": {}}), \
                    patch.object(diagnostics.time, "monotonic", side_effect=lambda: clock[0]), \
                    patch.object(diagnostics.time, "sleep", side_effect=wait), \
                    patch("builtins.print"):
                result = diagnostics.record_session(reader, output, 60, 5)
            self.assertEqual(result["stop_reason"], "interrupted")
            self.assertEqual(json.loads((output / "summary.json").read_text())["samples"], 1)
            evidence = (output / "samples.jsonl").read_text().splitlines()
            self.assertEqual(len(evidence), 1)
            with (output / "metrics.csv").open(newline="", encoding="utf-8") as file:
                rows = list(csv.DictReader(file))
            self.assertEqual(rows[0]["cat_fps"], "24.6")
            self.assertEqual(rows[0]["boot_health"], "")
            with self.assertRaises(FileExistsError):
                diagnostics.record_session(reader, output, 60, 5)

    def test_cli_rejects_invalid_duration_and_high_frequency_polling(self):
        for args in (("--duration", "nan"), ("--duration", "0"), ("--interval", "1")):
            with self.subTest(args=args), patch("sys.stderr", new=io.StringIO()), \
                    self.assertRaises(SystemExit) as error:
                diagnostics.main(["--host", "meodesk.local", *args])
            self.assertEqual(error.exception.code, 2)

    def test_normal_session_preserves_gaps_and_never_sums_overlapping_history(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        clock = [0.0]
        failed = {"data": {}, "errors": {"status": {"code": "connection_error"}}}
        healthy = {"data": {"status": status(cat_frames_skipped=3),
                            "performance": {"count": 2, "skipped": [100, 100]}}, "errors": {}}
        with tempfile.TemporaryDirectory() as folder:
            output = Path(folder) / "session"
            with patch.object(reader, "sample", side_effect=[failed, healthy, healthy]), \
                    patch.object(diagnostics.time, "monotonic", side_effect=lambda: clock[0]), \
                    patch.object(diagnostics.time, "sleep", side_effect=lambda delay: clock.__setitem__(0, clock[0] + delay)), \
                    patch("builtins.print"):
                result = diagnostics.record_session(reader, output, 15, 5)
            self.assertEqual(result["stop_reason"], "duration_elapsed")
            self.assertEqual(result["samples"], 3)
            self.assertEqual(result["status_samples"], 2)
            self.assertEqual(result["status_errors"], 1)
            self.assertEqual(result["metrics_by_source"]["device_reported"]["cat_frames_skipped"]["max"], 3)
            self.assertEqual(result["metrics_by_source"]["device_reported"]["cat_frames_skipped"]["mean"], 3)
            lines = (output / "samples.jsonl").read_text().splitlines()
            self.assertEqual([json.loads(line)["elapsed_s"] for line in lines], [0, 5, 10])

    def test_optional_transient_error_is_retried(self):
        reader = diagnostics.DeviceReader("meodesk.local")
        count = [0]

        def reply(name):
            if name == "status":
                return status()
            if name == "performance" and not count[0]:
                count[0] += 1
                raise HTTPError("http://meodesk.local/", 503, "busy", {}, None)
            return {}

        with patch.object(reader, "read", side_effect=reply):
            self.assertEqual(reader.sample()["errors"]["performance"]["http_status"], 503)
            self.assertIn("performance", reader.sample()["data"])


if __name__ == "__main__":
    unittest.main()
