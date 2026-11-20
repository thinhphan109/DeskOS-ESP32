"""Offline tests for the real streaming upload protocol; no ESP32 required."""

import hashlib
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import ota_client  # noqa: E402
sys.path.insert(0, str(ROOT.parent / "firmware" / "tools"))
from make_ota_manifest import describe  # noqa: E402


class MockResponse:
    status = 200

    def read(self, maximum: int) -> bytes:
        return b'{"ok":true,"verified":true,"rebooting":true}'


class MockConnection:
    latest = None

    def __init__(self, host, port, timeout):
        self.host = host
        self.port = port
        self.headers = {}
        self.parts = []
        self.closed = False
        MockConnection.latest = self

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


class OtaClientTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.firmware = Path(self.temp.name) / "firmware.bin"
        image = bytearray(b"\xe9" + b"m" * 9000)
        image[12:14] = b"\x09\x00"  # ESP-IDF chip_id ESP32-S3
        self.image = bytes(image)
        self.firmware.write_bytes(self.image)

    def test_sha256_and_image_validation(self):
        length, digest = ota_client.validate_image(self.firmware)
        self.assertEqual(length, len(self.image))
        self.assertEqual(digest, hashlib.sha256(self.image).hexdigest())
        self.firmware.write_bytes(b"x" + b"\0" * 8000)
        with self.assertRaisesRegex(ValueError, "ESP32"):
            ota_client.validate_image(self.firmware)

    def test_manifest_cannot_claim_signed_firmware(self):
        manifest = describe(self.firmware, "1.4.0")
        self.assertEqual(manifest["board"], ota_client.BOARD_ID)
        self.assertEqual(manifest["sha256"], hashlib.sha256(self.image).hexdigest())
        self.assertIsNone(manifest["signature"])
        self.assertNotIn("pairing_key", manifest)

    def test_adjacent_manifest_matches_before_any_network_request(self):
        manifest = describe(self.firmware, "1.9.0-dev")
        path = self.firmware.with_suffix('.manifest.json')
        path.write_text(json.dumps(manifest))
        info = ota_client.inspect_firmware(self.firmware)
        self.assertTrue(info['manifest_checked'])
        self.assertEqual(info['firmware_version'], '1.9.0-dev')
        manifest['sha256'] = '0'*64; path.write_text(json.dumps(manifest))
        with self.assertRaisesRegex(ValueError, 'Manifest'):
            ota_client.inspect_firmware(self.firmware)

    def test_boot_confirmation_rejects_other_version(self):
        state = {'board':ota_client.BOARD_ID,'ready':True,'running_slot':'app1','boot_health':'passed','version':'1.7.0'}
        with patch.object(ota_client,'read_ota_status',return_value=state), patch.object(ota_client.time,'sleep'):
            with self.assertRaisesRegex(OSError,'different'):
                ota_client.wait_for_boot('192.168.0.107','app1',expected_version='1.9.0-dev')

    def test_upload_streams_binary_with_exact_hash_and_auth(self):
        status = {"board": ota_client.BOARD_ID, "ready": True, "phase": "idle",
                  "inactive_slot_bytes": 6 * 1024 * 1024,
                  "running_slot": "app0", "inactive_slot": "app1"}
        progresses = []
        with patch.object(ota_client, "read_ota_status", return_value=status), \
                patch.object(ota_client.http.client, "HTTPConnection", MockConnection):
            result = ota_client.upload_firmware("192.168.0.107", "test-pairing-key",
                                                self.firmware, lambda done, size: progresses.append((done, size)))
        self.assertTrue(result["verified"])
        self.assertEqual(result["target_slot"], "app1")
        conn = MockConnection.latest
        self.assertTrue(conn.closed)
        self.assertEqual(conn.method, "POST")
        self.assertEqual(conn.path, "/api/v1/ota/upload")
        self.assertEqual(conn.headers["X-MEO-OTA-BOARD"], ota_client.BOARD_ID)
        self.assertEqual(conn.headers["X-MEO-OTA-SIZE"], str(len(self.image)))
        self.assertEqual(conn.headers["X-MEO-OTA-SHA256"], hashlib.sha256(self.image).hexdigest())
        self.assertEqual(conn.headers["X-MEO-KEY"], "test-pairing-key")
        self.assertEqual(conn.headers["Content-Length"], sum(len(p) for p in conn.parts))
        self.assertIn(self.image, b"".join(conn.parts))
        self.assertEqual(progresses[-1], (len(self.image), len(self.image)))

    def test_firmware_upload_refuses_missing_slot(self):
        with patch.object(ota_client, "read_ota_status", return_value={
            "board": ota_client.BOARD_ID, "ready": False, "phase": "idle",
            "inactive_slot_bytes": 0,
        }):
            with self.assertRaisesRegex(ValueError, "sẵn sàng"):
                ota_client.upload_firmware("192.168.0.107", "key", self.firmware)

    def test_firmware_upload_refuses_same_slot(self):
        with patch.object(ota_client, "read_ota_status", return_value={
            "board": ota_client.BOARD_ID, "ready": True, "phase": "idle",
            "running_slot": "app0", "inactive_slot": "app0", "inactive_slot_bytes": 6291456,
        }):
            with self.assertRaisesRegex(ValueError, "độc lập"):
                ota_client.upload_firmware("192.168.0.107", "key", self.firmware)

    def verify_sequence(self, responses, timeout=12):
        clock = [0.0]
        calls = iter(responses)

        def next_response(host, **_kwargs):
            result = next(calls)
            if isinstance(result, Exception):
                raise result
            return dict(board=ota_client.BOARD_ID, ready=True, **result)

        def sleep(seconds):
            clock[0] += seconds

        notices = []
        with patch.object(ota_client, "read_ota_status", side_effect=next_response), \
             patch.object(ota_client.time, "monotonic", side_effect=lambda: clock[0]), \
             patch.object(ota_client.time, "sleep", side_effect=sleep):
            result = ota_client.wait_for_boot("192.168.0.107", "app1",
                                              timeout_s=timeout, interval_s=2,
                                              status_callback=notices.append)
            return result, notices

    def test_boot_confirmation_waits_for_target_and_health(self):
        result, notices = self.verify_sequence([
            OSError("device rebooting"),
            {"running_slot": "app0", "boot_health": "passed"},
            {"running_slot": "app1", "boot_health": "checking"},
            {"running_slot": "app1", "boot_health": "passed"},
        ])
        self.assertEqual(result["running_slot"], "app1")
        self.assertGreaterEqual(len(notices), 3)

    def test_boot_confirmation_detects_rollback(self):
        with self.assertRaisesRegex(OSError, "quay lại"):
            self.verify_sequence([
                {"running_slot": "app1", "boot_health": "checking"},
                {"running_slot": "app0", "boot_health": "passed"},
            ])

    def test_boot_confirmation_rejects_failed_health(self):
        with self.assertRaisesRegex(OSError, "thất bại"):
            self.verify_sequence([{"running_slot": "app1", "boot_health": "failed"}])

    def test_boot_confirmation_times_out_if_old_slot_only(self):
        with self.assertRaises(TimeoutError):
            self.verify_sequence([{"running_slot": "app0", "boot_health": "passed"}] * 4,
                                 timeout=5)


if __name__ == "__main__":
    unittest.main()
