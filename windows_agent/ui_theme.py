"""mimi.desk desktop visual language; transport/config identifiers stay stable."""
from PySide6.QtCore import QSize, Qt
from PySide6.QtGui import QIcon, QPainter, QPixmap
from PySide6.QtSvg import QSvgRenderer
from PySide6.QtWidgets import QFrame, QPushButton, QStackedLayout, QVBoxLayout, QWidget

QSS = """
QMainWindow,QWidget#root,QWidget#content,QWidget#tabPage,QStackedWidget,QScrollArea {
  background:#f6f5ef; color:#263c32; }
QWidget { font-family:'Segoe UI'; font-size:13px; }
QLabel { color:#263c32; background:transparent; }
QFrame#sidebar { background:#eceee5; border-right:1px solid #dce1d5; }
QLabel#brand { color:#244c3a; font-size:24px; font-weight:750; }
QLabel#sideSmall { color:#58685d; font-size:11px; }
QPushButton#nav { text-align:left; background:transparent; border:0; border-radius:9px;
  padding:12px 14px; color:#53675a; font-size:13px; font-weight:600; }
QPushButton#nav:hover { background:#e0e7d8; color:#254b36; }
QPushButton#nav[chosen="true"] { background:#d2e3c5; color:#244c32; }
QLabel#title { font-size:27px; font-weight:700; color:#243e30; }
QLabel#subtitle { font-size:13px; color:#596b5e; }
QLabel#eyebrow,QLabel#smallLabel { color:#566b5b; font-size:12px; }
QLabel#pageTrail { color:#627363; font-size:12px; }
QLabel#connectionBadge { background:#e5e9de; color:#586354; border-radius:8px; padding:7px 12px; font-size:12px; }
QLabel#connectionBadge[live="true"] { background:#d9eacb; color:#2c5637; }
QFrame#card,QFrame#disclosure { background:#fffef9; border:1px solid #e0e5d9; border-radius:14px; }
QFrame#mintCard { background:#e1ebd4; border:0; border-radius:14px; }
QFrame#focusCard { background:#eee8dc; border:0; border-radius:14px; }
QFrame#darkCard { background:#294b3b; border-radius:14px; }
QLabel#bigNumber { font-size:46px; font-weight:650; color:#254934; }
QLabel#darkNumber { font-size:27px; font-weight:650; color:#f4f8eb; }
QLabel#darkMeta { font-size:12px; color:#d3e2cc; }
QLabel#cardTitle { font-size:16px; font-weight:650; }
QLabel#focusClock { font-size:62px; font-weight:650; color:#584b37; }
QLabel#albumCover { background:#e0e8d6; color:#4f6552; border-radius:12px; font-size:13px; }
QFrame#musicPanel { background:#244535; border-radius:14px; }
QLabel#musicSong { color:#f8f9ef; font-size:32px; font-weight:700; }
QLabel#musicArtist { color:#d6e3ca; font-size:18px; }
QLabel#musicState { color:#d6e3ca; font-size:13px; }
QPushButton#primary { background:#315b40; color:#ffffff; border:1px solid #315b40; border-radius:8px;
  padding:10px 16px; font-weight:600; }
QPushButton#primary:hover { background:#254b32; }
QPushButton#secondary { background:#fffef9; color:#31563b; border:1px solid #cbd6c4; border-radius:8px; padding:10px 15px; font-weight:600; }
QPushButton#secondary:hover { background:#edf3e5; border-color:#91a77d; }
QPushButton#disclosureToggle { background:transparent; text-align:left; border:0; padding:12px 14px; color:#405b45; font-weight:600; }
QPushButton:checked { background:#dceacb; color:#264d32; border:1px solid #6b8c56; }
QPushButton:disabled { background:#e8ebe2; color:#727c6b; border-color:#e0e5d9; }
QPushButton:focus,QComboBox:focus,QLineEdit:focus,QSpinBox:focus { border:2px solid #668755; }
QLineEdit,QSpinBox,QComboBox { background:#fffef9; color:#294534; border:1px solid #c9d3c1;
  border-radius:8px; padding:9px 11px; selection-background-color:#c7ddb4; }
QLineEdit[echoMode="2"] { font-family:'Segoe UI'; }
QComboBox QAbstractItemView { background:#fffef9; color:#294534; selection-background-color:#deebd2; }
QListWidget { background:#fffef9; color:#294534; border:1px solid #d5decf; border-radius:8px; padding:6px; }
QListWidget::item { padding:10px; border-radius:6px; }
QListWidget::item:selected { background:#dce9d0; }
QCheckBox { spacing:9px; color:#405644; }
QProgressBar { background:#e0e6d7; border:0; border-radius:4px; height:8px; }
QProgressBar::chunk { background:#6d8c55; border-radius:4px; }
QFrame#musicPanel QProgressBar { background:#52694e; }
QFrame#musicPanel QProgressBar::chunk { background:#b2d993; }
QScrollBar:vertical { background:#f0f1e9; width:9px; margin:2px; }
QScrollBar::handle:vertical { background:#bdc9b4; border-radius:4px; min-height:30px; }
QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical { height:0; }
QToolTip { background:#294b3b; color:white; border:0; padding:6px; }
"""


