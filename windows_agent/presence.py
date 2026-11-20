"""Session-local idle duration only; never captures keys or window titles."""
from __future__ import annotations
import ctypes
import sys


def idle_from_ticks(now: int, last: int) -> int | None:
    delta = (now-last) & 0xFFFFFFFF
    # Windows can report an input tick slightly ahead of the sampled clock.
    return min(604800, delta//1000) if delta < 0x80000000 else None


def read_idle_seconds() -> int | None:
    if sys.platform != "win32": return None
    class LastInput(ctypes.Structure):
        _fields_ = [("cbSize",ctypes.c_uint32),("dwTime",ctypes.c_uint32)]
    try:
        info = LastInput(); info.cbSize = ctypes.sizeof(info)
        user32 = ctypes.WinDLL("user32",use_last_error=True)
        kernel32 = ctypes.WinDLL("kernel32",use_last_error=True)
        user32.GetLastInputInfo.argtypes = [ctypes.POINTER(LastInput)]
        user32.GetLastInputInfo.restype = ctypes.c_int
        kernel32.GetTickCount.restype = ctypes.c_uint32
        if not user32.GetLastInputInfo(ctypes.byref(info)): return None
        return idle_from_ticks(kernel32.GetTickCount(),info.dwTime)
    except (OSError, AttributeError):
        return None
