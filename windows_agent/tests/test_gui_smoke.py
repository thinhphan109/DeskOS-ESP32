"""Construct each Companion tab in an offscreen Qt session (when available)."""
import importlib.util
import os
import sys
import unittest
import json
import time
from unittest.mock import patch
from pathlib import Path

os.environ.setdefault("QT_QPA_PLATFORM", "offscreen")
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))


@unittest.skipUnless(importlib.util.find_spec("PySide6"), "PySide6 optional in CLI-only installation")
class GuiSmokeTests(unittest.TestCase):
    def test_preview_controls_never_write_or_connect_to_device(self):
        from PySide6.QtWidgets import QApplication
        from companion import MainWindow
        app = QApplication.instance() or QApplication([])
        with patch("companion.json_request") as request, patch("companion.write_settings") as save:
            win = MainWindow(preview=True)
            try:
                win.send_device_control("theme", 2)
                win.send_device_control("motion_style", 0)
                win.save_device()
                win.start_agent()
                win.save_preferences()
                win.check_ota_status()
                win.start_ota_update()
                win.check_catpack_status()
                win.start_catpack_upload()
                win.scan_devices()
                win.refresh_device_state()
                app.processEvents()
                request.assert_not_called()
                save.assert_not_called()
                self.assertIsNone(win.worker)
                self.assertTrue(win.studio_buttons["theme"][2].isChecked())
                self.assertTrue(win.motion_style_buttons[0].isChecked())
                self.assertIn("DEMO", win.windowTitle())
            finally:
                win.quit_app()

    def test_two_clients_receive_same_authoritative_studio_state(self):
        from PySide6.QtCore import QUrl
        from PySide6.QtNetwork import QHostAddress
        from PySide6.QtWebSockets import QWebSocketServer
        from PySide6.QtWidgets import QApplication
        from companion_core import Settings
        from companion import MainWindow

        app = QApplication.instance() or QApplication([])
        server = QWebSocketServer("MeoDesk test", QWebSocketServer.SslMode.NonSecureMode)
        self.assertTrue(server.listen(QHostAddress.SpecialAddress.LocalHost, 0))
        peers = []
        server.newConnection.connect(lambda: peers.append(server.nextPendingConnection()))
        with patch("companion.read_settings", return_value=Settings()):
            windows = [MainWindow(), MainWindow()]
        try:
            for win in windows:
                win.state_timer.stop()
                win.state_socket.open(QUrl(f"ws://127.0.0.1:{server.serverPort()}/"))
            def until(condition):
                deadline = time.monotonic() + 3
                while time.monotonic() < deadline and not condition():
                    app.processEvents()
                    time.sleep(0.005)
                self.assertTrue(condition())
            until(lambda: len(peers) == 2)
            def publish(revision, theme):
                payload = json.dumps({"type": "state", "device": "MEO DESK",
                    "state_boot_id": "test-boot", "state_revision": revision,
                    "theme": theme, "page": 1, "fps_profile": 3, "motion_style": 2,
                    "motion_state": "focus", "cat_motion_frames": 3,
                    "scene_slots": [12, 0, 5, 2], "focus_remaining_s": 42,
                    "focus_running": False, "focus_phase": "work"})
                for peer in peers:
                    peer.sendTextMessage(payload)
            publish(1, 0)
            until(lambda: all(win.state_cursor.revision == 1 for win in windows))
            publish(2, 1)
            until(lambda: all(win.studio_buttons["theme"][1].isChecked() for win in windows))
            publish(1, 0)
            for _ in range(10):
                app.processEvents()
            for win in windows:
                self.assertTrue(win.studio_buttons["theme"][1].isChecked())
                self.assertTrue(win.studio_buttons["fps"][3].isChecked())
                self.assertTrue(win.motion_style_buttons[2].isChecked())
                self.assertEqual(win.studio_scene.currentData(), 1)
                self.assertEqual(win.focus_clock.text(), "00:42")
                self.assertIn("WebSocket", win.sync_label.text())
        finally:
            for win in windows:
                win.quit_app()
            for peer in peers:
                peer.close()
            server.close()

    def test_tabs_and_live_metrics(self):
        from PySide6.QtWidgets import QApplication
        from companion import MainWindow

        app = QApplication.instance() or QApplication([])
        win = MainWindow()
        try:
            for index in range(7):
                win.set_tab(index)
                self.assertEqual(win.stack.currentIndex(), index)
            win.update_from_worker({"type": "telemetry", "host": "192.168.0.107", "emotion": "healthy",
                                    "data": {"cpu_percent": 5.1, "gpu_percent": 0.0,
                                             "ram_percent": 52.7, "cpu_temp_c": None, "gpu_temp_c": 53.0}})
            self.assertEqual(win.cpu.text(), "5%")
            self.assertEqual(win.gpu.text(), "0%")
            win.update_from_worker({"type": "media", "data": {"available": False}})
            self.assertEqual(win.song.text(), "Chưa có bản nhạc")
            win.update_from_worker({"type": "artwork", "pixels": b"\x00\xf8" * 4096,
                                    "id": "a" * 16})
            self.assertIsNotNone(win.music_cover.pixmap())
            win.update_from_worker({"type": "artwork", "pixels": None, "id": ""})
            self.assertEqual(win.music_cover.text(), "ALBUM\nART N/A")
            win.update_from_worker({"type": "focus", "data": {"focus_remaining_s": 1390,
                "focus_running": True, "focus_phase": "work", "focus_sessions": 2}})
            self.assertEqual(win.focus_clock.text(), "23:10")
            win.update_from_worker({"type": "motion_status", "data": {
                "motion_state": "music", "motion_style": 2,
                "motion_budget_limited": True, "cat_motion_frames": 3, "motion_cel": 2}})
            self.assertTrue(win.motion_style_buttons[2].isChecked())
            self.assertIn("LITE", win.motion_live.text())
            app.processEvents()
        finally:
            win.quit_app()


if __name__ == "__main__":
    unittest.main()
