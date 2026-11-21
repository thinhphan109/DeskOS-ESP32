"""Compact advanced JSON editor for the bounded device rule contract."""
import json
from pathlib import Path
from PySide6.QtCore import Signal
from PySide6.QtGui import QFont
from PySide6.QtWidgets import QWidget,QVBoxLayout,QHBoxLayout,QPushButton,QLabel,QPlainTextEdit,QFileDialog
from automation_rules import PRESET,validate_rules


class RuleEditor(QWidget):
    reload_requested=Signal()
    save_requested=Signal(dict)

    def __init__(self,parent=None):
        super().__init__(parent)
        self.dirty=False;self.loading=False
        box=QVBoxLayout(self);box.setContentsMargins(0,0,0,0)
        toggle=QPushButton("Quy tắc tự động · chỉnh JSON")
        toggle.setObjectName("secondary");toggle.setCheckable(True);box.addWidget(toggle)
        body=QWidget();layout=QVBoxLayout(body);layout.setContentsMargins(0,10,0,0)
        help_text=QLabel("Tối đa 6 quy tắc. Bản mẫu: CPU ≥90% trong 30 giây → thông báo; 23 giờ → Midnight. Chỉ chạy sau khi lưu lên thiết bị.")
        help_text.setWordWrap(True);layout.addWidget(help_text)
        self.editor=QPlainTextEdit();self.editor.setFont(QFont("Consolas",10));self.editor.setFixedHeight(210)
        self.editor.setAccessibleName("Quy tắc tự động JSON")
        self.editor.setStyleSheet("QPlainTextEdit{background:#fffefa;color:#263e34;border:1px solid #bac8b9;border-radius:8px;padding:8px;selection-background-color:#cbe7d4}")
        layout.addWidget(self.editor)
        row=QHBoxLayout()
        for caption,callback in (("Bản mẫu",self.preset),("Tắt tất cả",self.clear_rules),("Nhập JSON",self.import_file),("Xuất JSON",self.export_file)):
            control=QPushButton(caption);control.setObjectName("secondary");control.clicked.connect(callback);row.addWidget(control)
        layout.addLayout(row)
        row=QHBoxLayout();reload_button=QPushButton("Tải lại từ thiết bị");reload_button.setObjectName("secondary")
        reload_button.clicked.connect(self.reload_requested.emit);row.addWidget(reload_button)
        self.save_button=QPushButton("Lưu quy tắc");self.save_button.setObjectName("primary")
        self.save_button.clicked.connect(self.save);row.addWidget(self.save_button);layout.addLayout(row)
        self.hint=QLabel();self.hint.setWordWrap(True);layout.addWidget(self.hint)
        box.addWidget(body);body.hide();toggle.toggled.connect(body.setVisible)
        self.editor.textChanged.connect(self.changed)
        self.set_data(dict(schema=1,rules=[]))

    def data(self):
        text=self.editor.toPlainText()
        if len(text.encode('utf-8'))>8192:raise ValueError("Rule JSON exceeds 8 KiB")
        return validate_rules(json.loads(text))

    def set_data(self,data,force=False):
        if self.dirty and not force:
            self.hint.setText("Thiết bị có quy tắc mới. Bản nháp được giữ; dùng Tải lại để nhận thay đổi.");return
        self.loading=True;self.editor.setPlainText(json.dumps(validate_rules(data),indent=2));self.loading=False
        self.dirty=False;self.save_button.setEnabled(True);self.hint.setText("Quy tắc đã tải. Bản nháp chỉ chạy sau khi lưu.")

    def changed(self):
        if self.loading:return
        self.dirty=True
        try:self.data();self.save_button.setEnabled(True);self.hint.setText("Bản nháp hợp lệ · chưa lưu lên thiết bị.")
        except ValueError as error:self.save_button.setEnabled(False);self.hint.setText(str(error))

    def preset(self):self.set_data(PRESET,force=True);self.dirty=True;self.changed()
    def clear_rules(self):self.set_data(dict(schema=1,rules=[]),force=True);self.dirty=True;self.changed()
    def save(self):
        try:self.save_requested.emit(self.data())
        except ValueError as error:self.hint.setText(str(error))

    def import_file(self):
        path,_=QFileDialog.getOpenFileName(self,"Nhập quy tắc","","JSON (*.json)")
        if not path:return
        try:
            with Path(path).open('rb') as file:payload=file.read(8193)
            if len(payload)>8192:raise ValueError("Rule file exceeds 8 KiB")
            self.set_data(json.loads(payload.decode('utf-8-sig')),force=True);self.changed()
        except (OSError,ValueError) as error:self.hint.setText(str(error))

    def export_file(self):
        try:data=self.data()
        except ValueError as error:self.hint.setText(str(error));return
        path,_=QFileDialog.getSaveFileName(self,"Xuất quy tắc","meodesk-rules.json","JSON (*.json)")
        if path:
            try:Path(path).write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8');self.hint.setText("Đã xuất quy tắc.")
            except OSError as error:self.hint.setText(str(error))
