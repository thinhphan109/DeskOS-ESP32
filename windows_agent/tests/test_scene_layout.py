import copy
import sys
import tempfile
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scene_layout import DEFAULT_LAYOUT, validate_layout, save_layout, load_layout


class SceneLayoutTests(unittest.TestCase):
    def test_external_kind_ids_preserve_old_contract(self):
        layout=copy.deepcopy(DEFAULT_LAYOUT);layout['widgets'][0]['kind']=10
        self.assertEqual(validate_layout(layout)['widgets'][0]['kind'],10)
        layout['widgets'][0]['kind']=11
        with self.assertRaises(ValueError):validate_layout(layout)
    def test_roundtrip(self):
        with tempfile.TemporaryDirectory() as root:
            path = Path(root) / "scene.json"
            save_layout(path, DEFAULT_LAYOUT)
            self.assertEqual(load_layout(path), DEFAULT_LAYOUT)

    def test_reject_invalid_geometry_and_labels(self):
        for key, value in (("x", 1), ("x", -1), ("h", 121), ("kind", True), ("label", "Mèo"), ("label", "x"*32)):
            layout = copy.deepcopy(DEFAULT_LAYOUT)
            layout["widgets"][0][key] = value
            with self.subTest(key=key, value=value), self.assertRaises(ValueError):
                validate_layout(layout)

    def test_overlap_and_count(self):
        layout = copy.deepcopy(DEFAULT_LAYOUT)
        layout["widgets"][1]["y"] = 38
        with self.assertRaises(ValueError):
            validate_layout(layout)
        with self.assertRaises(ValueError):
            validate_layout({"schema": 1, "widgets": []})

    def test_output_is_independent(self):
        clean = validate_layout(DEFAULT_LAYOUT)
        clean["widgets"][0]["label"] = "Changed"
        self.assertEqual(DEFAULT_LAYOUT["widgets"][0]["label"], "Clock")
