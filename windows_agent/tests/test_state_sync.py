import unittest
from unittest.mock import patch
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from state_sync import StateCursor, FocusDisplay, MediaDisplay


class StateSyncTests(unittest.TestCase):
    def test_media_play_pause_seek_end_and_stale(self):
        clock = [10.0]
        with patch("state_sync.time.monotonic", side_effect=lambda:clock[0]):
            display = MediaDisplay()
            state = {"available":True,"playing":True,"position_ms":1000,"duration_ms":3000}
            display.update(state)
            clock[0] += 1.2
            self.assertAlmostEqual(display.position(),2200,delta=1)
            clock[0] += 1
            self.assertEqual(display.position(),3000)
            display.update(dict(state,playing=False,position_ms=500))
            clock[0] += 1
            self.assertEqual(display.position(),500)
            display.update(dict(state,position_ms=200))
            self.assertEqual(display.position(),200)
            clock[0] += 3.5
            self.assertIsNone(display.position())
            display.update(dict(state,source_age_ms=9990))
            clock[0] += 0.1
            self.assertIsNone(display.position())
    def test_state_order_and_new_boot(self):
        cursor = StateCursor()
        def state(boot, revision):
            return {"device": "MEO DESK", "state_boot_id": boot, "state_revision": revision}
        self.assertTrue(cursor.accept(state("boot-a", 9)))
        self.assertFalse(cursor.accept(state("boot-a", 8)))
        self.assertTrue(cursor.accept(state("boot-a", 10)))
        self.assertTrue(cursor.accept(state("boot-b", 1)))

    def test_legacy_and_unknown_device(self):
        cursor = StateCursor()
        self.assertTrue(cursor.accept({"device": "MEO DESK", "version": "1.7.0"}))
        self.assertFalse(cursor.accept({"device": "OTHER"}))
        self.assertFalse(cursor.accept([]))

    def test_focus_interpolation_stops_when_stale(self):
        clock = [10.0]
        with patch("state_sync.time.monotonic", side_effect=lambda: clock[0]):
            display = FocusDisplay()
            display.update({"focus_remaining_s": 10, "focus_running": True})
            clock[0] += 1.1
            self.assertEqual(display.seconds(), 9)
            clock[0] += 1.0
            self.assertEqual(display.seconds(), 8)
            clock[0] += 1.4
            self.assertIsNone(display.seconds())

    def test_focus_pause_and_zero_never_advance_phase(self):
        clock = [10.0]
        with patch("state_sync.time.monotonic", side_effect=lambda: clock[0]):
            display = FocusDisplay()
            display.update({"focus_remaining_s": 1, "focus_running": False})
            clock[0] += 2
            self.assertEqual(display.seconds(), 1)
            display.update({"focus_remaining_s": 1, "focus_running": True})
            clock[0] += 2
            self.assertEqual(display.seconds(), 0)
            self.assertNotIn("focus_phase", display.state)


if __name__ == "__main__":
    unittest.main()
