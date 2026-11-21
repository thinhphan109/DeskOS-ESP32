"""One agent per Windows user; preview has its own independent instance."""
from __future__ import annotations

import hashlib
import tempfile
from pathlib import Path
from PySide6.QtCore import QObject, QLockFile, Signal, QThread
from PySide6.QtNetwork import QLocalServer, QLocalSocket


class SingleInstance(QObject):
    activated = Signal()

    def __init__(self, preview=False, *, name=None, directory=None):
        super().__init__()
        user = hashlib.sha256(str(Path.home()).encode()).hexdigest()[:16]
        self.name = name or f"mimi-desk-{user}-{'preview' if preview else 'live'}"
        self.lock = QLockFile(str(Path(directory or tempfile.gettempdir()) / (self.name + ".lock")))
        self.lock.setStaleLockTime(0)
        self.server = QLocalServer(self)
        self.server.setSocketOptions(QLocalServer.SocketOption.UserAccessOption)
        self.server.newConnection.connect(self._activate)
        self.owned = False

    def acquire(self):
        if not self.lock.tryLock(0):
            if self.lock.error() != QLockFile.LockError.LockFailedError:
                raise RuntimeError("Không tạo được phiên ứng dụng. Kiểm tra quyền truy cập thư mục tạm.")
            for _ in range(20):
                socket = QLocalSocket()
                socket.connectToServer(self.name)
                if socket.waitForConnected(100):
                    socket.disconnectFromServer()
                    return False
                QThread.msleep(50)
            raise RuntimeError("mimi.desk đang chạy nhưng chưa phản hồi. Hãy thoát bản đang mở rồi thử lại.")
        # The lock excludes a competing owner while cleaning up an abandoned endpoint.
        QLocalServer.removeServer(self.name)
        if not self.server.listen(self.name):
            self.lock.unlock()
            raise RuntimeError("Chưa khởi tạo được ứng dụng. Hãy thử mở lại mimi.desk.")
        self.owned = True
        return True

    def _activate(self):
        while self.server.hasPendingConnections():
            socket = self.server.nextPendingConnection()
            socket.close()
            socket.deleteLater()
            self.activated.emit()

    def release(self):
        if self.owned:
            self.server.close()
            self.lock.unlock()
            self.owned = False
