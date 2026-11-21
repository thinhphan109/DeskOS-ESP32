import unittest
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from telemetry_history import TelemetryHistory

class HistoryTests(unittest.TestCase):
    def test_bucket_gap_and_unavailable(self):
        history=TelemetryHistory()
        history.add({"cpu_percent":20},now=1)
        history.add({"cpu_percent":30},now=1.5)
        self.assertEqual(len(history.samples),1)
        history.add({"cpu_percent":40},now=2)
        history.add({"cpu_percent":50},now=6)
        self.assertEqual([len(s) for s in history.segments(0,6)],[2,1])
        self.assertEqual(history.segments(1,6),[])
        history.add({"cpu_percent":float('nan')},now=7)
        self.assertIsNone(history.samples[-1][1][0])
    def test_source_and_demo_isolation_and_bound(self):
        history=TelemetryHistory()
        for now in range(100): history.add({"cpu_percent":40},'boardA',now)
        self.assertEqual(len(history.samples),61)
        self.assertEqual(len(history.visible(101)),59)
        history.add({"cpu_percent":90,"demo":True},'boardA',102)
        self.assertEqual(len(history.samples),1)
        history.add({"cpu_percent":45},'boardB',103)
        self.assertEqual(len(history.samples),1)
    def test_stale_source_not_sampled(self):
        history=TelemetryHistory()
        history.add({"cpu_percent":50,"data_age_ms":2500},now=3)
        self.assertFalse(history.samples)
