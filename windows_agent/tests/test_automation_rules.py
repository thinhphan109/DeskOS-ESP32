import copy
import sys
import unittest
from pathlib import Path
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from automation_rules import PRESET,validate_rules

class AutomationRuleTests(unittest.TestCase):
    def test_presets_and_disable_all(self):
        self.assertEqual(validate_rules(PRESET),PRESET)
        self.assertEqual(validate_rules({'schema':1,'rules':[]})['rules'],[])
        clean=validate_rules(PRESET);clean['rules'][0]['label']='Changed'
        self.assertEqual(PRESET['rules'][0]['label'],'CPU busy')
    def test_limits_and_allowed_actions(self):
        for key,value in (('hold_s',601),('cooldown_s',0),('threshold',101),('value',1),('do','shell'),('enabled',1),('label','Mèo')):
            data=copy.deepcopy(PRESET);data['rules'][0][key]=value
            with self.subTest(key=key),self.assertRaises(ValueError):validate_rules(data)
        data=copy.deepcopy(PRESET);data['rules']*=4
        with self.assertRaises(ValueError):validate_rules(data)
    def test_focus_pulse_and_hour_ranges(self):
        data=copy.deepcopy(PRESET);rule=data['rules'][0]
        rule.update(when='focus_complete',threshold=0,hold_s=0,do='celebrate')
        validate_rules(data)
        rule['hold_s']=1
        with self.assertRaises(ValueError):validate_rules(data)
        data['rules'][1]['threshold']=24
        with self.assertRaises(ValueError):validate_rules(data)
