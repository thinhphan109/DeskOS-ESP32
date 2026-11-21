"""Compact server OTA controls; the device owns policy and update state."""
import uuid
from PySide6.QtCore import Qt, Signal, QSignalBlocker
from PySide6.QtWidgets import QWidget, QVBoxLayout, QHBoxLayout, QFormLayout, QLabel, QLineEdit, QCheckBox, QPushButton, QProgressBar
from ui_controls import ScrollSafeComboBox, ScrollSafeSpinBox
from update_client import MESSAGES
from update_contract import valid_url


class UpdatePanel(QWidget):
    requested = Signal(str, object)

    def __init__(self, preview=False, parent=None):
        super().__init__(parent)
        self.preview = preview
        self.dirty = False
        self.revision = 0
        self.pending = False
        self.supported = False
        self.state = {}
        box = QVBoxLayout(self);box.setContentsMargins(0,0,0,0)
        title = QLabel("Cập nhật từ server");title.setObjectName("cardTitle");box.addWidget(title)
        self.notice = QLabel("Đọc chính sách từ thiết bị để bắt đầu.")
        self.notice.setWordWrap(True);self.notice.setTextFormat(Qt.TextFormat.PlainText);box.addWidget(self.notice)
        form = QFormLayout();form.setFieldGrowthPolicy(QFormLayout.FieldGrowthPolicy.AllNonFixedFieldsGrow)
        self.url = QLineEdit();self.url.setMaxLength(511);self.url.setPlaceholderText("https://…/channels/preview.json")
        self.mode = ScrollSafeComboBox();self.mode.addItem("Kiểm tra và thông báo", "notify");self.mode.addItem("Tự cập nhật", "auto")
        self.channel = ScrollSafeComboBox();self.channel.addItem("Preview", "preview");self.channel.addItem("Stable", "stable")
        self.interval = ScrollSafeSpinBox();self.interval.setRange(60,86400);self.interval.setSingleStep(60);self.interval.setValue(120);self.interval.setSuffix(" giây")
        self.defer = QCheckBox("Hoãn khi đang tập trung");self.defer.setChecked(True)
        for caption, control in (("Manifest",self.url),("Chế độ",self.mode),("Kênh",self.channel),("Kiểm tra mỗi",self.interval)):
            form.addRow(caption,control);control.setAccessibleName(caption)
        form.addRow(self.defer);box.addLayout(form)
        self.fields = (self.url,self.mode,self.channel,self.interval,self.defer)
        self.url.textEdited.connect(self._edited);self.mode.activated.connect(self._edited)
        self.channel.activated.connect(self._edited);self.interval.valueChanged.connect(self._edited);self.defer.toggled.connect(self._edited)
        row = QHBoxLayout()
        self.reload = QPushButton("Đọc lại");self.save = QPushButton("Lưu chính sách")
        self.check = QPushButton("Kiểm tra ngay");self.start = QPushButton("Cập nhật")
        self.start.setObjectName("primary")
        for button in (self.reload,self.save,self.check,self.start):
            if button is not self.start:button.setObjectName("secondary")
            row.addWidget(button)
        box.addLayout(row)
        self.progress=QProgressBar();self.progress.setRange(0,100);self.progress.setValue(0);box.addWidget(self.progress)
        self.release=QLabel("Chưa kiểm tra phiên bản mới.");self.release.setWordWrap(True);self.release.setTextFormat(Qt.TextFormat.PlainText);box.addWidget(self.release)
        self.reload.clicked.connect(lambda:self.send("load",{}))
        self.save.clicked.connect(self.save_policy)
        self.check.clicked.connect(lambda:self.send("check",{"command_id":uuid.uuid4().hex}))
        self.start.clicked.connect(lambda:self.send("start",{"release_id":self.state.get("release_id",""),
                                                             "expected_revision":self.revision,"command_id":uuid.uuid4().hex}))
        if preview:self.notice.setText("DEMO · chính sách và tiến độ chỉ để xem bố cục.")
        self._controls()

    def _edited(self, *_):
        self.dirty=True;self._controls()

    def _controls(self):
        allowed=self.supported and not self.pending and not self.preview
        for field in self.fields:field.setEnabled(allowed)
        self.reload.setEnabled(not self.pending and not self.preview)
        self.save.setEnabled(allowed and self.dirty)
        self.check.setEnabled(allowed and not self.dirty)
        self.start.setEnabled(allowed and not self.dirty and bool(self.state.get("release_id")) and
                              self.state.get("phase") in ("available","error"))

    def policy(self):
        return {"expected_revision":self.revision,"manifest_url":self.url.text().strip(),"mode":self.mode.currentData(),
                "channel":self.channel.currentData(),"interval_s":self.interval.value(),"defer_focus":self.defer.isChecked()}

    def save_policy(self):
        if self.url.text().strip() and not valid_url(self.url.text().strip()):
            self.notice.setText("Dùng địa chỉ HTTPS, hoặc HTTP nội bộ để debug.");return
        self.send("configure",self.policy())

    def send(self,operation,payload):
        if self.preview or self.pending:return
        self.pending=True;self._controls();self.notice.setText("Đang trao đổi với thiết bị…")
        self.requested.emit(operation,payload)

    def complete(self,data,operation="load"):
        self.pending=False;self.supported=data.get("supported",False)
        if not self.supported:
            self.notice.setText("Thiết bị cần firmware cầu nối hỗ trợ OTA server. Có thể cập nhật bằng file bên dưới.")
            self._controls();return
        policy=data.get("policy",{})
        if operation in ("load","configure") or not self.dirty:
            blockers=[QSignalBlocker(field) for field in self.fields]
            self.url.setText(policy.get("manifest_url",""));self.mode.setCurrentIndex(max(0,self.mode.findData(policy.get("mode"))))
            self.channel.setCurrentIndex(max(0,self.channel.findData(policy.get("channel"))))
            self.interval.setValue(policy.get("interval_s",120));self.defer.setChecked(policy.get("defer_focus",True))
            self.revision=policy.get("revision",0);self.dirty=False;del blockers
        self.apply_state(data.get("update",{}));self.notice.setText("DEMO · chính sách và tiến độ mô phỏng." if self.preview or data.get("demo") else "Đã lưu chính sách." if operation=="configure" else "Chính sách lấy từ thiết bị.")
        self._controls()

    def reject(self,message):
        self.pending=False;self.notice.setText(message);self._controls()

    def receive_remote(self,policy,state):
        if not self.preview:self.apply_state(state)
        if self.supported and policy.get("revision",self.revision)!=self.revision:
            if self.dirty:self.notice.setText("Chính sách vừa thay đổi ở giao diện khác. Bản nháp vẫn được giữ; đọc lại trước khi lưu.")
            elif not self.pending:self.send("load",{})

    def apply_state(self,state):
        self.state=dict(state)
        phase=state.get("phase","idle");expected=state.get("expected",0)
        self.progress.setVisible(phase in ("checking","downloading","rebooting"))
        if phase in ("checking","rebooting"):
            self.progress.setRange(0,0)
        else:
            self.progress.setRange(0,100)
            self.progress.setValue(round(100*min(state.get("written",0),expected)/expected) if expected else 0)
        message=MESSAGES.get(state.get("code"),"Đang chờ trạng thái cập nhật từ thiết bị.")
        if phase=="countdown":message+=f" Bắt đầu sau {state.get('countdown_s',0)} giây."
        if state.get("quarantined"):message+=" Đã tạm ngừng thử tự động; chỉ thử lại khi chủ động bấm Cập nhật."
        target=state.get("active_version") if state.get("manual_request") or phase in ("downloading","rebooting") else state.get("available_version")
        versions=f"{state.get('current_version','—')} → {target or 'chưa có bản mới'}"
        if target and state.get("available_version") and target!=state["available_version"]:
            versions+=f" · Đang chờ thêm bản {state['available_version']}"
        notes=state.get("notes","")
        self.release.setText(versions+"\n"+message+("\n"+notes if notes else ""));self._controls()
