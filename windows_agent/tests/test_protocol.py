"""On-disk v1 fixtures; no network or optional jsonschema dependency."""
import json
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "windows_agent"))
from companion_core import wire_media


class ContractTests(unittest.TestCase):
    def test_python_and_firmware_versions_match(self):
        version = (ROOT / 'windows_agent/app_version.py').read_text(encoding="utf-8").split('"')[1]
        header = (ROOT / 'firmware/src/app_version.h').read_text(encoding="utf-8")
        self.assertIn('"'+version+'"', header)
    def test_ota_16mb_partition_layout(self):
        """Boot, NVS and two independent 6 MiB slots must never overlap."""
        csv = ROOT / "firmware" / "meodesk_ota_16mb.csv"
        partitions = {}
        for raw in csv.read_text(encoding="utf-8").splitlines():
            if not raw.strip() or raw.lstrip().startswith("#"):
                continue
            fields = [entry.strip() for entry in raw.split(",")]
            self.assertGreaterEqual(len(fields), 5)
            name, ptype, subtype = fields[:3]
            offset, size = int(fields[3], 16), int(fields[4], 16)
            self.assertGreater(size, 0)
            self.assertLessEqual(offset + size, 16 * 1024 * 1024)
            for other_name, (other_offset, other_size, _) in partitions.items():
                self.assertTrue(offset + size <= other_offset or other_offset + other_size <= offset,
                                f"{name} overlaps {other_name}")
            if ptype == "app":
                self.assertEqual(offset % 0x10000, 0)
            partitions[name] = (offset, size, subtype)
        self.assertEqual(partitions["nvs"], (0x9000, 0x5000, "nvs"))
        self.assertEqual(partitions["otadata"], (0xE000, 0x2000, "ota"))
        self.assertEqual(partitions["app0"], (0x10000, 0x600000, "ota_0"))
        self.assertEqual(partitions["app1"], (0x610000, 0x600000, "ota_1"))
        # v1.6 keeps v1.5's 0x1F0000 raw banks, now large enough for 48
        # uncompressed frames as well as legacy 16-frame packs.
        self.assertEqual(partitions["assets"], (0xC10000, 0x3E0000, "spiffs"))
        bank = 0x1F0000
        payload = 16 * 144 * 144 * 2
        self.assertLessEqual(0x1000 + payload, bank)
        self.assertLessEqual(0x1000 + payload * 3, bank)
        self.assertEqual(0xC10000 + 2 * bank, partitions["coredump"][0])

    def test_json_schema_parses(self):
        schema = json.loads((ROOT / "protocol" / "schema-v1.json").read_text(encoding="utf-8"))
        self.assertEqual(schema["$defs"]["media"]["properties"]["schema"]["const"], 1)
        self.assertIn("command_ack", schema["$defs"])

    def test_fixtures_keep_null_and_zero_distinct(self):
        payload = json.loads((ROOT / "protocol" / "fixtures" / "telemetry-real.json").read_text(encoding="utf-8"))
        self.assertIsNone(payload["cpu_temp_c"])
        self.assertEqual(payload["gpu_percent"], 0)
        self.assertFalse(payload["demo"])

    def test_media_fixtures_wire_compatible(self):
        for name in ("media-playing.json", "media-unavailable.json"):
            with self.subTest(name=name):
                payload = json.loads((ROOT / "protocol" / "fixtures" / name).read_text(encoding="utf-8"))
                sent = wire_media(payload)
                # v1.0+ adds an optional artwork ID. Historical v0.8 media
                # fixtures intentionally omit it and must remain compatible.
                self.assertEqual({key: sent[key] for key in payload}, payload)
                self.assertIn("artwork_id", sent)
                self.assertLess(len(json.dumps(sent).encode("utf-8")), 512)

    def test_firmware_keeps_legacy_routes(self):
        source = (ROOT / "firmware" / "src" / "pc_bridge.cpp").read_text(encoding="utf-8")
        for route in ("/api/status", "/api/telemetry", "/api/control", "/api/history",
                      "/api/settings", "/api/cat.bmp", "/api/v1/performance", "/api/v1/ota/status",
                      "/api/v1/ota/upload", "/api/v1/media",
                      "/api/v1/catpack/status", "/api/v1/catpack/upload",
                      "/api/v1/commands/ack", "/api/v1/wifi/config"):
            with self.subTest(route=route):
                self.assertIn('"' + route + '"', source)

    def test_motion_frame_switches_do_not_restart_pose_crossfade(self):
        renderer = (ROOT / "firmware" / "src" / "main.cpp").read_text(encoding="utf-8")
        self.assertIn("catPack.frameAt(pose, cue.cel)", renderer)
        self.assertIn("motionDirector.sample(liveMotionContext(now), now)", renderer)
        self.assertIn("context.realPc = pcTelemetryFresh() && !pcTelemetry.demo", renderer)
        self.assertIn("if (pose != lastCatPose)", renderer)
        self.assertNotIn("if (cat != lastCatImage)", renderer)
        self.assertIn("tft.pushImageDMA(", renderer)
        bridge = (ROOT / "firmware" / "src" / "pc_bridge.cpp").read_text(encoding="utf-8")
        self.assertIn('capabilities.add("catpack_motion_v2")', bridge)
        self.assertIn('capabilities.add("motion_director_v1")', bridge)
        self.assertIn('doc["accepted_pack_bytes"]', bridge)

    def test_firmware_has_one_entrypoint(self):
        src = ROOT / "firmware" / "src"
        contents = "\n".join(p.read_text(encoding="utf-8") for p in src.glob("*.cpp") if p.name != "cat_assets.cpp")
        self.assertEqual(contents.count("void setup()"), 1)
        self.assertEqual(contents.count("void loop()"), 1)


if __name__ == "__main__":
    unittest.main()
