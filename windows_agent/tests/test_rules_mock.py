"""Exercise full portable backups against the LAN-free preview API."""
import importlib.util
import json
import sys
import threading
import unittest
from http.server import ThreadingHTTPServer
from pathlib import Path
from urllib.request import Request,build_opener,ProxyHandler
from urllib.error import HTTPError
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'windows_agent'))
from automation_rules import PRESET


class RuleMockTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        spec=importlib.util.spec_from_file_location('rules_mock_api',ROOT/'firmware/tools/web_preview_server.py')
        module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
        cls.server=ThreadingHTTPServer(('127.0.0.1',0),module.Handler)
        cls.thread=threading.Thread(target=cls.server.serve_forever,daemon=True);cls.thread.start()
        cls.base=f'http://127.0.0.1:{cls.server.server_port}'
        cls.direct=build_opener(ProxyHandler({}))
    @classmethod
    def tearDownClass(cls):
        cls.server.shutdown();cls.server.server_close();cls.thread.join(timeout=2)
    def request(self,path,payload=None,pretty=False):
        body=(json.dumps(payload,indent=1) if pretty else json.dumps(payload,separators=(',',':'))).encode() if payload is not None else None
        with self.direct.open(Request(self.base+path,data=body,headers={'Content-Type':'application/json'}),timeout=2) as response:return json.load(response)
    def test_invalid_full_restore_preserves_all_current_state(self):
        self.request('/api/v1/rules',PRESET)
        backup=self.request('/api/v1/config')
        broken=json.loads(json.dumps(backup));broken['profile']['theme']=4
        broken['rules']['rules'][0]['threshold']=101
        with self.assertRaises(HTTPError) as error:self.request('/api/v1/config',broken)
        self.assertEqual(error.exception.code,400)
        current=self.request('/api/v1/config')
        for key in ('profile','scene_layout','rules'):self.assertEqual(current[key],backup[key])
    def test_large_valid_profile_layout_rules_roundtrip(self):
        backup=self.request('/api/v1/config')
        backup['scene_layout']={'schema':1,'widgets':[dict(kind=i,x=0,y=i*20,w=162,h=20,label='x'*31) for i in range(6)]}
        backup['rules']=json.loads(json.dumps(PRESET));backup['rules']['rules']*=3
        for rule in backup['rules']['rules']:rule['label']='x'*32
        self.assertGreater(len(json.dumps(backup,indent=1).encode()),2000)
        self.request('/api/v1/config',backup,pretty=True)
        current=self.request('/api/v1/config')
        for key in ('profile','scene_layout','rules'):self.assertEqual(current[key],backup[key])
