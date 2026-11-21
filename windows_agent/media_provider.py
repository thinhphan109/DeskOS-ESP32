"""Optional Windows System Media Transport Controls (SMTC) adapter.

Missing WinRT bindings, no active music or inaccessible media sessions are
normal conditions: they produce an honest unavailable state, never demo data.
"""
from __future__ import annotations

import asyncio
import io
import time
import unicodedata
from typing import Any


def unavailable() -> dict[str, Any]:
    return {"available": False, "title": "", "artist": "", "album": "",
            "playing": False, "position_ms": 0, "duration_ms": 0, "source": "windows_smtc"}


async def _get_session():
    try:
        from winrt.windows.media.control import GlobalSystemMediaTransportControlsSessionManager
    except ImportError:
        # Support older installations still using python-winsdk namespace.
        from winsdk.windows.media.control import GlobalSystemMediaTransportControlsSessionManager

    manager = await GlobalSystemMediaTransportControlsSessionManager.request_async()
    return manager.get_current_session()


async def _snapshot() -> dict[str, Any]:
    session = await _get_session()
    if session is None:
        return unavailable()
    props = await session.try_get_media_properties_async()
    playback = session.get_playback_info()
    timeline = session.get_timeline_properties()
    # WinRT TimeSpan values may be timedelta or integer ticks, depending on binding.
    def millis(value) -> int:
        if hasattr(value, "total_seconds"):
            return max(0, round(value.total_seconds() * 1000))
        return max(0, int(value) // 10000)

    start = millis(timeline.start_time) if timeline else 0
    end = millis(timeline.end_time) if timeline else 0
    position = millis(timeline.position) if timeline else 0
    # WinRT PlaybackStatus.PLAYING == 4. IntEnum.__str__ may just be '4', so
    # string matching for the word 'playing' silently misreports every track!
    is_playing = playback is not None and int(playback.playback_status) == 4
    return {"available": True, "title": unicodedata.normalize("NFC", str(props.title or ""))[:95],
            "artist": unicodedata.normalize("NFC", str(props.artist or ""))[:75],
            "album": unicodedata.normalize("NFC", str(props.album_title or ""))[:75],
            "playing": is_playing,
            "position_ms": max(0, position - start), "duration_ms": max(0, end - start),
            "source": "windows_smtc", "sampled_at": int(time.time())}


async def _control(action: str) -> bool:
    session = await _get_session()
    if session is None:
        return False
    operations = {"play_pause": session.try_toggle_play_pause_async,
                  "next": session.try_skip_next_async,
                  "previous": session.try_skip_previous_async}
    if action not in operations:
        return False
    return bool(await operations[action]())


def snapshot() -> dict[str, Any]:
    try:
        return asyncio.run(_snapshot())
    except Exception:  # Optional Windows/COM provider must never stop CPU telemetry.
        return unavailable()


def control(action: str) -> bool:
    if action not in {"play_pause", "next", "previous"}:
        return False
    try:
        return asyncio.run(_control(action))
    except Exception:
        return False


async def _thumbnail_bytes(expected_title: str, expected_artist: str) -> bytes | None:
    """Read the current SMTC thumbnail only when it matches a known track.

    Some players provide no thumbnail; that is a normal unavailable state.
    Limit input to 1 MiB so an unexpected stream cannot exhaust the agent.
    """
    session = await _get_session()
    if session is None:
        return None
    props = await session.try_get_media_properties_async()
    if (unicodedata.normalize("NFC", str(props.title or ""))[:95] != expected_title or
            unicodedata.normalize("NFC", str(props.artist or ""))[:75] != expected_artist or
            props.thumbnail is None):
        return None
    stream = await props.thumbnail.open_read_async()
    length = int(stream.size)
    if length <= 0 or length > 1024 * 1024:
        return None
    try:
        from winrt.windows.storage.streams import DataReader
    except ImportError:
        from winsdk.windows.storage.streams import DataReader
    reader = DataReader(stream)
    count = int(await reader.load_async(length))
    if count <= 0 or count > length:
        return None
    buffer = bytearray(count)
    reader.read_bytes(buffer)
    return bytes(buffer)


def artwork_rgb565(snapshot: dict[str, Any]) -> bytes | None:
    """Windows thumbnail -> exact 64x64 little-endian RGB565 frame.

    No synthetic fallback. Missing WinRT/Pillow, malformed image or a track
    change during extraction simply means no new cover is sent.
    """
    if not snapshot.get("available") or not snapshot.get("title"):
        return None
    try:
        from PIL import Image, ImageOps
        raw = asyncio.run(_thumbnail_bytes(str(snapshot.get("title", "")),
                                           str(snapshot.get("artist", ""))))
        if not raw:
            return None
        with Image.open(io.BytesIO(raw)) as original:
            reduced = ImageOps.fit(original.convert("RGB"), (64, 64),
                                   method=Image.Resampling.LANCZOS)
            pixels = bytearray(64 * 64 * 2)
            for index, (r, g, b) in enumerate(reduced.getdata()):
                value = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                pixels[index * 2] = value & 255
                pixels[index * 2 + 1] = value >> 8
            return bytes(pixels)
    except Exception:
        return None
