"""Run with: python -m unittest discover -s windows_agent/tests -v"""

import json
import sys
import threading
import unittest
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from types import SimpleNamespace
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from pc_agent import (  # noqa: E402
    NetworkRateSampler,
    NvidiaGpu,
    collect_demo,
    collect_real,
    extract_lhm_gpu,
    percent,
    read_battery,
    send_json,
    target_url,
)


class AgentTests(unittest.TestCase):
    def test_percent_validation(self):
        self.assertIsNone(percent(None))
        self.assertIsNone(percent(float("nan")))
        self.assertIsNone(percent(101))
        self.assertIsNone(percent(-2))
        self.assertEqual(percent(26.789), 26.8)

    def test_lhm_amd_gpu_and_temperature(self):
        sample = {
            "Text": "DESKTOP",
            "Children": [
                {"Text": "AMD Ryzen", "Children": [
                    {"Text": "CPU Total", "Type": "Load", "Value": "87.0 %",
                     "SensorId": "/amdcpu/0/load/0", "Children": []},
                    {"Text": "Core (Tctl/Tdie)", "Type": "Temperature", "Value": "63.2 °C",
                     "SensorId": "/amdcpu/0/temperature/2", "Children": []},
                ]},
                {"Text": "AMD Radeon RX 6800", "Children": [
                    {"Text": "Load", "Children": [
                        {"Text": "GPU Core", "Type": "Load", "Value": "62,5 %",
                         "SensorId": "/gpu-amd/0/load/0", "Children": []},
                        {"Text": "GPU Memory", "Type": "Load", "Value": "20.0 %",
                         "SensorId": "/gpu-amd/0/load/2", "Children": []},
                    ]},
                    {"Text": "Temperatures", "Children": [
                        {"Text": "GPU Core", "Type": "Temperature", "Value": "51.7 °C",
                         "SensorId": "/gpu-amd/0/temperature/0", "Children": []}
                    ]}
                ]}
            ]
        }
        gpu = extract_lhm_gpu(sample)
        self.assertEqual(gpu.usage, 62.5)
        self.assertEqual(gpu.temp_c, 51.7)
        self.assertEqual(gpu.cpu_temp_c, 63.2)
        self.assertEqual(gpu.name, "AMD Radeon RX 6800")

    def test_lhm_missing_gpu_returns_null(self):
        obj = {"Text": "PC", "Children": [
            {"Text": "CPU Total", "Type": "Load", "Value": "51 %",
             "SensorId": "/intelcpu/0/load/0", "Children": []}
        ]}
        self.assertIsNone(extract_lhm_gpu(obj).usage)

    def test_real_metrics_no_gpu_are_not_faked(self):
        actual = collect_real(NvidiaGpu.disabled(), "none", "http://127.0.0.1:1/data.json")
        self.assertFalse(actual["demo"])
        self.assertIsNone(actual["gpu_percent"])
        self.assertGreater(actual["ram_total_gb"], 0)
        self.assertTrue(0 <= actual["cpu_percent"] <= 100)
        self.assertTrue(0 <= actual["ram_percent"] <= 100)

    def test_demo_is_explicitly_marked(self):
        data = collect_demo()
        self.assertTrue(data["demo"])
        self.assertIn("battery_percent", data)
        self.assertIn("cpu_temp_c", data)
        self.assertIn("net_up_kbps", data)
        self.assertIn("vram_total_gb", data)

    def test_network_rate_sampler_no_fake_first_value_or_counter_rollback(self):
        rates = NetworkRateSampler()
        counters = lambda sent, recv: SimpleNamespace(bytes_sent=sent, bytes_recv=recv)
        self.assertEqual(rates.sample(counters(1000, 2000), 10.0), (None, None))
        self.assertEqual(rates.sample(counters(3048, 6096), 11.0), (2.0, 4.0))
        self.assertEqual(rates.sample(counters(100, 200), 12.0), (None, None))
        self.assertEqual(rates.sample(counters(100, 200), 12.1), (None, None))
        self.assertEqual(rates.sample(None, 13.0), (None, None))

    def test_battery_sensor_is_real_or_null(self):
        with patch("pc_agent.psutil.sensors_battery", return_value=None):
            self.assertEqual(read_battery(), (None, None))
        sample = SimpleNamespace(percent=19.4, power_plugged=False)
        with patch("pc_agent.psutil.sensors_battery", return_value=sample):
            self.assertEqual(read_battery(), (19.4, False))

    def test_host_normalization(self):
        self.assertEqual(target_url("192.168.4.1"), "http://192.168.4.1/api/telemetry")
        self.assertEqual(target_url("http://meodesk.local/"), "http://meodesk.local/api/telemetry")

    def test_http_payload_and_key(self):
        observed = {}

        class Handler(BaseHTTPRequestHandler):
            def do_POST(self):
                observed["path"] = self.path
                observed["key"] = self.headers.get("X-MEO-KEY")
                observed["body"] = json.loads(self.rfile.read(int(self.headers["Content-Length"])))
                data = b'{"ok":true,"emotion":"happy"}'
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(data)))
                self.end_headers()
                self.wfile.write(data)

            def log_message(self, *args):
                pass

        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        try:
            reply = send_json(
                f"http://127.0.0.1:{server.server_port}/api/telemetry",
                "test-api-key",
                {"cpu_percent": 12.3, "gpu_percent": None, "demo": False},
            )
        finally:
            server.shutdown()
            server.server_close()
            worker.join(timeout=2)
        self.assertTrue(reply["ok"])
        self.assertEqual(observed["path"], "/api/telemetry")
        self.assertEqual(observed["key"], "test-api-key")
        self.assertIsNone(observed["body"]["gpu_percent"])


if __name__ == "__main__":
    unittest.main()
