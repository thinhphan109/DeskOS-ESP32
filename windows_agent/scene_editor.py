"""Native editor for the firmware's bounded widget scene."""
from __future__ import annotations
import copy
from PySide6.QtCore import Qt, Signal, QRectF
from PySide6.QtGui import QColor, QPainter, QPen
from PySide6.QtWidgets import (QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel,
                              QLineEdit, QPushButton, QFileDialog)
from ui_controls import ScrollSafeComboBox, ScrollSafeSpinBox
from scene_layout import DEFAULT_LAYOUT, KINDS, validate_layout, load_layout, save_layout


class SceneCanvas(QWidget):
    selected = Signal(int)
    moved = Signal(int, int, int)

    def __init__(self, parent=None):
        super().__init__(parent)
        self.layout_data = copy.deepcopy(DEFAULT_LAYOUT)
        self.index = 0
        self.drag = None
        self.setFixedSize(324, 240)
        self.setAccessibleName("Scene canvas · 162 × 120 · kéo để di chuyển widget")

    def paintEvent(self, event):
        painter = QPainter(self)
        painter.fillRect(self.rect(), QColor("#f7f2e9"))
        for index, item in enumerate(self.layout_data["widgets"]):
            rect = QRectF(item["x"]*2+1, item["y"]*2+1, item["w"]*2-2, item["h"]*2-2)
            painter.setBrush(QColor("#d8f1e2" if index == self.index else "#fffefa"))
            painter.setPen(QPen(QColor("#315a46" if index == self.index else "#bcb7ac"), 2 if index == self.index else 1))
            painter.drawRoundedRect(rect, 6, 6)
            painter.setClipRect(rect.adjusted(5, 3, -5, -3))
            painter.setPen(QColor("#314438"))
            painter.drawText(rect.adjusted(8, 5, -5, -5), Qt.AlignmentFlag.AlignLeft | Qt.AlignmentFlag.AlignTop,
                             item["label"] or KINDS[item["kind"]])
            painter.setClipping(False)

    def mousePressEvent(self, event):
        if event.button() != Qt.MouseButton.LeftButton:
            return
        point = event.position()/2
        for index, item in enumerate(self.layout_data["widgets"]):
            if QRectF(item["x"],item["y"],item["w"],item["h"]).contains(point):
                self.index = index
                self.drag = (point.x()-item["x"], point.y()-item["y"])
                self.selected.emit(index)
                self.update()
                return

    def mouseMoveEvent(self, event):
        if self.drag is None:
            return
        item = self.layout_data["widgets"][self.index]
        x = min(162-item["w"], max(0, round((event.position().x()/2-self.drag[0])/2)*2))
        y = min(120-item["h"], max(0, round((event.position().y()/2-self.drag[1])/2)*2))
        self.moved.emit(self.index, x, y)

    def mouseReleaseEvent(self, event):
        self.drag = None


