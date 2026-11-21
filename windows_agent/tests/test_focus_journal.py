import unittest
import sys
import tempfile
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from focus_journal import FocusJournal

def state(seq,boot="a",minutes=25):
    return dict(state_boot_id=boot,focus_completed_total=seq,focus_last_completed_min=minutes)

class FocusJournalTests(unittest.TestCase):
    def test_fresh_dedupe_daily_and_persistence(self):
        with tempfile.TemporaryDirectory() as root:
            path=Path(root)/'focus.db'
            journal=FocusJournal(path)
            self.assertFalse(journal.observe('esp',state(8),now=0,day='2026-11-21'))
            self.assertTrue(journal.observe('esp',state(9),now=1,day='2026-11-21'))
            self.assertFalse(journal.observe('esp',state(9),now=2,day='2026-11-21'))
            self.assertEqual(journal.summary('2026-11-21'),(1,25))
            journal.close(); journal=FocusJournal(path)
            self.assertEqual(journal.summary('2026-11-21'),(1,25))
            journal.observe('esp',state(8),now=0)
            self.assertFalse(journal.observe('esp',state(9),now=1))
            journal.close()
    def test_no_backfill_after_gap_reboot_reset_or_jump(self):
        journal=FocusJournal()
        for seq,boot,now in ((1,'a',0),(2,'a',10),(3,'b',11),(0,'b',12),(3,'b',13)):
            self.assertFalse(journal.observe('esp',state(seq,boot),now=now))
        self.assertEqual(journal.summary(),(0,0));journal.close()
    def test_invalid_minutes_and_separate_devices(self):
        journal=FocusJournal()
        journal.observe('a',state(0),now=0)
        self.assertFalse(journal.observe('a',state(1,minutes=0),now=1))
        journal.observe('b',state(0),now=1)
        self.assertTrue(journal.observe('b',state(1,minutes=50),now=2))
        self.assertEqual(journal.summary(),(1,50));journal.close()
