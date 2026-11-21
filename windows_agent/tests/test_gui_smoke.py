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
    @classmethod
    def setUpClass(cls):
        from PySide6.QtGui import QFontDatabase
        from PySide6.QtWidgets import QApplication
        cls.app = QApplication.instance() or QApplication([])
        # Qt's offscreen plugin on Windows does not load the native font catalog.
        if sys.platform == "win32":
            fonts = Path(os.environ.get("WINDIR", "C:/Windows")) / "Fonts"
            for filename in ("segoeui.ttf", "segoeuib.ttf"):
                QFontDatabase.addApplicationFont(str(fonts / filename))

    def test_pages_use_own_scroll_extent_after_leaving_tall_studio(self):
        from PySide6.QtTest import QTest
        from PySide6.QtWidgets import QApplication
        from companion import MainWindow
        from ui_theme import Disclosure
        app = QApplication.instance() or QApplication([])
        win = MainWindow(preview=True)
        try:
            win.show()
            for width, height in ((850, 560), (1120, 760), (1440, 960)):
                with self.subTest(size=(width, height)):
                    win.resize(width, height)
                    win.set_tab(6)
                    for _ in range(4):
                        app.processEvents()
                    bar = win.content_scroll.verticalScrollBar()
                    self.assertGreater(bar.maximum(), 400)
                    bar.setValue(bar.maximum())
                    for tab in (0, 2, 1, 3, 4):
                        win.set_tab(tab)
                        for _ in range(4):
                            app.processEvents()
                        self.assertEqual(bar.value(), 0)
                        self.assertEqual(win.content_scroll.horizontalScrollBar().maximum(), 0)
                        self.assertLess(bar.maximum(), 400)
                        if height >= 760 and tab == 2:
                            self.assertEqual(bar.maximum(), 0)
                        if height == 960:
                            self.assertEqual(bar.maximum(), 0)
            win.resize(1120, 760)
            win.set_tab(0)
            app.processEvents()
            diagnostics = win.stack.currentWidget().findChild(Disclosure)
            collapsed = win.content_scroll.verticalScrollBar().maximum()
            diagnostics.toggle.click()
            for _ in range(4):
                app.processEvents()
            self.assertGreater(win.content_scroll.verticalScrollBar().maximum(), collapsed)
            diagnostics.toggle.click()
            QTest.qWait(10)
            self.assertEqual(win.content_scroll.verticalScrollBar().maximum(), collapsed)
        finally:
            win.quit_app()

    def test_real_mode_starts_local_music_without_a_device_and_ignores_old_state(self):
        from companion import MainWindow
        from companion_core import Settings
        with patch("companion.read_settings", return_value=Settings()), \
             patch("companion.MediaMonitor") as factory, patch("companion.write_settings"):
            win = MainWindow()
            try:
                factory.return_value.start.assert_called_once()
                self.assertIsNone(win.worker)
                win._stream_seen_at = time.monotonic()
                win.update_from_worker({"type": "media", "source": "local",
                                        "data": {"available": True, "title": "Nhạc thật", "playing": True}})
                win.update_from_worker({"type": "media", "source": "state", "data": {"available": False}})
                self.assertEqual(win.song.text(), "Nhạc thật")
                win.update_from_worker({"type": "media", "source": "local", "media_generation": 0,
                                        "data": {"available": False}})
                self.assertEqual(win.song.text(), "Nhạc thật")
                win.media_check.setChecked(False)
                win.save_preferences()
                factory.return_value.stop.assert_called_once()
                self.assertIn("Đã tắt", win.artist.text())
            finally:
                win.quit_app()

    def test_relocation_requires_same_device_preserves_pause_and_pairing_error(self):
        from companion import MainWindow
        from companion_core import Settings
        from unittest.mock import Mock
        from PySide6.QtWidgets import QApplication
        with patch("companion.read_settings", return_value=Settings(host="192.168.0.107", device_id="expected")), \
             patch("companion.secret_store.load", return_value="test"), \
             patch("companion.secret_store.save") as vault, patch("companion.write_settings") as save:
            win = MainWindow(start_services=False)
            try:
                paused = Mock(running=False)
                win.worker = paused
                win.update_from_worker({"type": "device_relocated", "host": win.settings.host,
                                        "device_id": "other", "new_host": "meodesk.local"})
                save.assert_not_called()
                win.update_from_worker({"type": "device_relocated", "host": win.settings.host,
                                        "device_id": "expected", "new_host": "meodesk.local"})
                self.assertEqual(win.settings.host, "meodesk.local")
                self.assertIsNone(win.worker)
                paused.stop.assert_called_once()
                vault.assert_called_once_with("meodesk.local", "test")
                save.assert_called_once()
                win.device_seen_at = time.monotonic()
                win.update_from_worker({"type": "error", "host": win.settings.host, "code": 401})
                win.refresh_connection_badge()
                self.assertIn("mã ghép nối", win.connection_badge.text())
                win.update_from_worker({"type": "state_error", "host": win.settings.host})
                self.assertFalse(win._state_retry.ready(time.monotonic()))
                win.quit_app()
                before = win.song.text()
                win.update_from_worker({"type": "media", "data": {"available": True, "title": "Late callback"}})
                self.assertEqual(win.song.text(), before)
            finally:
                win.quit_app()

    def test_wheel_over_inputs_scrolls_page_without_changing_values(self):
        from PySide6.QtCore import QPoint, QPointF, Qt
        from PySide6.QtGui import QWheelEvent
        from PySide6.QtTest import QTest
        from PySide6.QtWidgets import QApplication, QComboBox, QSpinBox
        from companion import MainWindow
        from ui_controls import ScrollSafeComboBox, ScrollSafeSpinBox
        app = QApplication.instance() or QApplication([])
        win = MainWindow(preview=True)
        try:
            self.assertTrue(all(isinstance(w, ScrollSafeComboBox) for w in win.findChildren(QComboBox)))
            self.assertTrue(all(isinstance(w, ScrollSafeSpinBox) for w in win.findChildren(QSpinBox)))
            win.resize(850, 560)
            win.show()
            win.set_tab(6)
            app.processEvents()
            changed = []
            combo = win.favorite_scenes[0]
            combo.setCurrentIndex(2)
            combo.activated.connect(changed.append)
            spin = win.scene_editor.fields["x"]
            spin.setValue(10)
            bar = win.content_scroll.verticalScrollBar()
            self.assertGreater(bar.maximum(), 0)
            for control in (*win.favorite_scenes, win.studio_scene, spin):
                for focused in (False, True):
                    with self.subTest(control=type(control).__name__, focused=focused):
                        if focused:
                            control.setFocus()
                        else:
                            control.clearFocus()
                        value = control.currentIndex() if isinstance(control, QComboBox) else control.value()
                        for delta in (120, -120):
                            bar.setValue(bar.maximum() // 2)
                            before = bar.value()
                            pos = control.rect().center()
                            event = QWheelEvent(QPointF(pos), QPointF(control.mapToGlobal(pos)),
                                                QPoint(), QPoint(0, delta), Qt.MouseButton.NoButton,
                                                Qt.KeyboardModifier.NoModifier, Qt.ScrollPhase.NoScrollPhase, False)
                            app.sendEvent(control, event)
                            self.assertEqual(control.currentIndex() if isinstance(control, QComboBox) else control.value(), value)
                            if delta > 0:
                                self.assertLess(bar.value(), before)
                            else:
                                self.assertGreater(bar.value(), before)
            self.assertEqual(changed, [])
            combo.setFocus()
            QTest.keyClick(combo, Qt.Key.Key_Down)
            self.assertEqual(combo.currentIndex(), 3)
            self.assertEqual(changed, [3])
            QTest.keyClick(spin, Qt.Key.Key_Up)
            self.assertEqual(spin.value(), 11)
        finally:
            win.quit_app()

    def test_open_dropdown_keeps_popup_scrolling_and_click_selection(self):
        from PySide6.QtCore import QPoint, QPointF, Qt
        from PySide6.QtGui import QWheelEvent
        from PySide6.QtTest import QTest
        from PySide6.QtWidgets import QApplication
        from ui_controls import ScrollSafeComboBox
        app = QApplication.instance() or QApplication([])
        combo = ScrollSafeComboBox()
        try:
            combo.addItems([str(i) for i in range(100)])
            combo.view().setMaximumHeight(120)
            combo.show()
            combo.showPopup()
            app.processEvents()
            view = combo.view()
            bar = view.verticalScrollBar()
            self.assertGreater(bar.maximum(), 0)
            bar.setValue(bar.maximum() // 2)
            before = bar.value()
            pos = view.viewport().rect().center()
            event = QWheelEvent(QPointF(pos), QPointF(view.viewport().mapToGlobal(pos)),
                                QPoint(), QPoint(0, 120), Qt.MouseButton.NoButton,
                                Qt.KeyboardModifier.NoModifier, Qt.ScrollPhase.NoScrollPhase, False)
            app.sendEvent(view.viewport(), event)
            self.assertLess(bar.value(), before)
            index = view.indexAt(pos)
            self.assertTrue(index.isValid())
            QTest.mouseClick(view.viewport(), Qt.MouseButton.LeftButton, pos=pos)
            self.assertEqual(combo.currentIndex(), index.row())
            self.assertFalse(view.isVisible())
        finally:
            combo.hidePopup()
            combo.close()

    def test_rule_editor_invalid_draft_and_remote_preservation(self):
        from PySide6.QtWidgets import QApplication
        from rule_editor import RuleEditor
        from automation_rules import PRESET
        app=QApplication.instance() or QApplication([])
        editor=RuleEditor();editor.preset();self.assertTrue(editor.dirty)
        editor.set_data({'schema':1,'rules':[]})
        self.assertEqual(editor.data(),PRESET)
        editor.editor.setPlainText('{invalid')
        self.assertFalse(editor.save_button.isEnabled())
        editor.set_data({'schema':1,'rules':[]},force=True)
        self.assertTrue(editor.save_button.isEnabled());editor.close()
    def test_scene_editor_validates_moves_and_keeps_dirty_draft(self):
        from PySide6.QtWidgets import QApplication
        from scene_editor import SceneEditor
        from scene_layout import DEFAULT_LAYOUT
        app = QApplication.instance() or QApplication([])
        editor = SceneEditor()
        editor.fields["h"].setValue(45)
        self.assertFalse(editor.apply_button.isEnabled())
        editor.set_data(DEFAULT_LAYOUT)
        self.assertEqual(editor.data["widgets"][0]["h"],45)
        editor.set_data(DEFAULT_LAYOUT,force=True)
        self.assertTrue(editor.apply_button.isEnabled())
        editor.move(1,0,38)
        self.assertEqual(editor.data["widgets"][1]["y"],42)
        editor.fields["h"].setValue(36)
        editor.move(1,0,38)
        self.assertEqual(editor.data["widgets"][1]["y"],38)
        editor.close()

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
                self.assertIn("không đọc Spotify", win.preview_notice.text())
                self.assertIn("mẫu", win.song.text())
            finally:
                win.quit_app()

    def test_preview_opens_live_app_without_preview_or_pairing_draft(self):
        from PySide6.QtWidgets import QApplication
        import companion
        app = QApplication.instance() or QApplication([])
        win = companion.MainWindow(preview=True)
        try:
            win.host_edit.setText("192.0.2.25")
            with patch("companion.subprocess.Popen") as launch:
                win.open_live_button.click()
                win.open_live_app()
                launch.assert_called_once()
                command = launch.call_args.args[0]
                self.assertEqual(command, [sys.executable, str(Path(companion.__file__).resolve()), "--show"])
                self.assertFalse(win.open_live_button.isEnabled())
                self.assertTrue(win.preview_mode)
                self.assertIsNone(win.worker)
        finally:
            win.quit_app()

    def test_preview_live_launch_failure_can_retry_packaged_app(self):
        from PySide6.QtWidgets import QApplication
        import companion
        app = QApplication.instance() or QApplication([])
        win = companion.MainWindow(preview=True)
        try:
            with patch("companion.sys.frozen", True, create=True), patch("companion.subprocess.Popen") as launch:
                launch.side_effect = OSError("cannot launch")
                win.open_live_button.click()
                self.assertTrue(win.open_live_button.isEnabled())
                self.assertIn("Chưa mở được", win.preview_notice.text())
                launch.side_effect = None
                win.open_live_button.click()
                self.assertEqual(launch.call_args.args[0], [sys.executable, "--show"])
                self.assertFalse(win.open_live_button.isEnabled())
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
            windows = [MainWindow(start_services=False), MainWindow(start_services=False)]
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
        from companion_core import Settings
        with patch("companion.read_settings",return_value=Settings()):
            win = MainWindow(start_services=False)
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
            self.assertEqual(win.music_cover.text(), "Chưa có\nảnh bìa")
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