class SceneEditor(QWidget):
    apply_requested = Signal(dict)
    reload_requested = Signal()

    def __init__(self, parent=None):
        super().__init__(parent)
        self.data = copy.deepcopy(DEFAULT_LAYOUT)
        self.dirty = False
        self.loading = False
        box = QVBoxLayout(self)
        box.setContentsMargins(0, 0, 0, 0)
        title = QLabel("Scene Studio")
        title.setObjectName("cardTitle")
        box.addWidget(title)
        note = QLabel("Kéo widget hoặc nhập tọa độ. Đây là bố cục vùng nội dung TFT; mèo giữ vùng riêng.")
        note.setWordWrap(True)
        box.addWidget(note)
        body = QHBoxLayout()
        self.canvas = SceneCanvas()
        body.addWidget(self.canvas, 0, Qt.AlignmentFlag.AlignTop)
        form = QGridLayout()
        self.selection = ScrollSafeComboBox()
        self.kind = ScrollSafeComboBox()
        self.kind.addItems(KINDS)
        self.label = QLineEdit()
        self.label.setMaxLength(31)
        self.label.setPlaceholderText("Nhãn TFT dùng ASCII")
        form.addWidget(QLabel("Widget"),0,0); form.addWidget(self.selection,0,1)
        form.addWidget(QLabel("Loại"),1,0); form.addWidget(self.kind,1,1)
        form.addWidget(QLabel("Nhãn"),2,0); form.addWidget(self.label,2,1)
        self.fields = {}
        for row, (key, caption, low, high) in enumerate((("x","X",0,161),("y","Y",0,119),("w","Rộng",32,162),("h","Cao",20,120)),3):
            field = ScrollSafeSpinBox(); field.setRange(low, high); field.setAccessibleName(caption)
            self.fields[key] = field
            form.addWidget(QLabel(caption),row,0); form.addWidget(field,row,1)
            field.valueChanged.connect(self.edit)
        body.addLayout(form,1)
        box.addLayout(body)
        row = QHBoxLayout()
        for caption, action in (("Thêm",self.add),("Xóa",self.remove),("Mặc định",self.reset),
                                ("Nhập JSON",self.import_file),("Xuất JSON",self.export_file)):
            control = QPushButton(caption); control.setObjectName("secondary")
            control.clicked.connect(action); row.addWidget(control)
        box.addLayout(row)
        actions = QHBoxLayout()
        reload_button = QPushButton("Tải lại từ thiết bị")
        reload_button.setObjectName("secondary")
        reload_button.clicked.connect(self.reload_requested.emit)
        self.apply_button = QPushButton("Lưu bố cục lên TFT")
        self.apply_button.setObjectName("primary")
        self.apply_button.clicked.connect(self.apply)
        actions.addWidget(reload_button); actions.addWidget(self.apply_button)
        box.addLayout(actions)
        self.hint = QLabel("Bản nháp cục bộ · chưa áp dụng")
        self.hint.setWordWrap(True); box.addWidget(self.hint)
        self.selection.currentIndexChanged.connect(self.select)
        self.kind.currentIndexChanged.connect(self.edit)
        self.label.textEdited.connect(self.edit)
        self.canvas.selected.connect(self.selection.setCurrentIndex)
        self.canvas.moved.connect(self.move)
        self.set_data(self.data)

    def set_data(self, data, force=False):
        if self.dirty and not force:
            self.hint.setText("Thiết bị có bố cục mới. Bản nháp được giữ; dùng Tải lại để nhận thay đổi.")
            return
        self.data = validate_layout(data)
        self.dirty = False
        self.refresh(0)
        self.check()
        self.hint.setText("Bố cục đã tải · vùng 162 × 120 · tối đa 6 widget")

    def refresh(self, index):
        self.loading = True
        self.selection.clear()
        self.selection.addItems([f"{i+1}. {w['label'] or KINDS[w['kind']]}" for i,w in enumerate(self.data["widgets"])])
        self.selection.setCurrentIndex(min(index,len(self.data["widgets"])-1))
        self.loading = False
        self.select(self.selection.currentIndex())

    def select(self, index):
        if self.loading or index < 0:
            return
        self.loading = True
        item = self.data["widgets"][index]
        self.kind.setCurrentIndex(item["kind"]); self.label.setText(item["label"])
        for key, field in self.fields.items(): field.setValue(item[key])
        self.canvas.layout_data = self.data; self.canvas.index = index; self.canvas.update()
        self.loading = False

    def edit(self, *args):
        if self.loading or self.selection.currentIndex() < 0:
            return
        item = self.data["widgets"][self.selection.currentIndex()]
        item.update(kind=self.kind.currentIndex(), label=self.label.text(), **{key:field.value() for key,field in self.fields.items()})
        self.selection.setItemText(self.selection.currentIndex(),f"{self.selection.currentIndex()+1}. {item['label'] or KINDS[item['kind']]}")
        self.dirty = True; self.canvas.update()
        self.check()

    def check(self):
        try:
            validate_layout(self.data)
        except ValueError as error:
            self.hint.setText(str(error)); self.apply_button.setEnabled(False); return False
        self.hint.setText("Bản nháp hợp lệ · chưa lưu lên thiết bị" if self.dirty else "Bố cục hợp lệ")
        self.apply_button.setEnabled(True); return True

    def move(self, index, x, y):
        candidate = copy.deepcopy(self.data)
        candidate["widgets"][index].update(x=x,y=y)
        try: validate_layout(candidate)
        except ValueError: return
        self.data = candidate; self.dirty = True; self.select(index); self.check()

    def add(self):
        if len(self.data["widgets"]) >= 6:
            self.hint.setText("Đã đủ 6 widget."); return
        for y in range(0,101,2):
            for x in range(0,131,2):
                candidate = copy.deepcopy(self.data)
                candidate["widgets"].append(dict(kind=7,x=x,y=y,w=32,h=20,label="Text"))
                try: validate_layout(candidate)
                except ValueError: continue
                self.data = candidate; self.dirty = True; self.refresh(len(candidate["widgets"])-1); self.check(); return
        self.hint.setText("Chưa có chỗ trống 32 × 20. Thu nhỏ hoặc xóa một widget.")

    def remove(self):
        if len(self.data["widgets"]) > 1:
            self.data["widgets"].pop(self.selection.currentIndex())
            self.dirty = True; self.refresh(0); self.check()

    def reset(self):
        self.set_data(DEFAULT_LAYOUT,force=True); self.dirty = True; self.check()

    def apply(self):
        if self.check(): self.apply_requested.emit(validate_layout(self.data))

    def import_file(self):
        path,_ = QFileDialog.getOpenFileName(self,"Nhập bố cục","","JSON (*.json)")
        if not path: return
        try: self.set_data(load_layout(path),force=True); self.dirty=True; self.check()
        except (OSError, ValueError) as error: self.hint.setText(str(error))

    def export_file(self):
        if not self.check(): return
        path,_ = QFileDialog.getSaveFileName(self,"Xuất bố cục","meodesk-scene.json","JSON (*.json)")
        if path:
            try: save_layout(path,self.data); self.hint.setText("Đã xuất bố cục.")
            except OSError as error: self.hint.setText(str(error))
