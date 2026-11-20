"""Smoke tests for the optional offline web UI development mock."""

import json
import sys
import threading
import copy
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.request import ProxyHandler, Request, build_opener

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "firmware"))
from tools.web_preview_server import Handler, VERSION, STATE  # noqa: E402


class WebMockTests(unittest.TestCase):
    def test_config_restore_and_focus_plan_validate_before_commit(self):
        saved = copy.deepcopy(STATE)
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True); worker.start()
        direct = build_opener(ProxyHandler({}))
        base = f"http://127.0.0.1:{server.server_port}"
        def read(path):
            with direct.open(base+path) as response: return json.load(response)
        def post(path, data):
            request = Request(base+path,data=json.dumps(data).encode(),headers={'Content-Type':'application/json'},method='POST')
            with direct.open(request) as response: return json.load(response)
        try:
            self.assertFalse(post('/api/v1/focus/config',{'work_min':50,'break_min':10,'goal':6})['persisted'])
            status = read('/api/status')
            self.assertEqual(status['focus_duration_s'],3000)
            data = read('/api/v1/config'); data['profile']['scene_slots']=[0,12,2,5]
            self.assertTrue(post('/api/v1/config',data)['ok'])
            self.assertEqual(read('/api/v1/config')['profile']['scene_slots'],[0,12,2,5])
            data['profile']['scene_slots']=[0,0,2,5]
            from urllib.error import HTTPError
            with self.assertRaises(HTTPError): post('/api/v1/config',data)
            self.assertEqual(read('/api/v1/config')['profile']['scene_slots'],[0,12,2,5])
            self.assertEqual(read('/api/v1/ota/history')['updates'],[])
        finally:
            STATE.clear(); STATE.update(saved)
            server.shutdown(); server.server_close(); worker.join(timeout=2)

    def test_dashboard_history_and_preview_controls(self):
        server = ThreadingHTTPServer(("127.0.0.1", 0), Handler)
        worker = threading.Thread(target=server.serve_forever, daemon=True)
        worker.start()
        base = f"http://127.0.0.1:{server.server_port}"
        direct = build_opener(ProxyHandler({}))
        try:
            with direct.open(base + "/") as response:
                html = response.read().decode("utf-8")
                self.assertIn('id="historyCanvas"', html)
                self.assertIn('data-value="11"', html)
                self.assertIn('id="focusTitle"', html)
                self.assertIn('data-value="13"', html)
                self.assertIn('data-action="theme"', html)
                self.assertIn('data-action="fps"', html)
                self.assertIn('data-action="fps" data-value="3"', html)
                self.assertIn('id="fpsHistoryCanvas"', html)
                self.assertIn('id="otaFile"', html)
                self.assertIn('id="otaStart"', html)
            with direct.open(base + "/api/history") as response:
                history = json.load(response)
            self.assertEqual(history["mode"], "demo")
            self.assertEqual(len(history["cpu"]), 300)
            self.assertEqual(len(history["gpu"]), 300)
            self.assertEqual(len(history["ram"]), 300)
            with direct.open(base + "/api/v1/performance") as response:
                perf = json.load(response)
            self.assertEqual(perf["schema"], 1)
            self.assertEqual(perf["sample_period_s"], 5)
            self.assertEqual(perf["count"], 60)
            self.assertTrue(perf["mode"] == "demo")
            self.assertEqual(len(perf["fps"]), 60)
            self.assertEqual(perf["target"], [25] * 60)
            with direct.open(base + "/api/status") as response:
                status = json.load(response)
            self.assertTrue(status["demo"])
            self.assertEqual(status["page_count"], 14)
            self.assertEqual(status["version"], VERSION + "-mock")
            self.assertEqual(status["motion_style"], 1)
            self.assertEqual(status["motion_pc_intensity"], 0)
            with direct.open(base + "/api/v1/ota/status") as response:
                ota = json.load(response)
            self.assertEqual(ota["board"], "meodesk-esp32s3-n16r8")
            self.assertFalse(ota["ready"])
            self.assertEqual(ota["boot_health"], "passed")
            with direct.open(base + "/api/v1/catpack/status") as response:
                pack = json.load(response)
            self.assertFalse(pack["ready"])
            self.assertEqual(pack["pack_bytes"], 663552)
            self.assertEqual(status["scene_slots"], [12, 0, 5, 2])
            self.assertEqual(status["cat_persona"], "dancing")
            self.assertEqual(status["fps_target"], 25)
            self.assertTrue(status["psram_ready"])
            self.assertIsInstance(status["net_down_kbps"], float)
            self.assertIn("focus_remaining_s", status)
            with direct.open(base + "/api/v1/events") as response:
                self.assertEqual(json.load(response)["events"][0]["code"], "demo")
            with direct.open(base + "/api/cat.bmp?state=healthy&size=72") as response:
                self.assertEqual(response.headers.get_content_type(), "image/png")
                self.assertTrue(response.read().startswith(b"\x89PNG"))
            body = json.dumps({"action": "page", "value": 11}).encode()
            req = Request(base + "/api/control", data=body,
                          headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(req) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                self.assertEqual(json.load(response)["page"], 11)
            focus = Request(base + "/api/v1/focus", data=b'{"action":"toggle"}',
                            headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(focus) as response:
                self.assertTrue(json.load(response)["focus_running"])
            theme = Request(base + "/api/control", data=b'{"action":"theme","value":1}',
                            headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(theme) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                self.assertEqual(json.load(response)["theme"], 1)
            fps = Request(base + "/api/control", data=b'{"action":"fps","value":2}',
                          headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(fps) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                self.assertEqual(json.load(response)["fps_target"], 30)
            auto_fps = Request(base + "/api/control", data=b'{"action":"fps","value":3}',
                               headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(auto_fps) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                automatic = json.load(response)
            self.assertEqual(automatic["fps_profile"], 3)
            self.assertEqual(automatic["fps_target"], 25)
            self.assertTrue(automatic["fps_auto"])
            motion = Request(base + "/api/control",
                             data=b'{"action":"motion_style","value":2}',
                             headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(motion) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                changed = json.load(response)
            self.assertEqual(changed["motion_style"], 2)
            self.assertEqual(changed["motion_pc_intensity"], 0)  # DEMO not real load.
            scene = Request(base + "/api/control", data=b'{"action":"scene_2","value":1}',
                            headers={"Content-Type": "application/json"}, method="POST")
            with direct.open(scene) as response:
                self.assertTrue(json.load(response)["ok"])
            with direct.open(base + "/api/status") as response:
                self.assertEqual(json.load(response)["scene_slots"][2], 1)
        finally:
            server.shutdown()
            server.server_close()
            worker.join(timeout=2)
