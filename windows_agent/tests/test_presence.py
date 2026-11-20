import sys
import unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from presence import idle_from_ticks, read_idle_seconds

class PresenceTests(unittest.TestCase):
    def test_ticks_and_rollover(self):
        self.assertEqual(idle_from_ticks(10000,7000),3)
        self.assertEqual(idle_from_ticks(1000,0xFFFFFFF0),1)
        self.assertIsNone(idle_from_ticks(1000,1001))
        self.assertEqual(idle_from_ticks(1000000000,0),604800)
    def test_unavailable_platform(self):
        with patch('presence.sys.platform','linux'):
            self.assertIsNone(read_idle_seconds())
