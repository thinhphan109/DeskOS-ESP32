"""Cat Pack format/transport/reboot tests without the physical ESP32."""
import hashlib
import json
import re
import struct
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT.parent / "firmware" / "tools"))
import catpack_client as cp  # noqa: E402
from make_catpack import build, NAMES  # noqa: E402


class MockResponse:
    status = 200

    def read(self, _max):
        return b'{"ok":true,"verified":true,"rebooting":true}'


class MockConnection:
    last = None

    def __init__(self, host, port, timeout):
        self.host, self.port, self.timeout = host, port, timeout
        self.headers = {}
        self.parts = []
        MockConnection.last = self

    def putrequest(self, method, path):
        self.method, self.path = method, path

    def putheader(self, name, value):
        self.headers[name] = value

    def endheaders(self):
        pass

    def send(self, data):
        self.parts.append(data)

    def getresponse(self):
        return MockResponse()

    def close(self):
        self.closed = True


class CatPackTests(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.path = Path(temp.name) / "cats.meocat"
        self.content = b"\x9e\xff" * (cp.PACK_BYTES // 2)
        self.path.write_bytes(self.content)
        self.sha = hashlib.sha256(self.content).hexdigest()

    def test_pack_validation_exact_bytes_and_sha(self):
        self.assertEqual(cp.validate_pack(self.path), (663552, self.sha))
        self.path.write_bytes(self.content[:-2])
        with self.assertRaisesRegex(ValueError, "648 KiB"):
            cp.validate_pack(self.path)

    def test_pack_builder_uses_all_16_pngs_in_stable_order(self):
        result = build(ROOT.parent / "firmware" / "images", self.path)
        self.assertEqual(result["frames"], list(NAMES))
        self.assertEqual(result["size"], cp.PACK_BYTES)
        self.assertEqual(cp.validate_pack(self.path)[1], result["sha256"])
        manifest = json.loads(self.path.with_suffix(".manifest.json").read_text())
        self.assertIsNone(manifest["signature"])
        self.assertNotIn("pairing_key", manifest)
        # First compiled image must be bit-for-bit identical to the proven
        # built-in RGB565 source, including red/blue channel order.
        compiled = (ROOT.parent / "firmware" / "src" / "cat_assets.cpp").read_text()
        match = re.search(r"const uint16_t cat_boot\[20736\] PROGMEM = \{(.*?)\};",
                          compiled, re.DOTALL)
        self.assertIsNotNone(match)
        values = [int(token, 16) for token in re.findall(r"0x([0-9A-Fa-f]{4})", match.group(1))]
        self.assertEqual(len(values), 144 * 144)
        self.assertEqual(self.path.read_bytes()[:144 * 144 * 2],
                         struct.pack("<" + "H" * len(values), *values))

    def test_motion_demo_generates_48_frames_pose_major(self):
        build(ROOT.parent / "firmware" / "images", self.path)
        v1 = self.path.read_bytes()
        manifest = build(ROOT.parent / "firmware" / "images", self.path, motion_demo=True)
        self.assertEqual(manifest["schema"], 2)
        self.assertEqual(manifest["frames_per_pose"], 3)
        self.assertTrue(manifest["motion_demo"])
        self.assertEqual(manifest["frames"][:4], ["boot_0", "boot_1", "boot_2", "clock_0"])
        self.assertEqual(manifest["size"], cp.MOTION_BYTES)
        self.assertEqual(cp.validate_pack(self.path)[0], cp.MOTION_BYTES)
        frame_bytes = 144 * 144 * 2
        animation = self.path.read_bytes()
        # Motion demo starts with the exact unchanged v1 boot pose; the
        # shifted second cel is actually different, not three duplicates.
        self.assertEqual(animation[:frame_bytes], v1[:frame_bytes])
        self.assertNotEqual(animation[:frame_bytes], animation[frame_bytes:2 * frame_bytes])

    def test_custom_motion_requires_all_48_frames(self):
        with self.assertRaises(FileNotFoundError):
            build(ROOT.parent / "firmware" / "images", self.path, motion=True)

    def test_streaming_upload_headers_and_size(self):
        status = {"board": cp.BOARD, "ready": True, "pack_bytes": cp.PACK_BYTES, "phase": "idle"}
        points = []
        with patch.object(cp, "read_status", return_value=status), \
             patch.object(cp.http.client, "HTTPConnection", MockConnection):
            result = cp.upload_pack("192.168.0.107", "pairing-secret", self.path,
                                    lambda sent, total: points.append((sent, total)))
        conn = MockConnection.last
        self.assertEqual((conn.method, conn.path), ("POST", "/api/v1/catpack/upload"))
        self.assertEqual(conn.headers["X-MEO-CAT-SIZE"], str(cp.PACK_BYTES))
        self.assertEqual(conn.headers["X-MEO-CAT-SHA256"], self.sha)
        self.assertEqual(conn.headers["X-MEO-KEY"], "pairing-secret")
        self.assertEqual(conn.headers["Content-Length"], sum(len(b) for b in conn.parts))
        self.assertEqual(result["expected_sha256"], self.sha)
        self.assertEqual(points[-1], (cp.PACK_BYTES, cp.PACK_BYTES))
        self.assertTrue(conn.closed)

    def test_motion_v2_upload_uses_1944_kib_and_capability_check(self):
        self.path.write_bytes(b"\x04\xe0" * (cp.MOTION_BYTES // 2))
        status = {"board": cp.BOARD, "ready": True,
                  "accepted_pack_bytes": [cp.PACK_BYTES, cp.MOTION_BYTES], "phase": "idle"}
        with patch.object(cp, "read_status", return_value=status), \
             patch.object(cp.http.client, "HTTPConnection", MockConnection):
            result = cp.upload_pack("192.168.0.107", "pairing-secret", self.path)
        self.assertEqual(MockConnection.last.headers["X-MEO-CAT-SIZE"], str(cp.MOTION_BYTES))
        self.assertEqual(result["expected_sha256"], cp.validate_pack(self.path)[1])
        # Older v1.5 firmware advertises only a single supported size.
        with patch.object(cp, "read_status", return_value={"board": cp.BOARD, "ready": True,
                          "pack_bytes": cp.PACK_BYTES, "phase": "idle"}):
            with self.assertRaisesRegex(ValueError, "v1.6"):
                cp.upload_pack("192.168.0.107", "pairing-secret", self.path)

    def test_upload_refuses_psram_unavailable(self):
        with patch.object(cp, "read_status", return_value={"board": cp.BOARD,
                          "ready": False, "pack_bytes": cp.PACK_BYTES, "phase": "idle"}):
            with self.assertRaisesRegex(ValueError, "Flash/PSRAM"):
                cp.upload_pack("192.168.0.107", "key", self.path)

    def test_reboot_confirms_new_pack_sha(self):
        fake_clock = [0.0]
        responses = iter([OSError("reboot"), {"source": "builtin", "sha256": ""},
                          {"source": "custom", "sha256": self.sha}])

        def read(_host):
            r = next(responses)
            if isinstance(r, Exception):
                raise r
            return r

        with patch.object(cp, "read_status", side_effect=read), \
             patch.object(cp.time, "monotonic", side_effect=lambda: fake_clock[0]), \
             patch.object(cp.time, "sleep", side_effect=lambda s: fake_clock.__setitem__(0, fake_clock[0] + s)):
            result = cp.wait_for_pack("192.168.0.107", self.sha, interval_s=1.0)
        self.assertEqual(result["sha256"], self.sha)

    def test_reboot_times_out_if_old_pack_remains(self):
        fake_clock = [0.0]
        with patch.object(cp, "read_status", return_value={"source": "custom", "sha256": "0" * 64}), \
             patch.object(cp.time, "monotonic", side_effect=lambda: fake_clock[0]), \
             patch.object(cp.time, "sleep", side_effect=lambda s: fake_clock.__setitem__(0, fake_clock[0] + s)):
            with self.assertRaises(TimeoutError):
                cp.wait_for_pack("192.168.0.107", self.sha, timeout_s=2.5, interval_s=1.0)


if __name__ == "__main__":
    unittest.main()
