"""Tests without Windows UI or real ESP32 hardware."""

import json
import sys
import tempfile
import time
import unittest
from pathlib import Path
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from tray_agent import AgentConfig, AgentWorker, dashboard_url, read_config  # noqa: E402


class TrayTests(unittest.TestCase):
    def test_config_defaults_and_overrides(self):
        with tempfile.TemporaryDirectory() as location:
            path = Path(location) / "config.json"
            with self.assertRaises(FileNotFoundError):
                read_config(path)
            path.write_text("{}", encoding="utf-8")
            self.assertEqual(read_config(path), AgentConfig())
            path.write_text(json.dumps({"host": "192.168.1.42", "interval": 0.5, "gpu": "none"}), encoding="utf-8")
            settings = read_config(path)
            self.assertEqual(settings.host, "192.168.1.42")
            self.assertEqual(settings.interval, 0.5)
            self.assertEqual(settings.gpu, "none")

    def test_config_rejects_invalid_values(self):
        with tempfile.TemporaryDirectory() as location:
            path = Path(location) / "config.json"
            for bad in ({"interval": 0}, {"gpu": "fake"}, {"host": ""}, {"demo": "yes"}, {"password": "not-a-field"}):
                path.write_text(json.dumps(bad), encoding="utf-8")
                with self.assertRaises((ValueError, TypeError)):
                    read_config(path)

    def test_dashboard_url(self):
        self.assertEqual(dashboard_url("192.168.4.1"), "http://192.168.4.1/")
        self.assertEqual(dashboard_url("http://192.168.1.42/"), "http://192.168.1.42/")

    def test_worker_retries_and_can_pause_then_resume(self):
        worker = AgentWorker(AgentConfig(host="127.0.0.1", gpu="none", interval=0.3, demo=True))
        sent = []

        def fake_send(url, key, data, timeout):
            sent.append((url, key, data["demo"]))
            return {"ok": True, "emotion": "demo"}

        with patch("tray_agent.send_json", side_effect=fake_send):
            worker.start()
            try:
                deadline = time.monotonic() + 2
                while not sent and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertTrue(sent)
                worker.toggle_pause()
                self.assertTrue(worker.paused)
                self.assertIn("Paused", worker.status)
                time.sleep(0.35)
                paused_count = len(sent)
                time.sleep(0.35)
                self.assertEqual(len(sent), paused_count)
                worker.toggle_pause()
                self.assertFalse(worker.paused)
                deadline = time.monotonic() + 2
                while len(sent) == paused_count and time.monotonic() < deadline:
                    time.sleep(0.01)
                self.assertGreater(len(sent), paused_count)
            finally:
                worker.stop()


if __name__ == "__main__":
    unittest.main()
