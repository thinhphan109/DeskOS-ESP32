#!/usr/bin/env python3
"""mimi.desk Companion v1.7 — Windows dashboard and Living Motion Engine.

Install GUI requirements then double-click the packaged app. The original
pc_agent.py and tray_agent.py remain available as independent fallbacks.
"""
from __future__ import annotations

import sys
import json
import time
import threading
import webbrowser
import sqlite3
import math
import subprocess
from urllib.error import HTTPError
from pathlib import Path

from PySide6.QtCore import QObject, Qt, Signal, QTimer, QUrl, QSignalBlocker, QSize
from PySide6.QtNetwork import QAbstractSocket, QNetworkProxy
from PySide6.QtWebSockets import QWebSocket
from PySide6.QtGui import QAction, QColor, QFont, QIcon, QImage, QPainter, QPainterPath, QPen, QPixmap
from PySide6.QtWidgets import (
    QApplication, QCheckBox, QFrame, QGridLayout, QHBoxLayout, QLabel,
    QFileDialog, QLineEdit, QListWidget, QMainWindow, QMenu, QMessageBox, QProgressBar, QPushButton,
    QScrollArea, QSizePolicy, QSystemTrayIcon, QVBoxLayout, QWidget,
)

import discovery
import ota_client
import catpack_client
import device_config
from scene_editor import SceneEditor
from scene_layout import validate_layout
from telemetry_chart import TelemetryChart
from focus_journal import FocusJournal
import startup
from desk_notification import notification_payload
from rule_editor import RuleEditor
from automation_rules import validate_rules
import secret_store
from companion_core import CompanionWorker, Settings, json_request, read_settings, write_settings, user_config_path
from state_sync import StateCursor, FocusDisplay, MediaDisplay
from app_version import VERSION


from ui_theme import QSS, ContentStack, Disclosure, nav_icon
from ui_controls import ScrollSafeComboBox, ScrollSafeSpinBox
from connection_state import RetryBackoff
from media_monitor import MediaMonitor
from single_instance import SingleInstance
from update_panel import UpdatePanel
import update_client



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
    frame.setSizePolicy(QSizePolicy.Policy.Preferred,QSizePolicy.Policy.Maximum)
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
    updates = Signal(object)


