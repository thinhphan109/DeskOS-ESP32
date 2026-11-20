#!/usr/bin/env python3
"""MèoDesk Companion v1.7 — Windows dashboard and Living Motion Engine.

Install GUI requirements then double-click the packaged app. The original
pc_agent.py and tray_agent.py remain available as independent fallbacks.
"""
from __future__ import annotations

import sys
import json
import time
import threading
import webbrowser
from pathlib import Path

from PySide6.QtCore import QObject, Qt, Signal, QTimer, QUrl, QSignalBlocker
from PySide6.QtNetwork import QAbstractSocket, QNetworkProxy
from PySide6.QtWebSockets import QWebSocket
from PySide6.QtGui import QAction, QColor, QFont, QIcon, QImage, QPainter, QPainterPath, QPen, QPixmap
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QComboBox, QFrame, QGridLayout, QHBoxLayout, QLabel,
    QFileDialog, QLineEdit, QListWidget, QMainWindow, QMenu, QMessageBox, QProgressBar, QPushButton,
    QScrollArea, QStackedWidget, QSystemTrayIcon, QVBoxLayout, QWidget,
)

import discovery
import ota_client
import catpack_client
import secret_store
from companion_core import CompanionWorker, Settings, json_request, read_settings, write_settings
from state_sync import StateCursor, FocusDisplay


QSS = """
QMainWindow,QWidget#root { background: #f7f4ef; color: #2d3134; }
QLabel { color: #2d3134; font-family: 'Segoe UI'; }
QLabel#brand { color: #f7f2e9; font-size: 27px; font-weight: 800; }
QLabel#sideSmall { color: #adb8b1; font: 11px Consolas; letter-spacing: 1px; }
QFrame#sidebar { background: #25332f; border-radius: 0; }
QPushButton#nav { text-align: left; border: none; background: transparent; color: #a9b7af;
  border-radius: 12px; padding: 14px 19px; font: 600 13px 'Segoe UI'; }
QPushButton#nav:hover { background: #36493f; color: #fff; }
QPushButton#nav[chosen="true"] { background: #bce7d2; color: #1c4c3c; }
QLabel#eyebrow { color: #8c7774; font: 700 10px Consolas; letter-spacing: 2px; }
QLabel#title { color: #252e2b; font: 800 31px 'Segoe UI'; }
QLabel#subtitle { color: #8a8b88; font: 12px 'Segoe UI'; }
QFrame#card { background: #fffefa; border: 1px solid #e7e2da; border-radius: 19px; }
QFrame#mintCard { background: #d8f1e2; border: 1px solid #c5e7d4; border-radius: 20px; }
QFrame#focusCard { background: #e6dcf6; border: 1px solid #d9caed; border-radius: 20px; }
QLabel#focusClock { font: 800 72px Consolas; color: #382f54; }
QLabel#albumCover { background: #f2e8dc; border: 1px solid #d6cabb; border-radius: 13px;
  color: #837569; font: 700 11px Consolas; }
QFrame#darkCard { background: #292f34; border-radius: 20px; }
QLabel#bigNumber { font: 800 60px Consolas; color: #25332f; }
QLabel#darkNumber { font: 800 33px Consolas; color: #fff9ee; }
QLabel#darkMeta { font: 11px Consolas; color: #b8c0bf; }
QLabel#smallLabel { font: 11px Consolas; color: #7d817e; }
QLabel#cardTitle { font: 750 17px 'Segoe UI'; color: #2c3833; }
QPushButton#primary { background: #263e34; color: #ffffff; border: none; border-radius: 11px;
  padding: 12px 21px; font: 700 12px 'Segoe UI'; }
QPushButton#primary:hover { background: #345e48; }
QPushButton#secondary { background: #ffffff; color: #314438; border: 1px solid #d7ddd5;
  border-radius: 11px; padding: 11px 17px; font: 700 12px 'Segoe UI'; }
QPushButton#secondary:hover { border-color: #7dac96; background: #f7fff9; }
QLineEdit { background: #fffefa; color: #24312b; border: 1px solid #ddd9d0;
  border-radius: 10px; padding: 12px; font: 13px Consolas; }
QLineEdit:focus { border: 2px solid #70aa88; }
QListWidget { border: 1px solid #ddd9d0; background: #fffefa; border-radius: 10px;
  padding: 6px; color: #303a34; }
QListWidget::item { padding: 11px; border-radius: 8px; }
QListWidget::item:selected { background: #d7ebde; color: #2b4236; }
QCheckBox { color: #444d49; font: 12px 'Segoe UI'; spacing: 10px; }
QProgressBar { background: #eaece6; border-radius: 6px; height: 10px; border: 0; }
QProgressBar::chunk { background: #e38e7b; border-radius: 6px; }
QPushButton:checked { background: #d8f1e2; border: 1px solid #42866a; color: #1d513b; }
QPushButton:disabled { color: #7d817c; background: #e4e7e0; }
QComboBox { padding: 7px 10px; background: #fffefa; color: #24312b; border: 1px solid #d3d8ce; border-radius: 7px; min-height: 22px; }
QComboBox QAbstractItemView { background: #fffefa; color: #24312b; selection-background-color: #d8f1e2; }
QScrollBar:vertical { background: #eeeae3; width: 10px; }
QScrollBar::handle:vertical { background: #bbb9af; border-radius: 5px; min-height: 24px; }
"""


def text(value: str, role: str | None = None, wrap: bool = False) -> QLabel:
    node = QLabel(value)
    if role:
        node.setObjectName(role)
    if wrap:
        node.setWordWrap(True)
    return node


def card(kind: str = "card") -> tuple[QFrame, QVBoxLayout]:
    frame = QFrame()
    frame.setObjectName(kind)
    layout = QVBoxLayout(frame)
    layout.setContentsMargins(16, 14, 16, 14)
    layout.setSpacing(8)
    return frame, layout


def button(title: str, primary: bool = False) -> QPushButton:
    node = QPushButton(title)
    node.setObjectName("primary" if primary else "secondary")
    node.setCursor(Qt.CursorShape.PointingHandCursor)
    return node


def cat_icon() -> QIcon:
    from PySide6.QtGui import QPixmap
    px = QPixmap(64, 64)
    px.fill(Qt.GlobalColor.transparent)
    p = QPainter(px)
    p.setRenderHint(QPainter.RenderHint.Antialiasing)
    p.setBrush(QColor("#e5ae82"))
    p.setPen(QPen(QColor("#9c725b"), 2))
    path = QPainterPath()
    path.moveTo(11, 26)
    path.lineTo(11, 8)
    path.lineTo(26, 16)
    path.quadTo(32, 13, 38, 16)
    path.lineTo(53, 8)
    path.lineTo(53, 26)
    path.cubicTo(65, 61, -1, 61, 11, 26)
    p.drawPath(path)
    p.setBrush(QColor("#30353a"))
    p.drawEllipse(20, 31, 5, 6)
    p.drawEllipse(40, 31, 5, 6)
    p.setPen(QPen(QColor("#a95b56"), 2))
    p.drawArc(27, 39, 10, 8, 190 * 16, 160 * 16)
    p.end()
    return QIcon(px)


class Bus(QObject):
    status = Signal(object)
    discovery = Signal(object)
    ota = Signal(object)
    catpack = Signal(object)


