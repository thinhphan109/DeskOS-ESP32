import unittest
import sys
from pathlib import Path
from unittest.mock import patch,MagicMock
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import startup

class StartupTests(unittest.TestCase):
    def test_quoted_frozen_and_source_commands(self):
        frozen=startup.launch_command('C:/Program Files/MeoDesk/app.exe',frozen=True)
        self.assertIn('"',frozen);self.assertTrue(frozen.endswith('--minimized'))
        source=startup.launch_command('C:/Python/pythonw.exe','G:/Project Name/companion.py',frozen=False)
        self.assertIn('Project Name',source);self.assertIn('companion.py"',source)
    def test_registry_is_only_changed_explicitly(self):
        registry=MagicMock()
        with patch('startup.sys.platform','win32'),patch.dict(sys.modules,{'winreg':registry}):
            startup.is_enabled()
            registry.SetValueEx.assert_not_called()
            startup.set_enabled(True)
            registry.SetValueEx.assert_called_once()
            startup.set_enabled(False)
            registry.DeleteValue.assert_called_once()
