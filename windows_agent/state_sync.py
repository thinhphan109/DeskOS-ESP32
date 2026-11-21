"""Ordering and short-lived display interpolation, independent of Qt/network."""
import time


class StateCursor:
    def __init__(self):
        self.boot = None
        self.revision = -1
        self.received_at = None

    def accept(self, state: dict) -> bool:
        if not isinstance(state, dict) or state.get("device") not in ("MEO DESK", "MEO DESK / MOCK"):
            return False
        boot, revision = state.get("state_boot_id"), state.get("state_revision")
        if isinstance(boot, str) and type(revision) is int:
            if boot == self.boot and revision < self.revision:
                return False
            self.boot, self.revision = boot, revision
        self.received_at = time.monotonic()
        return True


class FocusDisplay:
    def __init__(self):
        self.state = {}
        self.received_at = 0.0

    def update(self, state: dict):
        self.state = dict(state)
        self.received_at = time.monotonic()

    def seconds(self):
        elapsed = time.monotonic() - self.received_at
        if not self.state or elapsed >= 3.5:
            return None  # No invented countdown while disconnected.
        remaining = max(0, int(self.state.get("focus_remaining_s", 1500)))
        return max(0, remaining - int(elapsed)) if self.state.get("focus_running") else remaining


class MediaDisplay:
    def __init__(self):
        self.state = {}
        self.received_at = 0.0

    def update(self, state: dict):
        self.state = dict(state)
        self.received_at = time.monotonic()

    def position(self):
        elapsed = max(0, time.monotonic()-self.received_at)
        if (not self.state.get("available") or elapsed >= 3.5
                or self.state.get("source_age_ms",0)+elapsed*1000 >= 10000):
            return None
        position = max(0,int(self.state.get("position_ms") or 0))
        if self.state.get("playing"): position += int(elapsed*1000)
        return min(position, int(self.state.get("duration_ms") or 86400000))