class MainWindow(QMainWindow):
    def __init__(self, preview: bool = False, *, start_services: bool = True) -> None:
        super().__init__()
        self.preview_mode = preview
        self._closing = False
        self._stop_updates = threading.Event()
        self._upload_active = False
        self._quit_after_update = False
        self._update_journal = None
        self._verification_active = False
        self._agent_before_update = False
        self._studio_cas = False
        self._scene_base = None
        self._rules_base = None
        self.settings = Settings() if preview else read_settings()
        self.key = secret_store.load(self.settings.host) if self.settings.host else ""
        self.worker: CompanionWorker | None = None
        self.media_monitor = None
        self._media_generation = 0
        self._start_services = start_services
        self._state_retry = RetryBackoff()
        self._socket_retry = RetryBackoff()
        self._rediscovery_pending = False
        self._pairing_rejected = False
        self._next_rediscovery = time.monotonic() + 15
        self._last_cat_emotion = ""
        self.state_cursor = StateCursor()
        self.focus_display = FocusDisplay()
        self.media_display = MediaDisplay()
        self.focus_journal = None
        self._state_request_pending = False
        self._stream_seen_at = 0.0
        self._stream_supported = False
        self._stream_host = ""
        self.bus = Bus()
        self.bus.status.connect(self.update_from_worker)
        self.bus.discovery.connect(self.update_discovery)
        self.bus.ota.connect(self.update_ota)
        self.bus.catpack.connect(self.update_catpack)
        self.bus.updates.connect(self.receive_update_reply)
        self.setWindowTitle("mimi.desk · " + VERSION)
        self.setWindowIcon(cat_icon())
        self.resize(1120, 760)
        self.setMinimumSize(850, 560)
        self.setStyleSheet(QSS)

        root = QWidget()
        root.setObjectName("root")
        self.setCentralWidget(root)
        frame = QHBoxLayout(root)
        frame.setContentsMargins(0, 0, 0, 0)
        frame.setSpacing(0)
        side = QFrame()
        side.setObjectName("sidebar")
        side.setFixedWidth(178)
        sidebox = QVBoxLayout(side)
        sidebox.setContentsMargins(12, 26, 12, 20)
        sidebox.setSpacing(6)
        sidebox.addWidget(text("mimi.desk", "brand"))
        sidebox.addWidget(text("Không gian của bạn", "sideSmall"))
        sidebox.addSpacing(20)
        self.nav: list[QPushButton] = []
        names=("Tổng quan","Thiết bị","Âm nhạc","Tập trung","Cài đặt","Cập nhật","Studio")
        icons=('home','device','music','focus','settings','update','studio')
        for index,(caption,icon) in enumerate(zip(names,icons)):
            node=QPushButton(caption); node.setObjectName("nav")
            node.setIcon(nav_icon(icon)); node.setIconSize(QSize(19,19))
            node.setCursor(Qt.CursorShape.PointingHandCursor)
            node.clicked.connect(lambda checked=False,i=index:self.set_tab(i))
            self.nav.append(node)
        for index in (0,2,3,6,1): sidebox.addWidget(self.nav[index])
        self.nav[5].setVisible(False)
        sidebox.addStretch()
        sidebox.addWidget(self.nav[4])
        sidebox.addSpacing(12)
        sidebox.addWidget(text("Một góc bàn dễ chịu hơn.", "sideSmall"))
        frame.addWidget(side)

        area = QScrollArea()
        area.setWidgetResizable(True)
        area.setFrameShape(QFrame.Shape.NoFrame)
        page = QWidget()
        page.setObjectName("content")
        area.setWidget(page)
        self.stack = ContentStack()
        self.content_scroll=area
        self.device_seen_at=0.0
        pagebox = QVBoxLayout(page)
        pagebox.setContentsMargins(26, 22, 26, 24)
        pagebox.setSpacing(22)
        top=QHBoxLayout()
        self.page_trail=text("mimi.desk / Tổng quan", "pageTrail")
        self.connection_badge=text("Chưa có thiết bị", "connectionBadge")
        top.addWidget(self.page_trail); top.addStretch(); top.addWidget(self.connection_badge)
        if preview:
            self.open_live_button = button("Mở chế độ thật", primary=True)
            self.open_live_button.setToolTip("Dùng cấu hình đã lưu. Thông tin nhập trong DEMO không được lưu.")
            self.open_live_button.clicked.connect(self.open_live_app)
            top.addWidget(self.open_live_button)
        pagebox.addLayout(top)
        if preview:
            self.preview_notice = text(
                "Đang xem trước với dữ liệu mẫu. DEMO không đọc Spotify và không kết nối thiết bị. "
                "Mở chế độ thật để dùng nhạc trên PC; cấu hình nhập trong DEMO không được lưu.",
                "subtitle", wrap=True)
            pagebox.addWidget(self.preview_notice)
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
            self.setWindowTitle("mimi.desk · DEMO / Xem trước")
            self._preview_state = {"device": "MEO DESK / MOCK", "version": VERSION + "-mock",
                "page": 1, "theme": 1, "fps_profile": 1, "motion_style": 1,
                "motion_state": "music", "cat_motion_frames": 3, "motion": True,
                "auto_pages": False, "scene_slots": [12, 0, 5, 2], "pc_connected": True,
                "demo": True, "cpu_percent": 25, "gpu_percent": 14, "ram_percent": 60,
                "focus_remaining_s": 1500, "focus_phase": "work", "focus_running": False,
                "focus_sessions": 0, "emotion": "healthy"}
            self.update_from_worker({"type": "device_state", "data": self._preview_state})
            self.hint.setText("DEMO · dữ liệu mô phỏng · không kết nối hoặc cập nhật thiết bị")
            self.sync_label.setText("DEMO · xem trước trên máy")
            self.update_from_worker({"type":"media", "data":{"available":True,"playing":False,
                "title":"Bản nhạc mẫu · Tiếng Việt", "artist":"Dữ liệu mô phỏng · không phải Spotify",
                "position_ms":92000,"duration_ms":227000}})
            for age in range(60,-1,-1):
                self.pc_chart.history.add({"demo":True,"cpu_percent":25+7*math.sin(age/8),"gpu_percent":14+5*math.sin(age/10),"ram_percent":60+2*math.sin(age/16)},"preview",now=time.monotonic()-age)
            self.preview_graph_timer=QTimer(self)
            self.preview_graph_timer.timeout.connect(lambda:self.pc_chart.add({"demo":True,"cpu_percent":25,"gpu_percent":14,"ram_percent":60},"preview"))
            self.preview_graph_timer.start(1000)
            self.set_tab(0)
        if not preview and start_services:
            self.configure_media_monitor()
            self._update_journal=update_client.UpdateJournal(user_config_path().with_name("update-session.json"))
            previous=self._update_journal.read()
            if previous and previous.get("phase") in ("uploading","uploaded","unconfirmed"):
                self.ota_hint.setText("Có lượt cập nhật trước đang chờ xác nhận. App sẽ kiểm tra lại, không gửi lại firmware.")
                QTimer.singleShot(500,self.resume_update_confirmation)
        if self.settings.host:
            self.host_edit.setText(self.settings.host)
            self.update_from_worker({"type": "state", "message": "Đang khởi tạo kết nối tự động..."})
            if self.key and start_services:
                QTimer.singleShot(250, self.start_agent)
            else:
                self.update_from_worker({"type": "state", "message": "Nhập pairing key ở tab Thiết bị để bắt đầu"})
        if "--show" not in sys.argv and (self.settings.start_minimized or "--minimized" in sys.argv):
            QTimer.singleShot(0, self.hide)

    def open_live_app(self) -> None:
        if not self.preview_mode or not self.open_live_button.isEnabled():
            return
        command = [sys.executable]
        if not getattr(sys, "frozen", False):
            command.append(str(Path(__file__).resolve()))
        command.append("--show")
        try:
            subprocess.Popen(command, cwd=str(Path(sys.executable).parent)
                             if getattr(sys, "frozen", False) else str(Path(__file__).resolve().parent))
        except OSError:
            self.preview_notice.setText(
                "Chưa mở được chế độ thật. Hãy chạy lại mimi.desk không có tham số --preview.")
            return
        self.open_live_button.setEnabled(False)
        self.open_live_button.setText("Đã mở chế độ thật")
        self.preview_notice.setText(
            "Đã yêu cầu mở mimi.desk ở cửa sổ riêng. Trong cửa sổ mới, vào Thiết bị để kết nối. "
            "Cửa sổ này vẫn là DEMO và không đọc Spotify.")

    def configure_media_monitor(self) -> None:
        desired = not self.preview_mode and self._start_services and self.settings.media
        if desired == bool(self.media_monitor):
            return
        self._media_generation += 1
        if self.media_monitor:
            self.media_monitor.stop()
            self.media_monitor = None
        if desired:
            generation = self._media_generation
            self.media_monitor = MediaMonitor(lambda event: self.bus.status.emit(
                dict(event, media_generation=generation)))
            self.media_monitor.start()
        if self.worker:
            self.worker.media_source = self.media_monitor.snapshot if self.media_monitor else None

    def heading(self, root: QVBoxLayout, eyebrow: str, heading: str, sub: str) -> None:
        root.addWidget(text(heading, "title"))
        root.addWidget(text(sub, "subtitle", wrap=True))
        root.addSpacing(8)

    def make_overview(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "01 / LIVE STUDIO", "Chào bạn, chào ngày mới.",
                     "DEMO · dữ liệu mô phỏng, không kết nối thiết bị." if self.preview_mode else
                     "Theo dõi máy tính, nhạc và nhịp tập trung từ một nơi.")
        panel = QHBoxLayout()
        panel.setSpacing(14)
        primary, p = card("mintCard")
        primary.setMaximumHeight(220)
        p.addWidget(text("CPU", "eyebrow"))
        self.cpu = text("—%", "bigNumber")
        cpu_hero = QHBoxLayout()
        cpu_hero.addWidget(self.cpu, 1)
        self.cat_picture = QLabel()
        self.cat_picture.setFixedSize(126, 126)
        self.cat_picture.setAlignment(Qt.AlignmentFlag.AlignCenter)
        cpu_hero.addWidget(self.cat_picture, 0)
        p.addLayout(cpu_hero)
        p.addWidget(text("Trạng thái máy tính của bạn", "smallLabel"))
        p.addStretch()
        self.state = text("Chưa kết nối", "cardTitle", wrap=True)
        p.addWidget(self.state)
        self.set_cat_mood("healthy")
        panel.addWidget(primary, 3)
        dark, d = card("darkCard")
        dark.setMaximumHeight(220)
        for title in ("GPU", "Bộ nhớ RAM", "Nhiệt độ CPU / GPU"):
            d.addWidget(text(title, "darkMeta"))
            value = text("—", "darkNumber")
            d.addWidget(value)
            setattr(self, {"GPU": "gpu", "Bộ nhớ RAM": "ram", "Nhiệt độ CPU / GPU": "temps"}[title], value)
        panel.addWidget(dark, 2)
        box.addLayout(panel)
        self.pc_chart = TelemetryChart()
        box.addWidget(self.pc_chart)
        bottom, b = card()
        row = QHBoxLayout()
        self.agent_button = button("Bắt đầu kết nối", True)
        self.agent_button.clicked.connect(self.toggle_agent)
        row.addWidget(self.agent_button)
        link = button("Mở giao diện Web")
        link.clicked.connect(self.open_web)
        row.addWidget(link)
        b.addLayout(row)
        self.hint = text("Kết nối thiết bị để đồng bộ PC, nhạc và cảnh của bạn.", "subtitle", wrap=True)
        b.addWidget(self.hint)
        box.addWidget(bottom)
        diagnostics=Disclosure("Chẩn đoán kết nối và hiệu năng")
        self.extra_metrics = text("NET ↓ — KB/s  ↑ — KB/s   ·   GPU VRAM — / — GB", "smallLabel", wrap=True)
        diagnostics.content.addWidget(self.extra_metrics)
        self.motion_live = text("LIVING MOTION / CHỜ KẾT NỐI ESP32", "smallLabel", wrap=True)
        diagnostics.content.addWidget(self.motion_live)
        self.sync_label = text("HTTP · chờ kết nối", "smallLabel")
        diagnostics.content.addWidget(self.sync_label)
        self.lab_live = text("Performance Lab · chưa có dữ liệu đo", "smallLabel", wrap=True)
        diagnostics.content.addWidget(self.lab_live)
        self.lab_toggle = button("Bật đo Performance Lab")
        self.lab_toggle.setCheckable(True)
        self.lab_toggle.setEnabled(False)
        self.lab_toggle.clicked.connect(lambda checked: self.send_device_control("perf_trace", int(checked)))
        diagnostics.content.addWidget(self.lab_toggle)
        box.addWidget(diagnostics)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_devices(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "02 / DEVICE LINK", "Thiết bị của bạn",
                     "Bật nguồn thiết bị và kết nối cùng mạng Wi-Fi với máy tính.")
        panel, p = card()
        p.addWidget(text("Địa chỉ thiết bị", "eyebrow"))
        self.host_edit = QLineEdit(self.settings.host)
        self.host_edit.setPlaceholderText("Ví dụ: 192.168.0.107 hoặc meodesk.local")
        p.addWidget(self.host_edit)
        p.addWidget(text("Mã ghép nối", "eyebrow"))
        self.key_edit = QLineEdit(self.key)
        self.key_edit.setEchoMode(QLineEdit.EchoMode.Password)
        self.key_edit.setPlaceholderText("Nhập mã ghép nối của thiết bị")
        p.addWidget(self.key_edit)
        self.setup_hint=text("Tìm thiết bị → nhập mã ghép nối → lưu và kết nối.","subtitle",wrap=True)
        p.addWidget(self.setup_hint)
        setup_key=button("Lấy mã khi dùng Wi-Fi thiết bị")
        setup_key.clicked.connect(self.load_setup_key);p.addWidget(setup_key)
        actions = QHBoxLayout()
        apply_btn = button("Lưu và kết nối", True)
        apply_btn.clicked.connect(self.save_device)
        actions.addWidget(apply_btn)
        self.scan_btn = button("Tìm thiết bị")
        self.scan_btn.clicked.connect(self.scan_devices)
        actions.addWidget(self.scan_btn)
        p.addLayout(actions)
        box.addWidget(panel)
        shortcuts=QHBoxLayout()
        for caption,index in (("Cập nhật firmware",5),("Cài đặt và sao lưu",4)):
            entry=button(caption); entry.clicked.connect(lambda checked=False,i=index:self.set_tab(i)); shortcuts.addWidget(entry)
        box.addLayout(shortcuts)
        scan, s = card()
        s.addWidget(text("Thiết bị tìm thấy", "cardTitle"))
        self.found = QListWidget()
        self.found.setMinimumHeight(100)
        self.found.setMaximumHeight(150)
        self.found.itemDoubleClicked.connect(lambda _: self.select_device())
        s.addWidget(self.found)
        select_btn = button("Kết nối thiết bị đã chọn")
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
        self.heading(box, "03 / NOW PLAYING", "Nhạc đang phát",
                     "Nhạc mẫu để xem bố cục. Chế độ DEMO không đọc phiên Spotify trên PC."
                     if self.preview_mode else "Spotify và các trình phát hỗ trợ Windows Media Controls.")
        panel, p = card("musicPanel")
        panel.setMinimumHeight(380)
        panel.setSizePolicy(QSizePolicy.Policy.Preferred,QSizePolicy.Policy.Maximum)
        p.setContentsMargins(24, 24, 24, 24)
        p.setSpacing(16)
        music_hero = QHBoxLayout()
        music_hero.setSpacing(24)
        self.music_cover = text("Chưa có\nảnh bìa", "albumCover")
        self.music_cover.setAlignment(Qt.AlignmentFlag.AlignCenter)
        self.music_cover.setFixedSize(220, 220)
        music_hero.addWidget(self.music_cover)
        left = QVBoxLayout()
        left.setSpacing(12)
        left.addStretch()
        self.song = text("Chưa có bản nhạc", "musicSong", wrap=True)
        self.song.setTextFormat(Qt.TextFormat.PlainText)
        self.song.setMinimumWidth(0)
        left.addWidget(self.song)
        self.artist = text("Mở Spotify trên PC để bắt đầu.", "musicArtist", wrap=True)
        self.artist.setTextFormat(Qt.TextFormat.PlainText)
        left.addWidget(self.artist)
        left.addStretch()
        music_hero.addLayout(left, 1)
        p.addLayout(music_hero)
        self.progress = QProgressBar()
        self.progress.setRange(0, 100)
        self.progress.setValue(0)
        self.progress.setTextVisible(False)
        p.addWidget(self.progress)
        self.music_state = text("Chưa có phiên phát nhạc", "musicState", wrap=True)
        p.addWidget(self.music_state)
        box.addWidget(panel)
        box.addWidget(text("Ảnh bìa phụ thuộc trình phát. Điều khiển nhạc từ Web Studio hoặc trình phát trên PC.", "subtitle", wrap=True))
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
        layout.addWidget(text("KẾ HOẠCH FOCUS / ĐỒNG BỘ TỪ ESP32", "eyebrow"))
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
        config, config_box = card()
        fields = QHBoxLayout()
        self.focus_inputs = []
        self.focus_plan_dirty = False
        for caption, maximum, initial in (("Làm việc · phút", 180, 25), ("Nghỉ · phút", 60, 5), ("Mục tiêu phiên", 24, 4)):
            column = QVBoxLayout(); column.addWidget(text(caption, "smallLabel"))
            spin = ScrollSafeSpinBox(); spin.setRange(1, maximum); spin.setValue(initial)
            spin.valueChanged.connect(lambda _: setattr(self, "focus_plan_dirty", True))
            column.addWidget(spin); fields.addLayout(column); self.focus_inputs.append(spin)
        config_box.addLayout(fields)
        save_plan = button("Lưu kế hoạch Focus")
        save_plan.clicked.connect(self.save_focus_plan)
        config_box.addWidget(save_plan)
        self.focus_daily = text("Hôm nay · chờ phiên Focus hoàn tất khi Companion đang kết nối.", "smallLabel", wrap=True)
        config_box.addWidget(self.focus_daily)
        self.reminder_check = QCheckBox("Nhắc nghỉ khi hoàn thành Focus trên thiết bị")
        self.reminder_check.setChecked(self.settings.break_reminders)
        self.reminder_check.toggled.connect(self.set_break_reminders)
        config_box.addWidget(self.reminder_check)
        box.addWidget(config)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_settings(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "05 / PREFERENCES", "Cài đặt",
                     "Các tuỳ chọn cho máy tính, thông báo và cấu hình thiết bị.")
        panel, p = card()
        self.media_check = QCheckBox("Bật Now Playing (Windows Media Controls)")
        self.media_check.setChecked(self.settings.media)
        p.addWidget(self.media_check)
        self.min_check = QCheckBox("Mở thu nhỏ vào khay hệ thống từ lần chạy sau")
        self.min_check.setChecked(self.settings.start_minimized)
        p.addWidget(self.min_check)
        save_btn = button("Lưu tuỳ chọn", True)
        save_btn.clicked.connect(self.save_preferences)
        p.addWidget(save_btn)
        p.addWidget(text("Mỗi 1 giây: CPU/GPU/RAM. Mỗi ~2 giây: media nếu được bật. "
                         "Cần khởi động lại phiên truyền để nhận thay đổi.", "subtitle", wrap=True))
        box.addWidget(panel)
        backup, b = card()
        self.startup_check = QCheckBox("Mở mimi.desk khi đăng nhập Windows")
        self.startup_check.setChecked(False if self.preview_mode else startup.is_enabled())
        self.startup_check.setEnabled(sys.platform == "win32" and not self.preview_mode)
        b.addWidget(self.startup_check)
        startup_apply = button("Áp dụng khởi động cùng Windows")
        startup_apply.setEnabled(sys.platform == "win32" and not self.preview_mode)
        startup_apply.clicked.connect(self.apply_startup)
        b.addWidget(startup_apply)
        self.startup_hint = text("Chỉ thay đổi khi nhấn Áp dụng. Không cần quyền quản trị.", "subtitle", wrap=True)
        b.addWidget(self.startup_hint)
        b.addWidget(text("Thông báo lên mimi.desk", "cardTitle"))
        self.notification_text=QLineEdit("Desk reminder")
        self.notification_text.setMaxLength(60)
        self.notification_text.setPlaceholderText("Nhãn TFT dùng ASCII")
        b.addWidget(self.notification_text)
        self.notification_hidden=QCheckBox("Ẩn nội dung · chỉ hiện New notification")
        self.notification_hidden.setChecked(True);b.addWidget(self.notification_hidden)
        notify_button=button("Gửi thông báo · 8 giây")
        notify_button.clicked.connect(self.send_desk_notification);b.addWidget(notify_button)
        b.addWidget(text("Sao lưu cấu hình thiết bị", "cardTitle"))
        row = QHBoxLayout()
        for caption, operation in (("Xuất cấu hình", self.export_device_config), ("Khôi phục từ file", self.restore_device_config)):
            entry = button(caption); entry.clicked.connect(operation); row.addWidget(entry)
        b.addLayout(row)
        self.profile_hint = text("Sao lưu cảnh, theme, Focus và ngưỡng cảnh báo; không chứa Wi-Fi hoặc pairing key.", "subtitle", wrap=True)
        b.addWidget(self.profile_hint); box.addWidget(backup)
        box.addStretch()
        self.stack.addWidget(widget)

    def make_updater(self) -> None:
        widget = QWidget()
        widget.setObjectName("tabPage")
        box = QVBoxLayout(widget)
        box.setContentsMargins(0, 0, 0, 0)
        self.heading(box, "06 / LOCAL FIRMWARE UPDATER", "Cập nhật firmware",
                     "Chọn cách kiểm tra và cập nhật. Thiết bị giữ chính sách để tiếp tục khi đóng app.")
        server,server_box=card()
        self.server_updates=UpdatePanel(preview=self.preview_mode)
        self.server_updates.requested.connect(self.request_server_update)
        server_box.addWidget(self.server_updates);box.addWidget(server)

        summary=Disclosure("Cập nhật bằng file · công cụ nâng cao")
        self.manual_update=summary;s=summary.content
        self.ota_info = text("Chưa kiểm tra thiết bị. Nhập địa chỉ và pairing key ở tab Thiết bị.", "cardTitle", wrap=True)
        s.addWidget(self.ota_info)
        self.ota_history = text("Chưa đọc lịch sử cập nhật.", "subtitle", wrap=True)
        s.addWidget(self.ota_history)
        history_button = button("Đọc lịch sử cập nhật")
        history_button.clicked.connect(lambda: self.configuration_request("/api/v1/ota/history"))
        s.addWidget(history_button)
        self.ota_check = button("Kiểm tra thiết bị")
        self.ota_check.clicked.connect(self.check_ota_status)
        s.addWidget(self.ota_check)
        box.addWidget(summary)

        update=QWidget();u=QVBoxLayout(update);u.setContentsMargins(0,8,0,0)
        u.addWidget(text("Chọn file firmware", "cardTitle"))
        line = QHBoxLayout()
        self.ota_path = QLineEdit()
        self.ota_path.setPlaceholderText("firmware.bin từ PlatformIO / .pio/build/…")
        line.addWidget(self.ota_path, 1)
        browse = button("Chọn file")
        browse.clicked.connect(self.browse_ota_file)
        line.addWidget(browse)
        u.addLayout(line)
        self.ota_progress = QProgressBar()
        self.ota_progress.setRange(0, 100)
        self.ota_progress.setValue(0)
        u.addWidget(self.ota_progress)
        self.ota_hint = text("Sau khi upload, app sẽ tự kiểm tra phân vùng mới và sức khỏe khởi động (~30–85 giây).", "subtitle", wrap=True)
        u.addWidget(self.ota_hint)
        self.ota_start = button("Cập nhật firmware", True)
        self.ota_start.clicked.connect(self.start_ota_update)
        u.addWidget(self.ota_start)
        s.addWidget(update)
        resume=button("Kiểm tra lượt cập nhật trước")
        resume.clicked.connect(self.resume_update_confirmation);s.addWidget(resume)

        warning = Disclosure("Thông tin kỹ thuật và phục hồi")
        w = warning.content
        w.addWidget(text("• Lần đầu từ v1.2/huge_app phải chuyển phân vùng qua USB và sao lưu NVS.\n"
                         "• Cấp nguồn ổn định trong toàn bộ thời gian cập nhật.\n"
                         "• Đây là OTA nội bộ chưa xác minh chữ ký số; không mở cổng HTTP ra Internet.\n"
                         "• Rollback tự động chỉ có nếu bootloader thực sự hỗ trợ."
                         , "subtitle", wrap=True))
        box.addWidget(warning)
        box.addStretch()
        self.stack.addWidget(widget)

    def browse_ota_file(self) -> None:
        filename, _ = QFileDialog.getOpenFileName(self, "Chọn firmware mimi.desk", "", "ESP32 firmware (*.bin)")
        if filename:
            self.ota_path.setText(filename)

    def request_server_update(self, operation: str, payload: dict) -> None:
        if self.preview_mode or self._closing:
            return
        host,key=self.settings.host,self.key
        if not host or not key:
            self.server_updates.reject("Lưu thiết bị và mã ghép nối trước khi quản lý cập nhật.");return
        def work():
            try:
                data=update_client.request(host,key,operation,payload)
                self.bus.updates.emit({"operation":operation,"host":host,"data":data})
            except (OSError,ValueError) as error:
                self.bus.updates.emit({"operation":operation,"host":host,"error":str(error)[:240]})
        threading.Thread(target=work,name="MimiDeskUpdatePolicy",daemon=True).start()

    def receive_update_reply(self,event: dict) -> None:
        if self._closing:return
        if event.get("operation")=="pairing":
            if event.get("host")!=self.host_edit.text().strip():return
            if event.get("error"):self.setup_hint.setText(event["error"])
            else:
                self.key_edit.setText(event["data"]["pairing_key"])
                self.setup_hint.setText("Đã lấy mã từ thiết bị. Bấm Lưu và kết nối để hoàn tất.")
            return
        if event.get("host")!=self.settings.host:return
        if event.get("error"):self.server_updates.reject(event["error"])
        else:
            self.server_updates.complete(event["data"],event["operation"])
            if not event["data"].get("supported"):self.manual_update.toggle.setChecked(True)

    def load_setup_key(self) -> None:
        if self.preview_mode or self._closing:return
        try:host=discovery.normalize_host(self.host_edit.text().strip() or "192.168.4.1")
        except ValueError as error:self.setup_hint.setText(str(error));return
        self.host_edit.setText(host)
        self.setup_hint.setText("Đang lấy mã. Máy tính cần kết nối Wi-Fi thiết lập của thiết bị.")
        def work():
            try:
                data=json_request(host,"/api/v1/pairing",timeout=3)
                if not isinstance(data.get("pairing_key"),str) or not data["pairing_key"]:raise ValueError("missing pairing key")
                self.bus.updates.emit({"operation":"pairing","host":host,"data":data})
            except (OSError,ValueError):
                self.bus.updates.emit({"operation":"pairing","host":host,
                    "error":"Chưa lấy được mã. Kết nối Wi-Fi thiết lập của thiết bị rồi thử lại, hoặc nhập mã hiện có."})
        threading.Thread(target=work,name="MimiDeskSetup",daemon=True).start()

    def resume_update_confirmation(self) -> None:
        if self.preview_mode or self._closing or not self._update_journal or self._upload_active or self._verification_active:return
        job=self._update_journal.read()
        if not job or job.get("phase")=="confirmed":
            self.ota_hint.setText("Không có lượt cập nhật chưa xác nhận.");return
        if job.get("device_id") and self.settings.device_id and job["device_id"]!=self.settings.device_id:
            self.ota_hint.setText("Lượt cập nhật trước thuộc thiết bị khác. Chọn đúng thiết bị để kiểm tra lại.");return
        self._verification_active=True
        self.ota_hint.setText("Đang kiểm tra lượt cập nhật trước; không gửi lại firmware.")
        threading.Thread(target=lambda:self.verify_update_job(job),name="MimiDeskResumeUpdate",daemon=True).start()

    def verify_update_job(self,job: dict) -> None:
        try:
            verified=ota_client.wait_for_boot(job["host"],job["target_slot"],expected_version=job.get("version") or None,
                expected_device_id=job.get("device_id", ""),previous_boot_id=job.get("previous_boot_id", ""),
                stop_event=self._stop_updates,
                status_callback=lambda message:self.bus.ota.emit({"type":"verifying","message":message}))
            job["phase"]="confirmed";job["host"]=verified.get("host",job["host"])
            if self._update_journal:self._update_journal.write(job)
            self.bus.ota.emit({"type":"confirmed","data":verified})
        except (OSError,ValueError,KeyError) as error:
            job["phase"]="unconfirmed"
            if self._update_journal:self._update_journal.write(job)
            self.bus.ota.emit({"type":"verify_error","message":str(error)[:240]})

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
            info = ota_client.inspect_firmware(image)
            length, sha = info["size"], info["sha256"]
        except (ValueError, OSError) as error:
            self.ota_hint.setText(str(error))
            return
        choice = QMessageBox.question(
            self, "Xác nhận cập nhật firmware",
            f"Cập nhật mimi.desk tại {host}?\n\n{Path(image).name}\n{length / 1048576:.2f} MiB\n"
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
        self._upload_active=True
        self._agent_before_update=bool(self.worker and self.worker.running)
        if self._agent_before_update:self.worker.pause()
        expected_device_id=self.settings.device_id;previous_boot_id=self.state_cursor.boot or ""

        def work():
            last_percent = -1

            def progress(sent: int, size: int) -> None:
                nonlocal last_percent
                percent = min(99, int(100 * sent / max(1, size)))
                if percent != last_percent:
                    last_percent = percent
                    self.bus.ota.emit({"type": "progress", "percent": percent})

            job=None
            try:
                before=ota_client.read_ota_status(host)
                if expected_device_id and before.get("device_id") and before["device_id"]!=expected_device_id:
                    raise ValueError("Địa chỉ đang thuộc thiết bị khác. Chưa gửi firmware.")
                before.setdefault("device_id",expected_device_id);before.setdefault("state_boot_id",previous_boot_id)
                if self._update_journal:job=self._update_journal.begin(host,before,info)
                result = ota_client.upload_firmware(host, key, image, progress)
            except Exception as error:
                if job and self._update_journal:
                    job["phase"]="unconfirmed";self._update_journal.write(job)
                self.bus.ota.emit({"type": "upload_error", "message": str(error)[:200]})
                return
            if job is None:
                job={"host":host,"target_slot":result["target_slot"],"version":result.get("expected_version"),
                     "device_id":expected_device_id,"previous_boot_id":previous_boot_id}
            job["phase"]="uploaded";job["target_slot"]=result["target_slot"]
            if self._update_journal:self._update_journal.write(job)
            self.bus.ota.emit({"type": "uploaded", "data": result})
            self.verify_update_job(job)

        threading.Thread(target=work, name="MeoDeskOtaUpload", daemon=True).start()

    def update_ota(self, event: dict) -> None:
        if self._closing:return
        kind = event.get("type")
        if kind in ("uploaded","upload_error"):self._upload_active=False
        if kind=="uploaded":self._verification_active=True
        if kind in ("confirmed","verify_error","upload_error"):
            self._verification_active=False
            data=event.get("data",{})
            if(kind=="confirmed" and data.get("host") and data["host"]!=self.settings.host and
                    self.settings.device_id and data.get("device_id")==self.settings.device_id):
                self.update_from_worker({"type":"device_relocated","host":self.settings.host,
                                         "device_id":self.settings.device_id,"new_host":data["host"]})
            if self._agent_before_update and not self._quit_after_update:
                self._agent_before_update=False;self.start_agent()
        if self._quit_after_update and not self._upload_active:
            self.quit_app();return
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
        self.heading(box, "STUDIO", "Studio của bạn",
                     "Cảnh, giao diện và chuyển động lấy trạng thái chung từ ESP32.")
        controls, layout = card()
        self.studio_scene = ScrollSafeComboBox()
        self.scene_names = [(12, "Home"), (0, "PC"), (1, "Đồng hồ"), (2, "Focus"),
                            (3, "Wi-Fi"), (4, "Thiết bị"), (5, "Nhạc"), (6, "Print"),
                            (7, "Audio"), (8, "Moto"), (9, "Ngủ"), (10, "Health"),
                            (11, "Trends"), (13, "Network"), (14, "Widgets")]
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
        for action, label, captions in (("theme", "Giao diện TFT", ("Cozy Day", "Midnight", "Auto", "Cyberpunk", "Sakura", "Minimal")),
                                        ("fps", "Mục tiêu FPS", ("20", "25", "30", "Auto 20–25"))):
            row = QHBoxLayout()
            row.addWidget(text(label, "smallLabel"))
            self.studio_buttons[action] = []
            theme_grid = QGridLayout() if action == "theme" else None
            if theme_grid is not None: row.addLayout(theme_grid, 1)
            for value, caption in enumerate(captions):
                entry = button(caption)
                entry.setCheckable(True)
                entry.clicked.connect(lambda checked=False, a=action, v=value: self.send_device_control(a, v))
                self.studio_buttons[action].append(entry)
                if theme_grid is not None: theme_grid.addWidget(entry,value//3,value%3)
                else: row.addWidget(entry)
            layout.addLayout(row)
        layout.addWidget(text("Bốn cảnh yêu thích · dùng cho chế độ Auto", "smallLabel"))
        favorites = QGridLayout()
        self.favorite_scenes = []
        for index in range(4):
            combo = ScrollSafeComboBox()
            for page, caption in self.scene_names:
                combo.addItem(caption, page)
            combo.activated.connect(lambda _, slot=index: self.save_scene_favorite(slot))
            self.favorite_scenes.append(combo)
            favorites.addWidget(combo, index // 2, index % 2)
        layout.addLayout(favorites)
        self.studio_feedback=text("Thay đổi được xác nhận bởi thiết bị; bản nháp trong editor được giữ riêng.","subtitle",wrap=True)
        layout.addWidget(self.studio_feedback)
        box.addWidget(controls)

        scene_panel, scene_box = card()
        self.scene_editor = SceneEditor()
        self.scene_revision = None
        self.scene_request_pending = False
        self.scene_retry_at = 0.0
        self.scene_editor.reload_requested.connect(lambda: self.request_scene_layout(force=True))
        self.scene_editor.apply_requested.connect(self.save_widget_layout)
        scene_box.addWidget(self.scene_editor)
        box.addWidget(scene_panel)
        self.rule_editor=RuleEditor()
        self.rules_revision=None;self.rules_pending=False;self.rules_retry_at=0.0
        self.rule_editor.reload_requested.connect(lambda:self.request_rules(force=True))
        self.rule_editor.save_requested.connect(lambda data:self.request_rules(payload=data))
        box.addWidget(self.rule_editor)

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
        filename, _ = QFileDialog.getOpenFileName(self, "Bộ mèo mimi.desk", "", "mimi.desk Cat Pack (*.meocat)")
        if filename:
            self.pack_path.setText(filename)

    def set_motion_style(self, style: int) -> None:
        self.send_device_control("motion_style", style)

    def save_focus_plan(self) -> None:
        work, rest, goal = [field.value() for field in self.focus_inputs]
        if self.preview_mode:
            self._preview_state.update(focus_work_min=work, focus_break_min=rest, focus_goal=goal,
                                       focus_duration_s=work*60, focus_remaining_s=work*60, focus_running=False)
            self.update_from_worker({"type": "device_state", "data": self._preview_state})
            return
        self.configuration_request("/api/v1/focus/config", {"work_min": work, "break_min": rest, "goal": goal})

    def set_break_reminders(self, enabled) -> None:
        self.settings.break_reminders = bool(enabled)
        if not self.preview_mode:
            try: write_settings(self.settings)
            except OSError as error: self.focus_daily.setText(str(error))

    def send_desk_notification(self) -> None:
        try:
            payload=notification_payload(self.notification_text.text(),hidden=self.notification_hidden.isChecked())
            self.configuration_request("/api/v1/notifications",payload)
        except ValueError as error: self.profile_hint.setText(str(error))

    def apply_startup(self) -> None:
        if self.preview_mode: return
        try:
            startup.set_enabled(self.startup_check.isChecked())
            self.startup_hint.setText("Đã bật khởi động cùng Windows." if self.startup_check.isChecked() else "Đã tắt khởi động cùng Windows.")
        except OSError as error: self.startup_hint.setText(str(error))

    def observe_focus_completion(self,data) -> None:
        if self.preview_mode:
            self.focus_daily.setText("DEMO · nhật ký Focus không được ghi."); return
        if "focus_completed_total" not in data: return
        try:
            if self.focus_journal is None:
                self.focus_journal=FocusJournal(user_config_path().with_name("focus.sqlite3"))
            completed=self.focus_journal.observe(str(data.get("device_id") or self.settings.host),data)
            count,minutes=self.focus_journal.summary()
            goal=data.get("focus_goal",4)
            self.focus_daily.setText(f"Hôm nay · {count}/{goal} phiên · {minutes} phút. Chỉ tính phiên quan sát được khi đang kết nối.")
            if completed and self.settings.break_reminders and not data.get("quiet"):
                self.tray.showMessage("mimi.desk · hoàn thành Focus","Đến giờ nghỉ. Đứng dậy và thư giãn một chút.",QSystemTrayIcon.MessageIcon.Information,8000)
        except (OSError,sqlite3.Error) as error:
            self.focus_daily.setText("Nhật ký tạm không khả dụng: "+str(error)[:100])

    def configuration_request(self, endpoint: str, payload=None, export_path=None) -> None:
        if self.preview_mode:
            self.profile_hint.setText("DEMO · không đọc hoặc ghi cấu hình thiết bị")
            return
        host, key = self.settings.host, self.key
        if not host or not key:
            self.profile_hint.setText("Nhập IP và pairing key ở tab Thiết bị."); return
        def task():
            try:
                result = json_request(host, endpoint, key, payload, timeout=2.0)
                if payload is not None and result.get("ok") is not True:
                    raise ValueError("Thiết bị không chấp nhận cấu hình.")
                if export_path:
                    device_config.save_backup(export_path, result)
                if endpoint == "/api/v1/ota/history":
                    self.bus.status.emit({"type": "ota_history", "data": result})
                    return
                message=("Chế độ yên tĩnh đang ẩn thông báo." if result.get("suppressed") else "Đã gửi thông báo.") if endpoint=="/api/v1/notifications" else "Đã xuất cấu hình." if export_path else "Đã lưu cấu hình trên ESP32."
                self.bus.status.emit({"type": "config_result", "ok": True, "endpoint": endpoint, "message": message})
                if not export_path: self.refresh_device_state()
            except (OSError, ValueError, TimeoutError) as error:
                self.bus.status.emit({"type": "config_result", "message": str(error)[:160]})
        threading.Thread(target=task, name="MeoDeskConfig", daemon=True).start()

    def request_scene_layout(self, force=False) -> None:
        if self.preview_mode:
            self.scene_editor.hint.setText("DEMO · bố cục cục bộ, không đọc thiết bị")
            return
        if self.scene_request_pending or not self.settings.host or not self.key:
            return
        host, key = self.settings.host, self.key
        before = json.dumps(self.scene_editor.data, sort_keys=True)
        self.scene_request_pending = True
        def task():
            try:
                raw=json_request(host,"/api/v1/scene-layout",key,timeout=2.0)
                data=validate_layout(raw)
                base={"expected_revision":raw["revision"],"expected_boot_id":raw["state_boot_id"]} if "revision" in raw and "state_boot_id" in raw else None
                self.bus.status.emit({"type":"scene_layout", "host":host, "data":data, "force":force, "before":before,"base":base})
            except (OSError, ValueError, TimeoutError) as error:
                self.bus.status.emit({"type":"scene_error", "host":host, "message":str(error)[:160]})
        threading.Thread(target=task,name="MeoDeskSceneRead",daemon=True).start()

    def save_widget_layout(self, data) -> None:
        if self.preview_mode:
            self.scene_editor.dirty = False
            self.scene_editor.hint.setText("DEMO · đã áp dụng bố cục cục bộ")
            self.send_device_control("page",14)
            return
        if not self.settings.host or not self.key:
            self.scene_editor.hint.setText("Nhập IP và pairing key ở tab Thiết bị."); return
        host, key = self.settings.host, self.key
        data=validate_layout(data)
        if self._studio_cas and not self._scene_base:
            self.scene_editor.hint.setText("Đọc lại bố cục từ thiết bị trước khi lưu bản nháp.");return
        payload=dict(data,**(self._scene_base or {})) if self._studio_cas else data
        def task():
            try:
                result = json_request(host,"/api/v1/scene-layout",key,payload,timeout=2.0)
                if result.get("ok") is not True: raise ValueError("Thiết bị chưa lưu bố cục.")
                base={"expected_revision":result["revision"],"expected_boot_id":result["state_boot_id"]} if "revision" in result and "state_boot_id" in result else None
                self.bus.status.emit({"type":"scene_saved", "host":host, "data":data,"base":base})
            except (OSError, ValueError, TimeoutError) as error:
                message="Bố cục đã thay đổi ở giao diện khác hoặc thiết bị vừa khởi động lại. Bản nháp được giữ; đọc lại trước khi lưu." if isinstance(error,HTTPError) and error.code==409 else str(error)[:160]
                self.bus.status.emit({"type":"scene_error", "host":host, "message":message})
        threading.Thread(target=task,name="MeoDeskSceneSave",daemon=True).start()

    def request_rules(self,payload=None,force=False) -> None:
        if self.preview_mode:
            if payload is not None:self.rule_editor.dirty=False
            self.rule_editor.hint.setText("DEMO · quy tắc chỉ được giữ trong bản xem trước.");return
        if self.rules_pending:return
        host,key=self.settings.host,self.key
        if not host or not key:
            self.rule_editor.hint.setText("Nhập IP và pairing key ở tab Thiết bị.");return
        before=self.rule_editor.editor.toPlainText();self.rules_pending=True
        if payload is not None and self._studio_cas and not self._rules_base:
            self.rules_pending=False;self.rule_editor.hint.setText("Đọc lại quy tắc từ thiết bị trước khi lưu bản nháp.");return
        outgoing=dict(payload,**(self._rules_base or {})) if payload is not None and self._studio_cas else payload
        def task():
            try:
                result=json_request(host,"/api/v1/rules",key,outgoing,timeout=2.0)
                if payload is not None and result.get("ok") is not True:raise ValueError("Thiết bị chưa lưu quy tắc.")
                data=validate_rules(payload if payload is not None else result)
                base={"expected_revision":result["revision"],"expected_boot_id":result["state_boot_id"]} if "revision" in result and "state_boot_id" in result else None
                self.bus.status.emit(dict(type="rules_result",host=host,data=data,saved=payload is not None,force=force,before=before,base=base))
            except (OSError,ValueError,TimeoutError) as error:
                message="Quy tắc đã thay đổi ở giao diện khác hoặc thiết bị vừa khởi động lại. Bản nháp được giữ; đọc lại trước khi lưu." if isinstance(error,HTTPError) and error.code==409 else str(error)[:160]
                self.bus.status.emit(dict(type="rules_error",host=host,message=message))
        threading.Thread(target=task,name="MeoDeskRules",daemon=True).start()

    def export_device_config(self) -> None:
        if self.preview_mode:
            self.configuration_request("/api/v1/config"); return
        path, _ = QFileDialog.getSaveFileName(self, "Xuất cấu hình mimi.desk", "meodesk-profile.json", "JSON (*.json)")
        if path: self.configuration_request("/api/v1/config", export_path=path)

    def restore_device_config(self) -> None:
        if self.preview_mode:
            self.configuration_request("/api/v1/config"); return
        path, _ = QFileDialog.getOpenFileName(self, "Khôi phục cấu hình mimi.desk", "", "JSON (*.json)")
        if not path: return
        try:
            data = device_config.load_backup(path)
        except (OSError, ValueError) as error:
            self.profile_hint.setText(str(error)); return
        self.configuration_request("/api/v1/config", data)

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
            self.studio_feedback.setText("DEMO · thay đổi chỉ áp dụng trong bản xem trước")
            return
        host = self.host_edit.text().strip() or self.settings.host
        key = self.key_edit.text().strip() or self.key
        if not host or not key:
            self.pack_motion_status.setText("Nhập IP và pairing key trong tab Thiết bị trước.")
            return
        self.pack_motion_status.setText("Đang gửi thay đổi đến ESP32…")
        self.studio_feedback.setText("Đang áp dụng thay đổi…")

        def work() -> None:
            try:
                result = json_request(host, "/api/control", key,
                                      {"action": action, "value": value}, timeout=1.8)
                if not result.get("ok"):
                    raise OSError("ESP32 không chấp nhận thay đổi. Kiểm tra kết nối và firmware.")
                status = json_request(host, "/api/v1/status", timeout=2.0)
                self.bus.status.emit({"type": "device_state", "data": status, "source": "http"})
                self.bus.status.emit({"type":"control_saved","host":host})
            except Exception as error:
                self.bus.status.emit({"type": "motion_error", "message": str(error)[:180]})

        threading.Thread(target=work, name="MeoDeskStudioControl", daemon=True).start()

    def refresh_device_state(self) -> None:
        if self.preview_mode:
            return
        if self._state_request_pending or not self.settings.host or not self._state_retry.ready(time.monotonic()):
            return
        self._state_request_pending = True
        host = self.settings.host

        def read():
            try:
                data = json_request(host, "/api/status", timeout=1.0)
                self.bus.status.emit({"type": "device_state", "data": data, "source": "http", "host": host})
            except (OSError, ValueError, TimeoutError):
                self.bus.status.emit({"type": "state_error", "host": host})
            finally:
                self._state_request_pending = False

        threading.Thread(target=read, name="MeoDeskStateRead", daemon=True).start()

    def stream_disconnected(self) -> None:
        self._stream_seen_at = 0.0
        if self.worker:
            self.worker.stream_active = False
        self.sync_label.setText("HTTP · đang kết nối lại")

    def maintain_state_link(self) -> None:
        if self._closing or self.preview_mode or not self._start_services or not self.settings.host:
            return
        if self._stream_host != self.settings.host:
            self._stream_host = self.settings.host
            self._stream_supported = False
            self.state_cursor = StateCursor()
            self._stream_seen_at = 0.0
            self.state_socket.abort()
            self._state_retry.reset()
            self._socket_retry.reset()
        fresh = self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5
        if self.worker:
            self.worker.stream_active = bool(fresh)
        if not fresh:
            self.sync_label.setText("HTTP · đang đồng bộ")
            self.refresh_device_state()
            if self.state_socket.state() == QAbstractSocket.SocketState.ConnectedState:
                self.state_socket.abort()
            if (self._stream_supported and self.state_socket.state() == QAbstractSocket.SocketState.UnconnectedState
                    and self._socket_retry.ready(time.monotonic())):
                self._socket_retry.failed(time.monotonic())
                self.state_socket.open(QUrl(f"ws://{self.settings.host}:81/"))
            self.rediscover_device()

    def rediscover_device(self) -> None:
        if (not self.settings.device_id or self._rediscovery_pending
                or time.monotonic() < self._next_rediscovery
                or (self.device_seen_at and time.monotonic() - self.device_seen_at < 10)):
            return
        self._next_rediscovery = time.monotonic() + 30
        self._rediscovery_pending = True
        host, device_id = self.settings.host, self.settings.device_id

        def search():
            try:
                for item in discovery.discover(host, lan_scan=False):
                    if item.get("device_id") == device_id and item["host"] != host:
                        self.bus.status.emit({"type": "device_relocated", "host": host,
                                              "device_id": device_id, "new_host": item["host"]})
                        break
            except (OSError, ValueError):
                pass
            finally:
                self._rediscovery_pending = False

        threading.Thread(target=search, name="MimiDeskRediscovery", daemon=True).start()

    def receive_socket_state(self, message: str) -> None:
        if self._stream_host != self.settings.host or len(message) > 32768:
            return
        try:
            data = json.loads(message)
        except ValueError:
            return
        if isinstance(data,dict) and data.get("type")=="update":
            if hasattr(self,"server_updates"):
                self.server_updates.receive_remote(data.get("update_policy",{}),data.get("update_state",{}))
            return
        if not isinstance(data, dict) or data.get("type") != "state":
            return
        self.update_from_worker({"type": "device_state", "data": data, "source": "socket", "host": self._stream_host})

    def tick_focus_display(self) -> None:
        self.refresh_connection_badge()
        self.tick_media_display()
        self.pc_chart.update()
        seconds = self.focus_display.seconds()
        if seconds is not None:
            self.focus_clock.setText(f"{seconds // 60:02d}:{seconds % 60:02d}")

    def refresh_connection_badge(self) -> None:
        live=self.device_seen_at>0 and time.monotonic()-self.device_seen_at<6 and not self._pairing_rejected
        caption="Xem trước · mô phỏng" if self.preview_mode else "Kiểm tra mã ghép nối" if self._pairing_rejected else "Đã kết nối" if live else "Mất kết nối" if self.device_seen_at else "Đang kết nối…" if self.settings.host else "Chưa có thiết bị"
        if self.connection_badge.text()!=caption:
            self.connection_badge.setText(caption)
            self.connection_badge.setProperty("live",live and not self.preview_mode)
            self.connection_badge.style().unpolish(self.connection_badge)
            self.connection_badge.style().polish(self.connection_badge)

    def tick_media_display(self) -> None:
        media = self.media_display.state
        if self.preview_mode and media.get("available"):
            self.media_display.update(media) # Static DEMO sample; no provider/network.
        if not media.get("available"): return
        progress = self.media_display.position()
        if progress is None:
            self.music_state.setText("ĐỒNG BỘ NHẠC GIÁN ĐOẠN · đang chờ dữ liệu")
            return
        length = int(media.get("duration_ms") or 0)
        self.progress.setValue(round(100*progress/length) if length else 0)
        self.music_state.setText(("ĐANG PHÁT" if media.get("playing") else "TẠM DỪNG") +
                                f" · {progress//60000}:{progress//1000%60:02d}" +
                                (f" / {length//60000}:{length//1000%60:02d}" if length else " · thời lượng N/A"))

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
        if kind=="control_saved":
            if event.get("host")==self.settings.host:self.studio_feedback.setText("Đã áp dụng. Giao diện hiển thị trạng thái mới nhất từ thiết bị.")
            return
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
        show = QAction("Mở mimi.desk Companion", self)
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
        self.tray.setToolTip("mimi.desk Companion " + VERSION)
        self.tray.activated.connect(lambda reason: self.show_window()
                            if reason == QSystemTrayIcon.ActivationReason.DoubleClick else None)
        if QSystemTrayIcon.isSystemTrayAvailable():
            self.tray.show()

    def set_tab(self, index: int) -> None:
        self.stack.setCurrentIndex(index)
        self.stack.updateGeometry()
        self.content_scroll.verticalScrollBar().setValue(0)
        if index==5 and not self.preview_mode and self._start_services:
            self.server_updates.send("load",{})
        self.page_trail.setText("mimi.desk / "+("Tổng quan","Thiết bị","Âm nhạc","Tập trung","Cài đặt","Cập nhật","Studio")[index])
        for i, nav in enumerate(self.nav):
            nav.setProperty("chosen", i == index or (index==5 and i==1))
            nav.style().unpolish(nav)
            nav.style().polish(nav)

    def update_from_worker(self, event: dict) -> None:
        if self._closing:
            return
        if "media_generation" in event and event["media_generation"] != self._media_generation:
            return
        kind = event.get("type")
        if kind == "state_error":
            if event.get("host") == self.settings.host:
                self._state_retry.failed(time.monotonic())
            return
        if kind == "device_relocated":
            if event.get("host") != self.settings.host or event.get("device_id") != self.settings.device_id:
                return
            self.settings.host = discovery.normalize_host(event["new_host"])
            self.state_socket.abort()
            self._stream_host = ""
            self._stream_seen_at = 0.0
            self._state_retry.reset()
            self._socket_retry.reset()
            self.host_edit.setText(self.settings.host)
            if self.key:
                secret_store.save(self.settings.host, self.key)
            write_settings(self.settings)
            # Preserve a deliberately paused agent after an address change.
            running = self.worker is not None and self.worker.running
            if self.worker:
                self.worker.stop()
                self.worker = None
            if running:
                self.start_agent()
            self.device_seen_at = 0.0
            self.hint.setText("Đã tìm lại thiết bị. Đang kết nối lại…")
            return
        if kind in ("rules_result","rules_error"):
            if event.get("host")!=self.settings.host:return
            self.rules_pending=False
            if kind=="rules_error":
                self.rules_revision=None;self.rules_retry_at=time.monotonic()+5;self.rule_editor.hint.setText(event["message"])
            elif event["saved"]:
                self._rules_base=event.get("base")
                try:unchanged=self.rule_editor.data()==event["data"]
                except ValueError:unchanged=False
                if unchanged:self.rule_editor.dirty=False
                self.rule_editor.hint.setText("Đã lưu quy tắc." if unchanged else "Đã lưu. Thay đổi mới trong bản nháp chưa được gửi.")
                self.refresh_device_state()
            else:
                force=event["force"] and self.rule_editor.editor.toPlainText()==event["before"]
                if not self.rule_editor.dirty or force:self._rules_base=event.get("base")
                self.rule_editor.set_data(event["data"],force=force)
            return
        if kind in ("scene_layout", "scene_saved", "scene_error"):
            if event.get("host") != self.settings.host: return
            self.scene_request_pending = False
            if kind == "scene_error":
                self.scene_retry_at = time.monotonic()+5
                self.scene_revision = None
                self.scene_editor.hint.setText(event["message"])
            elif kind == "scene_saved":
                self._scene_base=event.get("base")
                if self.scene_editor.data == event["data"]:
                    self.scene_editor.dirty = False
                    self.scene_editor.hint.setText("Đã lưu bố cục lên thiết bị.")
                self.send_device_control("page",14)
            else:
                unchanged = json.dumps(self.scene_editor.data, sort_keys=True) == event["before"]
                if not self.scene_editor.dirty or (event.get("force",False) and unchanged):self._scene_base=event.get("base")
                self.scene_editor.set_data(event["data"],force=event.get("force",False) and unchanged)
            return
        if kind == "device_state":
            if event.get("host", self.settings.host) != self.settings.host:
                return
            if event.get("source") == "http" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            data = event["data"]
            device_id = str(data.get("device_id") or "")[:64]
            if not self.preview_mode and self.settings.device_id and device_id and device_id != self.settings.device_id:
                self.hint.setText("Địa chỉ này đang thuộc thiết bị khác. Đang tìm lại thiết bị đã ghép nối.")
                return
            self._state_retry.reset()
            if not self.preview_mode and device_id and not self.settings.device_id:
                self.settings.device_id = device_id
                write_settings(self.settings)
            old_boot = self.state_cursor.boot
            if not self.state_cursor.accept(data):
                return
            if self.worker and old_boot is not None and old_boot != self.state_cursor.boot:
                self.worker.reset_device_session()
            self._stream_supported = "state_websocket_v1" in data.get("capabilities", [])
            self._studio_cas = "studio_cas_v1" in data.get("capabilities", [])
            self.observe_focus_completion(data)
            rules_revision=(self.settings.host,data.get("state_boot_id"),data.get("rules_revision"))
            if ("automation_rules_v1" in data.get("capabilities",[]) and rules_revision!=self.rules_revision
                    and self.key and not self.rules_pending and time.monotonic()>=self.rules_retry_at):
                self.rules_revision=rules_revision;self.request_rules()
            scene_revision = (self.settings.host, data.get("state_boot_id"), data.get("scene_layout_revision"))
            if ("widget_scene_v1" in data.get("capabilities",[]) and scene_revision != self.scene_revision
                    and self.key and not self.scene_request_pending and time.monotonic() >= self.scene_retry_at):
                self.scene_revision = scene_revision
                self.request_scene_layout()
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
                self._socket_retry.reset()
                self._stream_seen_at = time.monotonic()
                self.sync_label.setText("WebSocket · đồng bộ trực tiếp")
            self.device_seen_at=time.monotonic()
            self.setup_hint.setText("Đã kết nối với thiết bị." if not self._pairing_rejected else "Kiểm tra lại mã ghép nối.")
            if isinstance(data.get("update_state"),dict):
                self.server_updates.receive_remote(data.get("update_policy",{}),data["update_state"])
                writing=data["update_state"].get("phase") in ("downloading","rebooting")
                self.ota_start.setEnabled(not writing and not self._upload_active)
                self.pack_start.setEnabled(not writing and not self._upload_active)
            self.refresh_connection_badge()
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
            if event.get("source") != "state":
                self._pairing_rejected = False
            if event.get("source") != "state" and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            data = event["data"]
            self.pc_chart.add(data,event.get("host",self.settings.host))
            self.cpu.setText(f"{data['cpu_percent']:.0f}%")
            self.gpu.setText("N/A" if data.get("gpu_percent") is None else f"{data['gpu_percent']:.0f}%")
            self.ram.setText(f"{data['ram_percent']:.0f}%")
            cpu = data.get("cpu_temp_c")
            gpu = data.get("gpu_temp_c")
            self.temps.setText(f"{cpu:.0f}° / {gpu:.0f}°" if cpu is not None and gpu is not None
                               else f"{gpu:.0f}° GPU" if gpu is not None else "N/A")
            self.state.setText("DEMO · XEM TRƯỚC" if self.preview_mode else "Kiểm tra mã ghép nối" if self._pairing_rejected else "● ĐÃ KẾT NỐI")
            self.set_cat_mood(event.get("emotion", "healthy"))
            mode = "DEMO / mô phỏng" if data.get("demo") else "Dữ liệu thực"
            self.hint.setText("Đang xem dữ liệu mô phỏng." if data.get("demo") else
                              "Kiểm tra lại mã ghép nối trong Thiết bị." if self._pairing_rejected else
                              "Máy tính và thiết bị đang đồng bộ.")
            up = data.get("net_up_kbps")
            down = data.get("net_down_kbps")
            used = data.get("vram_used_gb")
            total = data.get("vram_total_gb")
            value = lambda n: f"{n:.1f}" if n is not None else "N/A"
            self.extra_metrics.setText(f"NET ↓ {value(down)} KB/s  ↑ {value(up)} KB/s"
                                       f"   ·   VRAM {value(used)} / {value(total)} GB")
            self.tray.setToolTip("mimi.desk · Trực tuyến")
        elif kind == "media":
            if event.get("source") == "state" and self.media_monitor:
                return
            if event.get("source") not in ("state", "local") and self._stream_seen_at and time.monotonic() - self._stream_seen_at < 2.5:
                return
            media = event["data"]
            self.media_display.update(media)
            if media.get("available"):
                self.song.setText(media.get("title") or "Không có tên bài")
                self.artist.setText(media.get("artist") or "Không rõ nghệ sĩ")
                self.tick_media_display()
            else:
                self.song.setText("Chưa có bản nhạc")
                self.artist.setText({
                    "starting": "Đang tìm trình phát trên máy tính…",
                    "missing_provider": "Thiếu thành phần đọc nhạc Windows. Cài lại bản mimi.desk đầy đủ.",
                    "provider_error": "Chưa đọc được nhạc từ Windows. Hãy thử phát nhạc lại; app sẽ tự kiểm tra tiếp.",
                    "disabled": "Đã tắt đọc nhạc. Bật lại trong Cài đặt để tiếp tục.",
                }.get(media.get("reason"), "Mở Spotify hoặc trình phát trên PC và phát một bản nhạc."))
                self.progress.setValue(0)
                self.music_state.setText("KHÔNG CÓ DỮ LIỆU / N/A")
        elif kind == "artwork":
            pixels = event.get("pixels")
            if isinstance(pixels, bytes) and len(pixels) == 8192:
                # Copy: original bytes belong to the worker's signal payload.
                image = QImage(pixels, 64, 64, 64 * 2, QImage.Format.Format_RGB16).copy()
                self.music_cover.setPixmap(QPixmap.fromImage(image).scaled(
                    218, 218, Qt.AspectRatioMode.KeepAspectRatio,
                    Qt.TransformationMode.SmoothTransformation))
            else:
                self.music_cover.clear()
                self.music_cover.setText("Chưa có\nảnh bìa")
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
            total = int(focus.get("focus_duration_s") or (1500 if work else 300))
            self.focus_progress.setValue(round(100 * (total - min(total, seconds)) / total))
            self.focus_toggle.setText("Ⅱ  TẠM DỪNG" if running else "▶  TIẾP TỤC")
            self.focus_hint.setText("Đồng bộ từ ESP32 · " + ("FOCUS" if work else "BREAK"))
            for field, name, default in zip(self.focus_inputs, ("focus_work_min", "focus_break_min", "focus_goal"), (25,5,4)):
                if not self.focus_plan_dirty and not field.hasFocus():
                    blocked = QSignalBlocker(field)
                    field.setValue(int(focus.get(name, default)))
                    del blocked
        elif kind == "config_result":
            if event.get("ok") and event.get("endpoint") == "/api/v1/focus/config":
                self.focus_plan_dirty = False
            self.profile_hint.setText(event.get("message", ""))
            self.focus_hint.setText(event.get("message", ""))
        elif kind == "ota_history":
            updates = event["data"].get("updates", [])
            self.ota_history.setText("\n".join(" · ".join(str(item.get(key) or "") for key in
                ("phase", "version_before", "version_after", "target_slot")) for item in reversed(updates[-6:]))
                or "Chưa có cập nhật được ghi lại bởi firmware này.")
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
            self.studio_feedback.setText(event.get("message","Chưa áp dụng được thay đổi. Kiểm tra kết nối rồi thử lại."))
        else:
            if event.get("host", self.settings.host) != self.settings.host:
                return
            message = event.get("message", "Đang chờ thiết bị")
            self.hint.setText(message)
            if kind == "error":
                if event.get("code") == 401:
                    self._pairing_rejected = True
                self.refresh_connection_badge()
                self.state.setText("○ CHƯA KẾT NỐI")
                self.set_cat_mood("rescue")
                self.tray.setToolTip("mimi.desk · Ngoại tuyến")

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
        if host != self.settings.host:
            self.settings.device_id = ""
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
        if self._closing: return
        if self.preview_mode:
            return
        if not self.settings.host or not self.key:
            self.set_tab(1)
            self.hint.setText("Nhập IP thiết bị và pairing key ở tab Thiết bị.")
            return
        if self.worker is not None:
            self.worker.stop()
        self.worker = CompanionWorker(self.settings, self.key, self.bus.status.emit,
                                      media_source=self.media_monitor.snapshot if self.media_monitor else None)
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
        self.configure_media_monitor()
        if not self.settings.media:
            self.update_from_worker({"type": "media", "source": "local",
                                     "data": {"available": False, "reason": "disabled"}})
            self.update_from_worker({"type": "artwork", "pixels": None})
        self.hint.setText("Đã lưu cài đặt.")

    def open_web(self) -> None:
        host = self.settings.host
        if host:
            webbrowser.open(f"http://{host}/")
        else:
            self.set_tab(1)

    def show_window(self) -> None:
        self.showNormal()
        self.raise_()
        self.activateWindow()

    def closeEvent(self, event) -> None:
        if QSystemTrayIcon.isSystemTrayAvailable():
            event.ignore()
            self.hide()
        else:
            self.quit_app()

    def quit_app(self) -> None:
        if self._upload_active:
            self._quit_after_update=True
            self.ota_hint.setText("Đang gửi firmware. App sẽ thoát sau khi gửi xong; mở lại để kiểm tra kết quả.")
            self.hide();return
        self._closing = True
        self._stop_updates.set()
        if hasattr(self,"preview_graph_timer"): self.preview_graph_timer.stop()
        self.state_timer.stop()
        self.display_timer.stop()
        self.state_socket.abort()
        if self.worker is not None:
            self.worker.stop()
        if self.media_monitor is not None:
            self.media_monitor.stop()
        self.tray.hide()
        if self.focus_journal is not None: self.focus_journal.close()
        QApplication.instance().quit()


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("mimi.desk")
    app.setQuitOnLastWindowClosed(False)
    instance = SingleInstance(preview="--preview" in sys.argv)
    try:
        if not instance.acquire():
            return 0
        window = MainWindow(preview="--preview" in sys.argv)
        instance.activated.connect(window.show_window)
        window.show()
        return app.exec()
    except RuntimeError as error:
        QMessageBox.critical(None, "mimi.desk", str(error))
        return 1
    finally:
        instance.release()


if __name__ == "__main__":
    sys.exit(main())
