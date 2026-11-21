"""Support exports contain capability status, never saved settings or song content."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))


@unittest.skipUnless(importlib.util.find_spec('PySide6'),'Qt optional')
class SupportTests(unittest.TestCase):
    def test_report_does_not_load_profile_or_export_track_content(self):
        import runtime_support
        with patch('companion_core.read_settings') as profile, \
             patch('media_provider.snapshot',return_value={'available':True,'playing':True,'title':'Private song','artist':'Private artist'}), \
             patch('media_provider.artwork_rgb565',return_value=b'\0'*8192):
            report=runtime_support.diagnostics(check_media=True)
            profile.assert_not_called()
            self.assertEqual(report['media']['artwork_bytes'],8192)
            with tempfile.TemporaryDirectory() as folder:
                path=Path(folder)/'diagnostics.json';runtime_support.write_report(path,report)
                raw=path.read_text(encoding='utf8')
                self.assertNotIn('Private',raw)
                self.assertNotIn('pairing_key',raw)
                self.assertEqual(json.loads(raw)['app_version'],runtime_support.VERSION)


if __name__=='__main__':unittest.main()
