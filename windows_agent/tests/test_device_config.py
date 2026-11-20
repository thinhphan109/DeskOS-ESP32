import json
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import device_config

def backup():
    return {"schema": 1, "board": device_config.BOARD, "profile": {
        "theme": 1, "fps_profile": 3, "motion_style": 1, "motion": True,
        "auto_pages": False, "quiet": False, "scene_slots": [12,0,5,2],
        "focus_work_min": 50, "focus_break_min": 10, "focus_goal": 6,
        "cpu_limit": 88, "gpu_limit": 92, "temp_limit": 80, "battery_limit": 20}}

class ConfigTests(unittest.TestCase):
    def test_roundtrip_and_excluded_envelope_secrets(self):
        data = backup(); data["key"] = "PRIVATE"; data["wifi_password"] = "PRIVATE"
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / "profile.json"
            device_config.save_backup(path, data)
            self.assertNotIn("PRIVATE", path.read_text())
            self.assertEqual(device_config.load_backup(path), backup())
    def test_invalid_fields_do_not_overwrite_backup(self):
        for field, value in (("focus_work_min",0), ("fps_profile",True), ("scene_slots",[0,0,5,2]), ("quiet",1)):
            data=backup(); data["profile"][field]=value
            with self.subTest(field=field), self.assertRaises(ValueError):
                device_config.validate_backup(data)
        data=backup(); data["profile"]["wifi_password"]="private"
        with self.assertRaises(ValueError): device_config.validate_backup(data)
    def test_wrong_schema_board_and_large_file(self):
        for extra in ({"schema":2},{"board":"OTHER"}):
            with self.assertRaises(ValueError): device_config.validate_backup(dict(backup(),**extra))
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder)/"large.json"; path.write_bytes(b" "*8193)
            with self.assertRaises(ValueError): device_config.load_backup(path)

if __name__ == '__main__': unittest.main()
