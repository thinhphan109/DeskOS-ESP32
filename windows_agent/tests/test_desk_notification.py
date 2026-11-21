import unittest
import sys
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from desk_notification import notification_payload

class NotificationTests(unittest.TestCase):
    def test_hidden_content_not_transmitted(self):
        self.assertNotIn('PRIVATE',str(notification_payload('PRIVATE')))
        self.assertEqual(notification_payload('Reminder',hidden=False)['text'],'Reminder')
    def test_invalid_options(self):
        for options in ({'hidden':1},{'source':'a.b'},{'ttl':31},{'priority':3},{'hidden':False,'text':'Mèo'}):
            with self.subTest(options=options),self.assertRaises(ValueError): notification_payload(**options)