def nav_icon(name: str) -> QIcon:
    paths = {
        'home':'<path d="M3 11 12 3l9 8M6 10v10h12V10M10 20v-6h4v6"/>',
        'device':'<rect x="5" y="3" width="14" height="18" rx="3"/><path d="M9 17h6"/>',
        'music':'<path d="M9 17V5l11-2v12M9 7l11-2"/><ellipse cx="6" cy="17" rx="3" ry="2"/><ellipse cx="17" cy="15" rx="3" ry="2"/>',
        'focus':'<circle cx="12" cy="13" r="8"/><path d="M12 9v5l3 2M9 2h6"/>',
        'settings':'<path d="M4 6h16M4 12h16M4 18h16"/><circle cx="8" cy="6" r="2"/><circle cx="16" cy="12" r="2"/><circle cx="10" cy="18" r="2"/>',
        'update':'<path d="M12 16V3m-5 5 5-5 5 5M4 15v6h16v-6"/>',
        'studio':'<rect x="3" y="4" width="18" height="16" rx="2"/><path d="M10 4v16M10 11h11"/>',
    }
    source=f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="none" stroke="#44624b" stroke-width="1.7" stroke-linecap="round" stroke-linejoin="round">{paths[name]}</svg>'
    pixmap=QPixmap(40,40); pixmap.fill(Qt.GlobalColor.transparent)
    painter=QPainter(pixmap); QSvgRenderer(source.encode()).render(painter); painter.end()
    return QIcon(pixmap)


class _CurrentPageLayout(QStackedLayout):
    """Measure only the visible page, including wrapped text's height for width."""
    def sizeHint(self):
        return self.currentWidget().sizeHint() if self.currentWidget() else QSize(600,400)

    def minimumSize(self):
        page = self.currentWidget()
        return page.minimumSizeHint().expandedTo(page.minimumSize()) if page else QSize(0,0)

    def hasHeightForWidth(self):
        return bool(self.currentWidget() and self.currentWidget().hasHeightForWidth())

    def heightForWidth(self, width):
        page = self.currentWidget()
        if page is None:
            return -1
        return page.heightForWidth(width) if page.hasHeightForWidth() else page.sizeHint().height()


class ContentStack(QWidget):
    """Keep inactive editors alive without adding their height to other pages."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self._pages = _CurrentPageLayout(self)
        self._pages.setContentsMargins(0, 0, 0, 0)
        self._pages.currentChanged.connect(self.updateGeometry)

    def addWidget(self, widget):
        return self._pages.addWidget(widget)

    def setCurrentIndex(self, index):
        self._pages.setCurrentIndex(index)

    def currentIndex(self):
        return self._pages.currentIndex()

    def currentWidget(self):
        return self._pages.currentWidget()

    def count(self):
        return self._pages.count()

    def widget(self, index):
        return self._pages.widget(index)


class Disclosure(QFrame):
    def __init__(self, title: str, parent=None):
        super().__init__(parent)
        self.setObjectName('disclosure')
        layout=QVBoxLayout(self); layout.setContentsMargins(0,0,0,0); layout.setSpacing(0)
        self.toggle=QPushButton(title); self.toggle.setObjectName('disclosureToggle'); self.toggle.setCheckable(True)
        self.toggle.setAccessibleName(title); self.toggle.setToolTip('Bấm để mở hoặc thu gọn')
        self.body=QWidget(); self.content=QVBoxLayout(self.body); self.content.setContentsMargins(14,0,14,14)
        self.body.setVisible(False)
        self.toggle.toggled.connect(self.body.setVisible)
        layout.addWidget(self.toggle); layout.addWidget(self.body)