class MainWindow(QMainWindow):
    def __init__(self, preview: bool = False) -> None:
        super().__init__()
        self.preview_mode = preview
        self.settings = Settings() if preview else read_settings()
        self.key = secret_store.load(self.settings.host) if self.settings.host else ""
        self.worker: CompanionWorker | None = None
        self._last_cat_emotion = ""
        self.state_cursor = StateCursor()
        self.focus_display = FocusDisplay()
        self._state_request_pending = False
        self._stream_seen_at = 0.0
        self._stream_supported = False
        self._stream_host = ""
        self.bus = Bus()
        self.bus.status.connect(self.update_from_worker)
        self.bus.discovery.connect(self.update_discovery)
        self.bus.ota.connect(self.update_ota)
        self.bus.catpack.connect(self.update_catpack)
        self.setWindowTitle("MèoDesk Personal Studio 1.8 dev")
        self.setWindowIcon(cat_icon())
        self.resize(1000, 700)
        self.setMinimumSize(790, 530)
        self.setStyleSheet(QSS + """
            QLabel#title { font-size: 25px; } QLabel#bigNumber { font-size: 46px; }
            QLabel#focusClock { font-size: 56px; } QFrame#card,QFrame#mintCard,QFrame#focusCard,QFrame#darkCard { border-radius: 12px; }
            QPushButton#nav { padding: 11px 12px; border-radius: 8px; }
            QPushButton#primary,QPushButton#secondary { padding: 9px 13px; border-radius: 8px; }
            QLineEdit { padding: 9px; border-radius: 8px; }
            QWidget#content,QWidget#tabPage,QStackedWidget,QScrollArea { background: #f7f4ef; }
            QLabel#subtitle,QLabel#smallLabel { color: #5c665e; }
            QPushButton#secondary:checked { background: #d8f1e2; border: 1px solid #42866a; color: #1d513b; }
        """)

        root = QWidget()
        root.setObjectName("root")
        self.setCentralWidget(root)
        frame = QHBoxLayout(root)
        frame.setContentsMargins(0, 0, 0, 0)
        frame.setSpacing(0)
        side = QFrame()
        side.setObjectName("sidebar")
        side.setFixedWidth(176)
        sidebox = QVBoxLayout(side)
        sidebox.setContentsMargins(14, 24, 14, 20)
        sidebox.setSpacing(6)
        sidebox.addWidget(text("mèo.desk", "brand"))
        sidebox.addWidget(text("PERSONAL STUDIO", "sideSmall"))
        sidebox.addSpacing(20)
        self.nav: list[QPushButton] = []
        for label in ("◈   Tổng quan", "◎   Thiết bị", "♫   Âm nhạc",
                      "◷   Tập trung", "⚙   Cài đặt", "↑   Firmware", "✦   Studio"):
            n = QPushButton(label)
            n.setObjectName("nav")
            n.setCursor(Qt.CursorShape.PointingHandCursor)
            n.clicked.connect(lambda _, i=len(self.nav): self.set_tab(i))
            self.nav.append(n)
            sidebox.addWidget(n)
        sidebox.addStretch()
        sidebox.addWidget(text("YOUR LITTLE DESK BUDDY\n\n● LOCAL / NO CLOUD", "sideSmall"))
        frame.addWidget(side)

        area = QScrollArea()
        area.setWidgetResizable(True)
        area.setFrameShape(QFrame.Shape.NoFrame)
        page = QWidget()
        page.setObjectName("content")
        area.setWidget(page)
        self.stack = QStackedWidget()
        pagebox = QVBoxLayout(page)
        pagebox.setContentsMargins(20, 20, 20, 20)
        pagebox.addWidget(self.stack)
        frame.addWidget(area, 1)
        self.make_overview()
        self.make_devices()
        self.make_music()
        self.make_focus()
        self.make_settings()
        self.make_updater()
        self.make_catpack()
        self.set_tab(0)
        self.make_tray()
        self.state_socket = QWebSocket(parent=self)
        self.state_socket.setProxy(QNetworkProxy(QNetworkProxy.ProxyType.NoProxy))
        self.state_socket.connected.connect(lambda: self.state_socket.sendTextMessage(
            json.dumps({"type": "subscribe", "key": self.key})))
        self.state_socket.textMessageReceived.connect(self.receive_socket_state)
        self.state_socket.disconnected.connect(self.stream_disconnected)
        self.state_timer = QTimer(self)
        self.state_timer.timeout.connect(self.maintain_state_link)
        self.state_timer.start(1500)
        self.display_timer = QTimer(self)
        self.display_timer.timeout.connect(self.tick_focus_display)
        self.display_timer.start(200)
        if preview:
            self.setWindowTitle("MèoDesk Studio — DEMO / xem trước")
            self._preview_state = {"device": "MEO DESK / MOCK", "version": "1.8.0-dev-mock",
                "page": 1, "theme": 1, "fps_profile": 1, "motion_style": 1,
                "motion_state": "music", "cat_motion_frames": 3, "motion": True,
                "auto_pages": False, "scene_slots": [12, 0, 5, 2], "pc_connected": True,
                "demo": True, "cpu_percent": 25, "gpu_percent": 14, "ram_percent": 60,
                "focus_remaining_s": 1500, "focus_phase": "work", "focus_running": False,
                "focus_sessions": 0, "emotion": "healthy"}
            self.update_from_worker({"type": "device_state", "data": self._preview_state})
            self.hint.setText("DEMO · dữ liệu mô phỏng · không kết nối hoặc cập nhật thiết bị")
            self.sync_label.setText("DEMO · xem trước trên máy")
            self.set_tab(6)
        if self.settings.host:
            self.host_edit.setText(self.settings.host)
            self.update_from_worker({"type": "state", "message": "Đang khởi tạo kết nối tự động..."})
            if self.key:
                QTimer.singleShot(250, self.start_agent)
            else:
                self.update_from_worker({"type": "state", "message": "Nhập pairing key ở tab Thiết bị để bắt đầu"})
        if self.settings.start_minimized:
            QTimer.singleShot(0, self.hide)

    def heading(self, root: QVBoxLayout, eyebrow: str, heading: str, sub: str) -> None:
        root.addWidget(text(heading, "title"))
        root.addWidget(text(sub, "subtitle", wrap=True))
        root.addSpacing(8)

    def make_overview(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "01 / LIVE STUDIO", "Cậu bạn nhỏ trên bàn.",
                     "Tất cả dữ liệu là số đo thật từ máy Windows, gửi qua Wi-Fi nội bộ.")
        panel = QHBoxLayout()
        panel.setSpacing(14)
        primary, p = card("mintCard")
        primary.setMaximumHeight(240)
        p.addWidget(text("CPU / LIVE", "eyebrow"))
        self.cpu = text("—%", "bigNumber")
        cpu_hero = QHBoxLayout()
        cpu_hero.addWidget(self.cpu, 1)
        self.cat_picture = QLabel()
        self.cat_picture.setFixedSize(126, 126)
        self.cat_picture.setAlignment(Qt.AlignmentFlag.AlignCenter)
        cpu_hero.addWidget(self.cat_picture, 0)
        p.addLayout(cpu_hero)
        p.addWidget(text("Năng lượng của chiếc máy hôm nay.", "smallLabel"))
        p.addStretch()
        self.state = text("CHƯA KẾT NỐI", "cardTitle", wrap=True)
        p.addWidget(self.state)
        self.set_cat_mood("healthy")
        panel.addWidget(primary, 3)
        dark, d = card("darkCard")
        dark.setMaximumHeight(240)
        for title in ("GPU LOAD", "RAM USED", "CPU / GPU TEMP"):
            d.addWidget(text(title, "darkMeta"))
            value = text("—", "darkNumber")
            d.addWidget(value)
            setattr(self, {"GPU LOAD": "gpu", "RAM USED": "ram", "CPU / GPU TEMP": "temps"}[title], value)
        panel.addWidget(dark, 2)
        box.addLayout(panel)
        bottom, b = card()
        row = QHBoxLayout()
        self.agent_button = button("▶  BẮT ĐẦU TRUYỀN DỮ LIỆU", True)
        self.agent_button.clicked.connect(self.toggle_agent)
        row.addWidget(self.agent_button)
        link = button("↗  WEB STUDIO")
        link.clicked.connect(self.open_web)
        row.addWidget(link)
        b.addLayout(row)
        self.hint = text("Chọn thiết bị và nhập pairing key để bắt đầu.", "subtitle", wrap=True)
        b.addWidget(self.hint)
        self.extra_metrics = text("NET ↓ — KB/s  ↑ — KB/s   ·   GPU VRAM — / — GB", "smallLabel", wrap=True)
        b.addWidget(self.extra_metrics)
        self.motion_live = text("LIVING MOTION / CHỜ KẾT NỐI ESP32", "smallLabel", wrap=True)
        b.addWidget(self.motion_live)
        self.sync_label = text("HTTP · chờ kết nối", "smallLabel")
        b.addWidget(self.sync_label)
        self.lab_live = text("Performance Lab · chưa có dữ liệu đo", "smallLabel", wrap=True)
        b.addWidget(self.lab_live)
        self.lab_toggle = button("Bật đo Performance Lab")
        self.lab_toggle.setCheckable(True)
        self.lab_toggle.setEnabled(False)
        self.lab_toggle.clicked.connect(lambda checked: self.send_device_control("perf_trace", int(checked)))
        b.addWidget(self.lab_toggle)
        box.addWidget(bottom)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_devices(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "02 / DEVICE LINK", "Tìm MèoDesk của bạn.",
                     "Ưu tiên IP đã lưu → meodesk.local → hotspot → quét mạng LAN khi bạn yêu cầu.")
        panel, p = card()
        p.addWidget(text("ĐỊA CHỈ THIẾT BỊ", "eyebrow"))
        self.host_edit = QLineEdit(self.settings.host)
        self.host_edit.setPlaceholderText("Ví dụ: 192.168.0.107 hoặc meodesk.local")
        p.addWidget(self.host_edit)
        p.addWidget(text("PAIRING KEY / KHÔNG PHẢI MẬT KHẨU WI-FI", "eyebrow"))
        self.key_edit = QLineEdit(self.key)
        self.key_edit.setEchoMode(QLineEdit.EchoMode.Password)
        self.key_edit.setPlaceholderText("Khóa API đã cấu hình trong firmware")
        p.addWidget(self.key_edit)
        actions = QHBoxLayout()
        apply_btn = button("✓  LƯU VÀ KẾT NỐI", True)
        apply_btn.clicked.connect(self.save_device)
        actions.addWidget(apply_btn)
        self.scan_btn = button("⌕  QUÉT IP TRONG LAN")
        self.scan_btn.clicked.connect(self.scan_devices)
        actions.addWidget(self.scan_btn)
        p.addLayout(actions)
        box.addWidget(panel)
        scan, s = card()
        s.addWidget(text("THIẾT BỊ PHÁT HIỆN", "eyebrow"))
        self.found = QListWidget()
        self.found.setMinimumHeight(100)
        self.found.setMaximumHeight(150)
        self.found.itemDoubleClicked.connect(lambda _: self.select_device())
        s.addWidget(self.found)
        select_btn = button("DÙNG THIẾT BỊ ĐƯỢC CHỌN")
        select_btn.clicked.connect(self.select_device)
        s.addWidget(select_btn)
        box.addWidget(scan)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_music(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "03 / NOW PLAYING", "Nhạc đang chạm tới tai.",
                     "Đọc trực tiếp Windows Media Session; không nhạc sẽ hiển thị đúng 'chưa phát'.")
        panel, p = card("mintCard")
        p.addWidget(text("WINDOWS MEDIA SESSION", "eyebrow"))
        music_hero = QHBoxLayout()
        left = QVBoxLayout()
        self.song = text("Chưa có bản nhạc", "title", wrap=True)
        left.addWidget(self.song)
        self.artist = text("Mở Spotify hoặc trình phát hỗ trợ Windows Media Controls.", "subtitle", wrap=True)
        left.addWidget(self.artist)
        music_hero.addLayout(left, 1)
        self.music_cover = text("ALBUM\nART N/A", "albumCover")
        self.music_cover.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.music_cover.setFixedSize(116, 116)
        music_hero.addWidget(self.music_cover)
        music_cat = QLabel()
        music_pixmap = QPixmap(str(Path(__file__).resolve().parent / "assets" / "music.png"))
        music_cat.setPixmap(music_pixmap.scaled(106, 106, Qt.AspectRatioMode.KeepAspectRatio,
                                                Qt.TransformationMode.SmoothTransformation))
        music_cat.setFixedSize(108, 108)
        music_hero.addWidget(music_cat)
        p.addLayout(music_hero)
        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.progress.setValue(0)
        self.progress.setTextVisible(False)
        p.addWidget(self.progress)
        self.music_state = text("— / —", "smallLabel")
        p.addWidget(self.music_state)
        box.addWidget(panel)
        note, n = card()
        n.addWidget(text("Điều khiển từ Web Studio", "cardTitle"))
        n.addWidget(text("Bản v1.0 hiển thị ảnh bìa album khi Windows cung cấp, đồng bộ lên TFT."
                         " Lệnh phát/dừng và chuyển bài vẫn điều khiển qua ESP32."
                         " Chức năng phụ thuộc trình phát hỗ trợ Windows Media Session.", "subtitle", wrap=True))
        box.addWidget(note)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_focus(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "04 / DEEP FOCUS", "Dành một chút thời gian.",
                     "Đồng hồ chạy trên ESP32; đóng ứng dụng Windows vẫn không làm mất phiên Focus.")
        panel, layout = card("focusCard")
        layout.addWidget(text("POMODORO / 25 MIN FOCUS + 5 MIN BREAK", "eyebrow"))
        hero = QHBoxLayout()
        left = QVBoxLayout()
        self.focus_clock = text("25:00", "focusClock")
        left.addWidget(self.focus_clock)
        self.focus_phase = text("SẴN SÀNG / CHƯA BẮT ĐẦU", "cardTitle", wrap=True)
        left.addWidget(self.focus_phase)
        self.focus_sessions = text("0 phiên đã hoàn thành", "smallLabel")
        left.addWidget(self.focus_sessions)
        hero.addLayout(left, 1)
        kitty = QLabel()
        art = QPixmap(str(Path(__file__).resolve().parent / "assets" / "paused.png"))
        kitty.setPixmap(art.scaled(146, 146, Qt.AspectRatioMode.KeepAspectRatio,
                                   Qt.TransformationMode.SmoothTransformation))
        kitty.setFixedSize(147, 147)
        hero.addWidget(kitty)
        layout.addLayout(hero)
        self.focus_progress = QProgressBar()
        self.focus_progress.setRange(0, 100)
        self.focus_progress.setValue(0)
        self.focus_progress.setTextVisible(False)
        layout.addWidget(self.focus_progress)
        box.addWidget(panel)
        actions, action_box = card()
        row = QHBoxLayout()
        self.focus_toggle = button("▶  BẮT ĐẦU / TẠM DỪNG", True)
        self.focus_toggle.clicked.connect(lambda: self.change_focus("toggle"))
        row.addWidget(self.focus_toggle)
        reset = button("↺  RESET")
        reset.clicked.connect(lambda: self.change_focus("reset"))
        row.addWidget(reset)
        skip = button("⇥  ĐỔI PHIÊN")
        skip.clicked.connect(lambda: self.change_focus("skip"))
        row.addWidget(skip)
        action_box.addLayout(row)
        self.focus_hint = text("Chờ liên kết với firmware hỗ trợ Focus (v0.9+).", "subtitle")
        action_box.addWidget(self.focus_hint)
        box.addWidget(actions)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_settings(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "05 / PREFERENCES", "Đúng chất của bạn.",
                     "Dữ liệu nằm trong mạng nội bộ, không gửi lên máy chủ bên ngoài.")
        panel, p = card()
        self.media_check = QCheckBox("Bật Now Playing (Windows Media Controls)")
        self.media_check.setChecked(self.settings.media)
        p.addWidget(self.media_check)
        self.min_check = QCheckBox("Mở thu nhỏ vào khay hệ thống từ lần chạy sau")
        self.min_check.setChecked(self.settings.start_minimized)
        p.addWidget(self.min_check)
        save_btn = button("LƯU TÙY CHỌN", True)
        save_btn.clicked.connect(self.save_preferences)
        p.addWidget(save_btn)
        p.addWidget(text("Mỗi 1 giây: CPU/GPU/RAM. Mỗi ~2 giây: media nếu được bật. "
                         "Cần khởi động lại phiên truyền để nhận thay đổi.", "subtitle", wrap=True))
        box.addWidget(panel)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_updater(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "06 / LOCAL FIRMWARE UPDATER", "Một bộ não mới cho mèo.",
                     "Cập nhật firmware ESP32-S3 N16R8 qua Wi-Fi. Chỉ dùng trên mạng nội bộ của bạn.")

        summary, s = card("mintCard")
        s.addWidget(text("DEVICE / A-B FIRMWARE UPDATE", "eyebrow"))
        self.ota_info = text("Chưa kiểm tra thiết bị. Nhập địa chỉ và pairing key ở tab Thiết bị.", "cardTitle", wrap=True)
        s.addWidget(self.ota_info)
        self.ota_check = button("⌕  KIỂM TRA OTA TRÊN ESP32")
        self.ota_check.clicked.connect(self.check_ota_status)
        s.addWidget(self.ota_check)
        box.addWidget(summary)

        update, u = card()
        u.addWidget(text("CHỌN FIRMWARE ĐÃ BUILD", "eyebrow"))
        line = QHBoxLayout()
        self.ota_path = QLineEdit()
        self.ota_path.setPlaceholderText("firmware.bin từ PlatformIO / .pio/build/…")
        line.addWidget(self.ota_path, 1)
        browse = button("CHỌN FILE")
        browse.clicked.connect(self.browse_ota_file)
        line.addWidget(browse)
        u.addLayout(line)
        self.ota_progress = QProgressBar()
        self.ota_progress.setRange(0, 100)
        self.ota_progress.setValue(0)
        u.addWidget(self.ota_progress)
        self.ota_hint = text("Sau khi upload, app sẽ tự kiểm tra phân vùng mới và sức khỏe khởi động (~30–85 giây).", "subtitle", wrap=True)
        u.addWidget(self.ota_hint)
        self.ota_start = button("↑  XÁC NHẬN VÀ CẬP NHẬT QUA WI-FI", True)
        self.ota_start.clicked.connect(self.start_ota_update)
        u.addWidget(self.ota_start)
        box.addWidget(update)

        warning, w = card("focusCard")
        w.addWidget(text("AN TOÀN / ĐỌC TRƯỚC KHI CẬP NHẬT", "eyebrow"))
        w.addWidget(text("• Lần đầu từ v1.2/huge_app phải chuyển phân vùng qua USB và sao lưu NVS.\n"
                         "• Cấp nguồn ổn định trong toàn bộ thời gian cập nhật.\n"
                         "• Đây là OTA nội bộ chưa xác minh chữ ký số; không mở cổng HTTP ra Internet.\n"
                         "• Rollback tự động chỉ có nếu bootloader thực sự hỗ trợ."
                         , "subtitle", wrap=True))
        box.addWidget(warning)
        box.addStretch()
        self.stack.addWidget(widget)

    def browse_ota_file(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Chọn firmware MèoDesk", "", "ESP32 firmware (*.bin)")
        if filename:
            self.ota_path.setText(filename)

    def check_ota_status(self) -> None:
        if self.preview_mode:
            self.ota_hint.setText("DEMO · không kết nối thiết bị")
            return
        host = self.host_edit.text().strip()
        self.ota_check.setEnabled(False)
        self.ota_info.setText("Đang kiểm tra thiết bị và phân vùng…")

        def work():
            try:
                data = ota_client.read_ota_status(host)
                self.bus.ota.emit({"type": "status", "data": data})
            except Exception as error:
                self.bus.ota.emit({"type": "error", "message": str(error)[:150]})

        threading.Thread(target=work, name="MeoDeskOtaProbe", daemon=True).start()

    def start_ota_update(self) -> None:
        if self.preview_mode:
            self.ota_hint.setText("DEMO · không nạp firmware trong bản xem trước")
            return
        host = self.host_edit.text().strip()
        key = self.key_edit.text().strip()
        image = self.ota_path.text().strip()
        if not host or not key:
            self.ota_hint.setText("Cần IP và pairing key ở tab Thiết bị.")
            return
        try:
            length, sha = ota_client.validate_image(image)
        except (ValueError, OSError) as error:
            self.ota_hint.setText(str(error))
            return
        choice = QMessageBox.question(
            self, "Xác nhận cập nhật firmware",
            f"Cập nhật MèoDesk tại {host}?\n\n{Path(image).name}\n{length / 1048576:.2f} MiB\n"
            f"SHA-256: {sha[:16]}…\n\nGiữ nguồn điện ổn định. Tiếp tục?",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if choice != QMessageBox.StandardButton.Yes:
            return
        self.ota_start.setEnabled(False)
        self.pack_start.setEnabled(False)
        self.ota_check.setEnabled(False)
        self.ota_progress.setValue(0)
        self.ota_hint.setText("Kiểm tra phân vùng và gửi firmware tới ESP32…")

        def work():
            last_percent = -1

            def progress(sent: int, size: int) -> None:
                nonlocal last_percent
                percent = min(99, int(100 * sent / max(1, size)))
                if percent != last_percent:
                    last_percent = percent
                    self.bus.ota.emit({"type": "progress", "percent": percent})

            try:
                result = ota_client.upload_firmware(host, key, image, progress)
            except Exception as error:
                self.bus.ota.emit({"type": "upload_error", "message": str(error)[:200]})
                return
            self.bus.ota.emit({"type": "uploaded", "data": result})
            try:
                verified = ota_client.wait_for_boot(
                    host, result["target_slot"],
                    status_callback=lambda message: self.bus.ota.emit({"type": "verifying", "message": message}),
                )
            except Exception as error:
                self.bus.ota.emit({"type": "verify_error", "message": str(error)[:200]})
                return
            self.bus.ota.emit({"type": "confirmed", "data": verified})

        threading.Thread(target=work, name="MeoDeskOtaUpload", daemon=True).start()

    def update_ota(self, event: dict) -> None:
        kind = event.get("type")
        if kind == "status":
            data = event["data"]
            availability = "SẴN SÀNG A/B" if data.get("ready") else "CHƯA CÓ HAI SLOT OTA"
            rollback = "HỖ TRỢ" if data.get("bootloader_rollback_configured") else "CHƯA XÁC NHẬN"
            self.ota_info.setText(f"Firmware {data.get('version', '—')} · {availability}\n"
                                  f"Slot: {data.get('running_slot', '—')} → {data.get('inactive_slot', '—')} "
                                  f"· Rollback: {rollback}")
            self.ota_check.setEnabled(True)
        elif kind == "progress":
            self.ota_progress.setValue(event["percent"])
            self.ota_hint.setText(f"Đang gửi firmware… {event['percent']}%")
        elif kind == "uploaded":
            self.ota_progress.setValue(100)
            self.ota_hint.setText("SHA-256 đúng. Đã gửi xong; đợi khởi động và tự kiểm tra ESP32…")
        elif kind == "verifying":
            self.ota_hint.setText(event["message"])
        elif kind == "confirmed":
            data = event["data"]
            self.ota_info.setText(f"FIRMWARE ĐÃ KHỞI ĐỘNG · {data.get('version', '—')}\n"
                                  f"Phân vùng: {data.get('running_slot', '—')} · Startup check: PASS")
            self.ota_hint.setText("✓ XÁC NHẬN THÀNH CÔNG: ESP32 đã chạy phân vùng mới và qua kiểm tra khởi động.")
            self.ota_start.setEnabled(True)
            self.ota_check.setEnabled(True)
            self.pack_start.setEnabled(True)
        elif kind in ("error", "upload_error", "verify_error"):
            self.ota_info.setText("Không thể kiểm tra OTA" if kind == "error" else self.ota_info.text())
            prefix = "⚠ Upload xong nhưng chưa xác nhận được boot: " if kind == "verify_error" else ""
            self.ota_hint.setText(prefix + event.get("message", "OTA failed"))
            self.ota_check.setEnabled(True)
            self.ota_start.setEnabled(True)
            self.pack_start.setEnabled(True)

    def make_catpack(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "STUDIO", "Cá nhân hóa góc bàn.",
                     "Cảnh, giao diện và chuyển động lấy trạng thái chung từ ESP32.")
        controls, layout = card()
        self.studio_scene = QComboBox()
        self.scene_names = [(12, "Home"), (0, "PC"), (1, "Đồng hồ"), (2, "Focus"),
                            (3, "Wi-Fi"), (4, "Thiết bị"), (5, "Nhạc"), (6, "Print"),
                            (7, "Audio"), (8, "Moto"), (9, "Ngủ"), (10, "Health"),
                            (11, "Trends"), (13, "Network")]
        for page, caption in self.scene_names:
            self.studio_scene.addItem(caption, page)
        self.studio_scene.activated.connect(lambda _: self.send_device_control("page", self.studio_scene.currentData()))
        scene_row = QHBoxLayout()
        scene_row.addWidget(text("Cảnh trên TFT", "smallLabel"))
        scene_row.addWidget(self.studio_scene, 1)
        layout.addLayout(scene_row)
        toggle_row = QHBoxLayout()
        self.studio_toggles = {}
        for action, caption in (("motion", "Chuyển động mèo"), ("auto", "Tự chuyển cảnh")):
            entry = button(caption)
            entry.setCheckable(True)
            entry.clicked.connect(lambda checked=False, a=action: self.send_device_control(a, 0))
            self.studio_toggles[action] = entry
            toggle_row.addWidget(entry)
        layout.addLayout(toggle_row)
        self.studio_buttons = {}
        for action, label, captions in (("theme", "Giao diện TFT", ("Cozy Day", "Midnight", "Auto")),
                                        ("fps", "Mục tiêu FPS", ("20", "25", "30", "Auto 20–25"))):
            row = QHBoxLayout()
            row.addWidget(text(label, "smallLabel"))
            self.studio_buttons[action] = []
            for value, caption in enumerate(captions):
                entry = button(caption)
                entry.setCheckable(True)
                entry.clicked.connect(lambda checked=False, a=action, v=value: self.send_device_control(a, v))
                self.studio_buttons[action].append(entry)
                row.addWidget(entry)
            layout.addLayout(row)
        layout.addWidget(text("Bốn cảnh yêu thích · dùng cho chế độ Auto", "smallLabel"))
        favorites = QGridLayout()
        self.favorite_scenes = []
        for index in range(4):
            combo = QComboBox()
            for page, caption in self.scene_names:
                combo.addItem(caption, page)
            combo.activated.connect(lambda _, slot=index: self.save_scene_favorite(slot))
            self.favorite_scenes.append(combo)
            favorites.addWidget(combo, index // 2, index % 2)
        layout.addLayout(favorites)
        box.addWidget(controls)

        details, d = card("focusCard")
        d.addWidget(text("CAT COLLECTION / ACTIVE BANK", "eyebrow"))
        self.pack_info = text("Đang chờ kết nối với ESP32-S3 N16R8.", "cardTitle", wrap=True)
        d.addWidget(self.pack_info)
        self.pack_check = button("⌕  KIỂM TRA BỘ ẢNH MÈO")
        self.pack_check.clicked.connect(self.check_catpack_status)
        d.addWidget(self.pack_check)
        d.addWidget(text("LIVING MOTION / ĐIỀU KHIỂN TRÊN ESP32", "eyebrow"))
        self.pack_motion_status = text("Chưa nhận trạng thái chuyển động từ firmware v1.7.",
                                       "subtitle", wrap=True)
        d.addWidget(self.pack_motion_status)
        choices = QHBoxLayout()
        self.motion_style_buttons = []
        for style, caption in enumerate(("☾ CALM", "✦ NATURAL", "♫ PLAYFUL")):
            entry = button(caption)
            entry.setCheckable(True)
            entry.clicked.connect(lambda checked=False, s=style: self.set_motion_style(s))
            choices.addWidget(entry)
            self.motion_style_buttons.append(entry)
        d.addLayout(choices)
        box.addWidget(details)

        update, u = card()
        u.addWidget(text(".MEOCAT · 648 KiB TĨNH / 1.944 KiB ĐỘNG", "eyebrow"))
        file_row = QHBoxLayout()
        self.pack_path = QLineEdit()
        self.pack_path.setPlaceholderText("Bộ ảnh tạo bằng tools/make_catpack.py")
        file_row.addWidget(self.pack_path, 1)
        choose = button("CHỌN BỘ ẢNH")
        choose.clicked.connect(self.browse_catpack_file)
        file_row.addWidget(choose)
        u.addLayout(file_row)
        self.pack_progress = QProgressBar()
        self.pack_progress.setRange(0, 100)
        self.pack_progress.setValue(0)
        u.addWidget(self.pack_progress)
        self.pack_hint = text("Ảnh mới được kiểm tra SHA-256, ESP32 khởi động lại và xác nhận đã nạp vào PSRAM.",
                              "subtitle", wrap=True)
        u.addWidget(self.pack_hint)
        self.pack_start = button("✦  CÀI BỘ MÈO MỚI QUA WI-FI", True)
        self.pack_start.clicked.connect(self.start_catpack_upload)
        u.addWidget(self.pack_start)
        box.addWidget(update)

        tip, t = card("mintCard")
        t.addWidget(text("CÁCH TẠO BỘ MÈO CỦA BẠN", "eyebrow"))
        t.addWidget(text("Dùng 16 PNG 144×144 và --motion-demo để tạo 48 khung demo; "
                         "hoặc vẽ 48 ảnh theo tên boot_0.png…shutdown_2.png rồi dùng --motion. "
                         "Chạy python tools/make_catpack.py --motion-demo --out motion.meocat "
                         "(cần Pillow một lần). Không cần biên dịch lại PlatformIO. "
                         "Không mở API này ra Internet.", "subtitle", wrap=True))
        box.addWidget(tip)
        box.addStretch()
        self.stack.addWidget(widget)

    def browse_catpack_file(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Bộ mèo MèoDesk", "", "MèoDesk Cat Pack (*.meocat)")
        if filename:
            self.pack_path.setText(filename)

    def set_motion_style(self, style: int) -> None:
        self.send_device_control("motion_style", style)

    def save_scene_favorite(self, slot: int) -> None:
        choices = [combo.currentData() for combo in self.favorite_scenes]
        if len(set(choices)) != 4:
            self.pack_motion_status.setText("Chọn bốn cảnh khác nhau. Trạng thái sẽ được tải lại từ ESP32.")
            self.refresh_device_state()
            return
        self.send_device_control(f"scene_{slot}", choices[slot])

    def send_device_control(self, action: str, value: int) -> None:
        if self.preview_mode:
            field = {"page": "page", "theme": "theme", "fps": "fps_profile", "motion_style": "motion_style", "perf_trace": "lab_enabled"}.get(action)
            if field:
                self._preview_state[field] = value
            elif action in ("motion", "auto"):
                field = "motion" if action == "motion" else "auto_pages"
                self._preview_state[field] = not self._preview_state[field]
            elif action.startswith("scene_"):
                self._preview_state["scene_slots"][int(action[-1])] = value
            self.update_from_worker({"type": "device_state", "data": self._preview_state})
            self.pack_motion_status.setText("DEMO · thay đổi chỉ áp dụng trong bản xem trước")
            return
        host = self.host_edit.text().strip() or self.settings.host
        key = self.key_edit.text().strip() or self.key
        if not host or not key:
            self.pack_motion_status.setText("Nhập IP và pairing key trong tab Thiết bị trước.")
            return
        self.pack_motion_status.setText("Đang gửi thay đổi đến ESP32…")

        def work() -> None:
            try:
                result = json_request(host, "/api/control", key,
                                      {"action": action, "value": value}, timeout=1.8)
                if not result.get("ok"):
                    raise OSError("ESP32 không chấp nhận thay đổi. Kiểm tra kết nối và firmware.")
                status = json_request(host, "/api/v1/status", timeout=2.0)
                self.bus.status.emit({"type": "device_state", "data": status, "source": "http"})
            except Exception as error:
                self.bus.status.emit({"type": "motion_error", "message": str(error)[:180]})

        threading.Thread(target=work, name="MeoDeskStudioControl", daemon=True).start()

    def refresh_device_state(self) -> None:
        if self.preview_mode:
            return
        if self._state_request_pending or not self.settings.host:
            return
        self._state_request_pending = True
        host = self.settings.host

        def read():
            try:
                data = json_request(host, "/api/status", timeout=1.0)
                self.bus.status.emit({"type": "device_state", "data": data, "source": "http", "host": host})
            except (OSError, ValueError, TimeoutError):
                pass
            finally:
                self._state_request_pending = False

        threading.Thread(target=read, name="MeoDeskStateRead", daemon=True).start()

    def stream_disconnected(self) -> None:
        self._stream_seen_at = 0.0
        if self.worker:
            self.worker.stream_active = False
        self.sync_label.setText("HTTP · đang kết nối lại")

    def maintain_state_link(self) -> None:
        if not self.settings.host:
            return
        if self._stream_host != self.settings.host:
            self._stream_host = self.settings.host
            self._stream_supported = False
            self.state_cursor = StateCursor()
            self._stream_seen_at = 0.0
            self.state_socket.abort()
        fresh = self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5
        if self.worker:
            self.worker.stream_active = bool(fresh)
        if not fresh:
            self.sync_label.setText("HTTP · đang đồng bộ")
            self.refresh_device_state()
            if self.state_socket.state() == QAbstractSocket.SocketState.ConnectedState:
                self.state_socket.abort()
            if self._stream_supported and self.state_socket.state() == QAbstractSocket.SocketState.UnconnectedState:
                self.state_socket.open(QUrl(f"ws://{self.settings.host}:81/"))

    def receive_socket_state(self, message: str) -> None:
        if len(message) > 32768:
            return
        try:
            data = json.loads(message)
        except ValueError:
            return
        if not isinstance(data, dict) or data.get("type") != "state":
            return
        self.update_from_worker({"type": "device_state", "data": data, "source": "socket"})

    def tick_focus_display(self) -> None:
        seconds = self.focus_display.seconds()
        if seconds is not None:
            self.focus_clock.setText(f"{seconds // 60:02d}:{seconds % 60:02d}")

    def check_catpack_status(self) -> None:
        if self.preview_mode:
            self.pack_hint.setText("DEMO · không kết nối thiết bị")
            return
        host = self.host_edit.text().strip()
        self.pack_check.setEnabled(False)
        self.pack_info.setText("Đang kiểm tra PSRAM và bộ mèo trên ESP32…")

        def work():
            try:
                data = catpack_client.read_status(host)
                self.bus.catpack.emit({"type": "status", "data": data})
            except Exception as error:
                self.bus.catpack.emit({"type": "error", "message": str(error)[:180]})

        threading.Thread(target=work, name="MeoDeskCatPackProbe", daemon=True).start()

    def start_catpack_upload(self) -> None:
        if self.preview_mode:
            self.pack_hint.setText("DEMO · không cài bộ mèo trong bản xem trước")
            return
        host = self.host_edit.text().strip()
        key = self.key_edit.text().strip()
        pack = self.pack_path.text().strip()
        if not host or not key:
            self.pack_hint.setText("Hãy nhập IP và pairing key ở tab Thiết bị.")
            return
        try:
            size, sha = catpack_client.validate_pack(pack)
        except (OSError, ValueError) as error:
            self.pack_hint.setText(str(error))
            return
        answer = QMessageBox.question(
            self, "Thay bộ mèo qua Wi-Fi",
            f"Cài {Path(pack).name} lên {host}?\n\n16 tâm trạng × {size // catpack_client.PACK_BYTES} frame / {size // 1024} KiB\n"
            f"SHA-256: {sha[:16]}…\n\nESP32 sẽ khởi động lại một lần. Giữ nguồn ổn định.",
            QMessageBox.StandardButton.Yes | QMessageBox.StandardButton.No,
            QMessageBox.StandardButton.No,
        )
        if answer != QMessageBox.StandardButton.Yes:
            return
        self.pack_start.setEnabled(False)
        self.pack_check.setEnabled(False)
        self.ota_start.setEnabled(False)
        self.pack_progress.setValue(0)
        self.pack_hint.setText("Đang truyền bộ mèo vào vùng Flash dự phòng…")

        def work():
            last = -1

            def report(sent: int, total: int) -> None:
                nonlocal last
                percent = min(99, int(100 * sent / max(1, total)))
                if percent != last:
                    last = percent
                    self.bus.catpack.emit({"type": "progress", "percent": percent})

            try:
                result = catpack_client.upload_pack(host, key, pack, report)
            except Exception as error:
                self.bus.catpack.emit({"type": "upload_error", "message": str(error)[:200]})
                return
            self.bus.catpack.emit({"type": "uploaded"})
            try:
                verified = catpack_client.wait_for_pack(host, result["expected_sha256"],
                    callback=lambda message: self.bus.catpack.emit({"type": "verifying", "message": message}))
            except Exception as error:
                self.bus.catpack.emit({"type": "verify_error", "message": str(error)[:200]})
                return
            self.bus.catpack.emit({"type": "confirmed", "data": verified})

        threading.Thread(target=work, name="MeoDeskCatPackUpload", daemon=True).start()

    def update_catpack(self, event: dict) -> None:
        kind = event.get("type")
        if kind in ("status", "confirmed"):
            data = event["data"]
            source = "MÈO TÙY CHỈNH / PSRAM" if data.get("source") == "custom" else "MÈO GỐC / FLASH"
            ready = "SẴN SÀNG" if data.get("ready") else "CHƯA SẴN SÀNG (KIỂM TRA PSRAM)"
            self.pack_info.setText(f"{source} · {ready}\n"
                                   f"Bank {data.get('active_bank', '—')} · Generation {data.get('generation', 0)}"
                                   f" · {data.get('frames_per_pose', 1)} frame/trạng thái")
            self.pack_check.setEnabled(True)
            if kind == "confirmed":
                self.pack_progress.setValue(100)
                self.pack_hint.setText("✓ THÀNH CÔNG: bộ mèo đã nạp vào PSRAM, "
                                       f"{data.get('frames_per_pose', 1)} khung/trạng thái.")
                self.pack_start.setEnabled(True)
                self.ota_start.setEnabled(True)
        elif kind == "progress":
            self.pack_progress.setValue(event["percent"])
            self.pack_hint.setText(f"Đang cập nhật ảnh mèo… {event['percent']}%")
        elif kind == "uploaded":
            self.pack_progress.setValue(100)
            self.pack_hint.setText("Đã xác minh SHA-256. Chờ ESP32 khởi động để nạp ảnh vào PSRAM…")
        elif kind == "verifying":
            self.pack_hint.setText(event["message"])
        elif kind in ("error", "upload_error", "verify_error"):
            prefix = "Đã gửi xong nhưng chưa xác nhận được pack mới: " if kind == "verify_error" else ""
            self.pack_hint.setText(prefix + event.get("message", "Cat Pack failed"))
            self.pack_check.setEnabled(True)
            self.pack_start.setEnabled(True)
            self.ota_start.setEnabled(True)

    def make_tray(self) -> None:
        self.tray = QSystemTrayIcon(cat_icon(), self)
        menu = QMenu()
        show = QAction("Mở MèoDesk Companion", self)
        show.triggered.connect(self.show_window)
        menu.addAction(show)
        toggle = QAction("Bắt đầu / Tạm dừng", self)
        toggle.triggered.connect(self.toggle_agent)
        menu.addAction(toggle)
        menu.addSeparator()
        quit_action = QAction("Thoát ứng dụng", self)
        quit_action.triggered.connect(self.quit_app)
        menu.addAction(quit_action)
        self.tray.setContextMenu(menu)
        self.tray.setToolTip("MèoDesk Companion 1.7")
        self.tray.activated.connect(lambda reason: self.show_window()
                            if reason == QSystemTrayIcon.ActivationReason.DoubleClick else None)
        if QSystemTrayIcon.isSystemTrayAvailable():
            self.tray.show()

    def set_tab(self, index: int) -> None:
        self.stack.setCurrentIndex(index)
        for i, nav in enumerate(self.nav):
            nav.setProperty("chosen", i == index)
            nav.style().unpolish(nav)
            nav.style().polish(nav)

    def update_from_worker(self, event: dict) -> None:
        kind = event.get("type")
        if kind == "device_state":
            if event.get("host", self.settings.host) != self.settings.host:
                return
            if event.get("source") == "http" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            data = event["data"]
            old_boot = self.state_cursor.boot
            if not self.state_cursor.accept(data):
                return
            if self.worker and old_boot is not None and old_boot != self.state_cursor.boot:
                self.worker.reset_device_session()
            self._stream_supported = "state_websocket_v1" in data.get("capabilities", [])
            self.lab_toggle.setEnabled("performance_lab_v1" in data.get("capabilities", []) or self.preview_mode)
            self.lab_toggle.setChecked(bool(data.get("lab_enabled")))
            self.lab_toggle.setText("Tắt đo Performance Lab" if data.get("lab_enabled") else "Bật đo Performance Lab")
            if data.get("lab_valid"):
                fmt = lambda key: f"{data[key]:.2f}" if isinstance(data.get(key), (int, float)) else "N/A"
                self.lab_live.setText(f"CPU strip/frame {fmt('lab_cpu_frame_ms')} ms · DMA wait/strip {fmt('lab_dma_wait_ms')} ms"
                                      f" · Card {fmt('lab_card_wall_ms')} ms · Bulk {fmt('lab_payload_bytes_s')} B/s")
            else:
                self.lab_live.setText("Performance Lab · đang chờ cửa sổ đo" if data.get("lab_enabled") else "Performance Lab · chưa bật đo")
            if event.get("source") == "socket":
                self._stream_seen_at = time.monotonic()
                self.sync_label.setText("WebSocket · đồng bộ trực tiếp")
            self.studio_scene.setCurrentIndex(self.studio_scene.findData(data.get("page")))
            for action, field in (("theme", "theme"), ("fps", "fps_profile")):
                for value, entry in enumerate(self.studio_buttons[action]):
                    entry.setChecked(data.get(field) == value)
            self.studio_toggles["motion"].setChecked(bool(data.get("motion")))
            self.studio_toggles["auto"].setChecked(bool(data.get("auto_pages")))
            for combo, page in zip(self.favorite_scenes, data.get("scene_slots", [])):
                combo.setCurrentIndex(combo.findData(page))
            if data.get("pc_connected") and data.get("cpu_percent") is not None:
                self.update_from_worker({"type": "telemetry", "data": data, "source": "state",
                                         "host": self.settings.host, "emotion": data.get("emotion", "healthy")})
            self.update_from_worker({"type": "focus", "data": data, "source": "state"})
            self.update_from_worker({"type": "motion_status", "data": data, "source": "state"})
            if isinstance(data.get("media"), dict):
                self.update_from_worker({"type": "media", "data": data["media"], "source": "state"})
        elif kind == "telemetry":
            if event.get("source") != "state" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            data = event["data"]
            self.cpu.setText(f"{data['cpu_percent']:.0f}%")
            self.gpu.setText("N/A" if data.get("gpu_percent") is None else f"{data['gpu_percent']:.0f}%")
            self.ram.setText(f"{data['ram_percent']:.0f}%")
            cpu = data.get("cpu_temp_c")
            gpu = data.get("gpu_temp_c")
            self.temps.setText(f"{cpu:.0f}° / {gpu:.0f}°" if cpu is not None and gpu is not None
                               else f"{gpu:.0f}° GPU" if gpu is not None else "N/A")
            self.state.setText("● ĐANG KẾT NỐI")
            self.set_cat_mood(event.get("emotion", "healthy"))
            mode = "DEMO / mô phỏng" if data.get("demo") else "Dữ liệu thực"
            self.hint.setText(f"ESP32: {event['host']}    ·    Mèo: {event['emotion']}    ·    {mode}")
            up = data.get("net_up_kbps")
            down = data.get("net_down_kbps")
            used = data.get("vram_used_gb")
            total = data.get("vram_total_gb")
            value = lambda n: f"{n:.1f}" if n is not None else "N/A"
            self.extra_metrics.setText(f"NET ↓ {value(down)} KB/s  ↑ {value(up)} KB/s"
                                       f"   ·   VRAM {value(used)} / {value(total)} GB")
            self.tray.setToolTip("MèoDesk · Trực tuyến")
        elif kind == "media":
            if event.get("source") != "state" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            media = event["data"]
            if media.get("available"):
                self.song.setText(media.get("title") or "Không có tên bài")
                self.artist.setText(media.get("artist") or "Không rõ nghệ sĩ")
                length = media.get("duration_ms", 0)
                progress = media.get("position_ms", 0)
                self.progress.setValue(round(100 * progress / length) if length > 0 else 0)
                self.music_state.setText(("▶ ĐANG PHÁT" if media.get("playing") else "Ⅱ TẠM DỪNG") +
                                         f"   ·   {progress // 60000}:{progress // 1000 % 60:02d}" +
                                         (f" / {length // 60000}:{length // 1000 % 60:02d}" if length > 0 else ""))
            else:
                self.song.setText("Chưa có bản nhạc")
                self.artist.setText("Trình phát chưa cung cấp Windows Media Session")
                self.progress.setValue(0)
                self.music_state.setText("KHÔNG CÓ DỮ LIỆU / N/A")
        elif kind == "artwork":
            pixels = event.get("pixels")
            if isinstance(pixels, bytes) and len(pixels) == 8192:
                # Copy: original bytes belong to the worker's signal payload.
                image = QImage(pixels, 64, 64, 64 * 2, QImage.Format.Format_RGB16).copy()
                self.music_cover.setPixmap(QPixmap.fromImage(image).scaled(
                    114, 114, Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation))
            else:
                self.music_cover.clear()
                self.music_cover.setText("ALBUM\nART N/A")
        elif kind == "focus":
            if event.get("source") != "state" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            focus = event["data"]
            self.focus_display.update(focus)
            seconds = max(0, int(focus.get("focus_remaining_s", 1500)))
            self.focus_clock.setText(f"{seconds // 60:02d}:{seconds % 60:02d}")
            work = focus.get("focus_phase", "work") == "work"
            running = bool(focus.get("focus_running"))
            self.focus_phase.setText(("ĐANG TẬP TRUNG" if work else "ĐANG NGHỈ")
                                     if running else "TẠM DỪNG / SẴN SÀNG")
            self.focus_sessions.setText(f"{focus.get('focus_sessions', 0)} phiên đã hoàn thành")
            total = 1500 if work else 300
            self.focus_progress.setValue(round(100 * (total - min(total, seconds)) / total))
            self.focus_toggle.setText("Ⅱ  TẠM DỪNG" if running else "▶  TIẾP TỤC")
            self.focus_hint.setText("Đồng bộ từ ESP32 · " + ("FOCUS" if work else "BREAK"))
        elif kind == "focus_error":
            self.focus_hint.setText("Focus: " + event.get("message", "Không gửi được lệnh"))
        elif kind == "motion_status":
            if event.get("source") != "state" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            data = event["data"]
            mode = str(data.get("motion_state") or "chưa có")
            style = data.get("motion_style")
            guard = "LITE / FPS GUARD" if data.get("motion_budget_limited") else "NORMAL"
            frames = data.get("cat_motion_frames", 1)
            self.motion_live.setText(f"LIVING MOTION / {mode.upper()} · {guard} · {frames} FRAME/TRẠNG THÁI")
            self.pack_motion_status.setText(f"Trạng thái: {mode.upper()} · {guard} · "
                                            f"khung {data.get('motion_cel', 0)} / {frames}")
            for index, choice in enumerate(self.motion_style_buttons):
                choice.setChecked(style == index)
        elif kind == "motion_error":
            self.pack_motion_status.setText(event.get("message", "Không điều khiển được chuyển động"))
        else:
            message = event.get("message", "Đang chờ thiết bị")
            self.hint.setText(message)
            if kind == "error":
                self.state.setText("○ CHƯA KẾT NỐI")
                self.set_cat_mood("rescue")
                self.tray.setToolTip("MèoDesk · Ngoại tuyến")

    def set_cat_mood(self, mood: str) -> None:
        if mood == self._last_cat_emotion:
            return
        self._last_cat_emotion = mood
        names = {"stressed": "storage", "low_battery": "shutdown", "hot": "health",
                 "power": "power", "music": "music", "sleep": "sleep", "rescue": "rescue",
                 "paused": "paused", "hotspot": "hotspot", "healthy": "healthy", "demo": "music"}
        asset = Path(__file__).resolve().parent / "assets" / (names.get(mood, "healthy") + ".png")
        pixmap = QPixmap(str(asset))
        if not pixmap.isNull():
            self.cat_picture.setPixmap(pixmap.scaled(124, 124, Qt.AspectRatioMode.KeepAspectRatio,
                                                      Qt.TransformationMode.SmoothTransformation))

    def save_device(self) -> None:
        if self.preview_mode:
            self.hint.setText("DEMO · không lưu kết nối hoặc gửi dữ liệu đến thiết bị")
            return
        try:
            host = discovery.normalize_host(self.host_edit.text())
        except ValueError as exc:
            self.hint.setText(str(exc))
            self.set_tab(0)
            return
        self.settings.host = host
        self.key = self.key_edit.text().strip()
        write_settings(self.settings)
        if self.key and not secret_store.save(host, self.key):
            self.hint.setText("Không lưu được key vào Windows vault; key chỉ giữ trong phiên này.")
        else:
            self.hint.setText("Đã lưu thiết bị. Không lưu mật khẩu Wi-Fi.")
        self.start_agent()
        self.set_tab(0)

    def change_focus(self, action: str) -> None:
        if not self.settings.host or not self.key:
            self.set_tab(1)
            self.hint.setText("Cần kết nối ESP32 và nhập pairing key trước.")
            return
        self.focus_hint.setText("Đang gửi lệnh đến ESP32...")

        def task() -> None:
            try:
                response = json_request(
                    self.settings.host, "/api/v1/focus", self.key, {"action": action}, timeout=1.5)
                if not response.get("ok"):
                    raise ValueError("Thiết bị chưa hỗ trợ giao thức Focus (v0.9+)")
                self.bus.status.emit({"type": "focus", "data": response})
            except Exception as error:
                self.bus.status.emit({"type": "focus_error", "message": str(error)[:75]})

        threading.Thread(target=task, name="MeoDeskFocusControl", daemon=True).start()

    def start_agent(self) -> None:
        if self.preview_mode:
            return
        if not self.settings.host or not self.key:
            self.set_tab(1)
            self.hint.setText("Nhập IP thiết bị và pairing key ở tab Thiết bị.")
            return
        if self.worker is not None:
            self.worker.stop()
        self.worker = CompanionWorker(self.settings, self.key, self.bus.status.emit)
        self.worker.start()
        self.agent_button.setText("Ⅱ  TẠM DỪNG TRUYỀN DỮ LIỆU")

    def toggle_agent(self) -> None:
        if self.worker is None:
            self.start_agent()
        elif self.worker.running:
            self.worker.pause()
            self.agent_button.setText("▶  TIẾP TỤC TRUYỀN DỮ LIỆU")
        else:
            self.worker.start()
            self.agent_button.setText("Ⅱ  TẠM DỪNG TRUYỀN DỮ LIỆU")

    def scan_devices(self) -> None:
        if self.preview_mode:
            return
        self.scan_btn.setEnabled(False)
        self.scan_btn.setText("ĐANG QUÉT LAN...")
        self.found.clear()

        def task():
            try:
                devices = discovery.discover(self.host_edit.text(), lan_scan=True)
            except Exception:
                devices = []
            self.bus.discovery.emit(devices)

        threading.Thread(target=task, name="MeoDeskDiscovery", daemon=True).start()

    def update_discovery(self, devices: list[dict]) -> None:
        self.scan_btn.setEnabled(True)
        self.scan_btn.setText("⌕  QUÉT LẠI LAN")
        for item in devices:
            host = item["host"]
            from PySide6.QtWidgets import QListWidgetItem
            row = QListWidgetItem(f"●  {host}  ·  Firmware {item['version']}   ·   {item['name']}")
            row.setData(Qt.ItemDataRole.UserRole, host)
            self.found.addItem(row)
        if not devices:
            self.found.addItem("Không tìm được. Có thể nhập IP thủ công phía trên.")

    def select_device(self) -> None:
        row = self.found.currentItem()
        if row is None:
            return
        host = row.data(Qt.ItemDataRole.UserRole)
        if host:
            self.host_edit.setText(host)
            self.key_edit.setText(secret_store.load(host) or self.key_edit.text())

    def save_preferences(self) -> None:
        if self.preview_mode:
            return
        self.settings.media = self.media_check.isChecked()
        self.settings.start_minimized = self.min_check.isChecked()
        write_settings(self.settings)
        self.hint.setText("Đã lưu cài đặt. Khởi động lại stream để áp dụng.")

    def open_web(self) -> None:
        host = self.settings.host
        if host:
            webbrowser.open(f"http://{host}/")
        else:
            self.set_tab(1)

    def show_window(self) -> None:
        self.showNormal()
        self.activateWindow()

    def closeEvent(self, event) -> None:
        if QSystemTrayIcon.isSystemTrayAvailable():
            event.ignore()
            self.hide()
        else:
            self.quit_app()

    def quit_app(self) -> None:
        self.state_timer.stop()
        self.display_timer.stop()
        self.state_socket.abort()
        if self.worker is not None:
            self.worker.stop()
        self.tray.hide()
        QApplication.instance().quit()


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("MèoDesk Companion")
    app.setQuitOnLastWindowClosed(False)
    window = MainWindow(preview="--preview" in sys.argv)
    window.show()
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
