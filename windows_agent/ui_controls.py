"""Inputs inside scrolling pages must not change values on mouse-wheel scroll."""
from PySide6.QtCore import Qt
from PySide6.QtGui import QWheelEvent
from PySide6.QtWidgets import QApplication, QAbstractScrollArea, QComboBox, QSpinBox


def _scroll_parent(control, event):
    parent = control.parentWidget()
    while parent is not None:
        if isinstance(parent, QAbstractScrollArea):
            viewport = parent.viewport()
            forwarded = QWheelEvent(viewport.mapFromGlobal(event.globalPosition().toPoint()),
                                    event.globalPosition(), event.pixelDelta(), event.angleDelta(),
                                    event.buttons(), event.modifiers(), event.phase(), event.inverted(),
                                    event.source(), event.pointingDevice())
            forwarded.setTimestamp(event.timestamp())
            QApplication.sendEvent(viewport, forwarded)
            # Consume the original so native propagation cannot scroll a second time.
            event.accept()
            return
        parent = parent.parentWidget()
    event.ignore()


class ScrollSafeComboBox(QComboBox):
    """Keep closed selections stable; the open popup retains normal scrolling."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def wheelEvent(self, event):
        _scroll_parent(self, event)


class ScrollSafeSpinBox(QSpinBox):
    """Use buttons or keys to edit numbers; pass the wheel to the page."""
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)

    def wheelEvent(self, event):
        _scroll_parent(self, event)
