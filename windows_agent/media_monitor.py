"""Local music remains available even when no ESP32 is configured or reachable."""
from __future__ import annotations

import threading
import media_provider
from companion_core import media_artwork_id


class MediaMonitor:
    def __init__(self, on_update, interval=2.0):
        self.on_update = on_update
        self.interval = interval
        self._stop = threading.Event()
        self._lock = threading.Lock()
        self._thread = None
        self._song = media_provider.unavailable("starting")
        self._pixels = None
        self._song_id = ""

    def snapshot(self):
        with self._lock:
            return dict(self._song), self._pixels

    def start(self):
        if self._thread and self._thread.is_alive():
            return
        self._stop.clear()
        self._thread = threading.Thread(target=self._loop, name="MimiDeskMusic", daemon=True)
        self._thread.start()

    def stop(self):
        self._stop.set()
        if self._thread and self._thread is not threading.current_thread():
            self._thread.join(timeout=2.5)

    def _emit(self, event):
        if not self._stop.is_set():
            self.on_update(event)

    def _sample(self):
        song = media_provider.snapshot()
        if self._stop.is_set():
            return
        song_id = media_artwork_id(song)
        changed = song_id != self._song_id
        # Publish metadata before reading the optional artwork.
        with self._lock:
            self._song = dict(song)
            if changed:
                self._pixels = None
        self._emit({"type": "media", "data": song, "source": "local"})
        if changed:
            self._emit({"type": "artwork", "pixels": None, "id": song_id, "source": "local"})
            if self._stop.is_set():
                return
            pixels = media_provider.artwork_rgb565(song) if song_id else None
            if self._stop.is_set():
                return
            with self._lock:
                self._pixels = pixels
            if pixels is not None:
                self._emit({"type": "artwork", "pixels": pixels, "id": song_id, "source": "local"})
            self._song_id = song_id

    def _loop(self):
        while not self._stop.is_set():
            self._sample()
            self._stop.wait(self.interval)
