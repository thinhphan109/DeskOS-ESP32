import contextlib
import io
import json
import sys
import threading
import unittest
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch
from urllib.error import HTTPError
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from integration_client import IntegrationClient,main
from integration_feed import validate_feed


class IntegrationTests(unittest.TestCase):
    def test_payload_bounds(self):
        valid=dict(name='Room',value='22.5',unit='C',ttl_s=30)
        self.assertEqual(validate_feed(valid),valid)
        for key,value in (('ttl_s',True),('ttl_s',301),('value','x'*21),('name',''),('unit','°C')):
            with self.subTest(key=key),self.assertRaises(ValueError):validate_feed(dict(valid,**{key:value}))
    def test_cli_dry_run_never_connects_and_removes_private_text(self):
        out=io.StringIO()
        with patch('integration_client.IntegrationClient') as client,contextlib.redirect_stdout(out):
            self.assertEqual(main(['--host','127.0.0.1','--dry-run','notify','--text','PRIVATE']),0)
            client.assert_not_called()
        self.assertNotIn('PRIVATE',out.getvalue())
    def test_http_feed_privacy_and_redirect(self):
        received=[]
        class Handler(BaseHTTPRequestHandler):
            def log_message(self,*args):pass
            def do_POST(self):
                payload=json.loads(self.rfile.read(int(self.headers['Content-Length'])))
                received.append((self.path,payload,self.headers.get('X-MEO-KEY')))
                if self.path=='/redirect':
                    self.send_response(302);self.send_header('Location','/leak');self.end_headers();return
                body=b'{"ok":true}';self.send_response(200);self.send_header('Content-Length',str(len(body)));self.end_headers();self.wfile.write(body)
        server=ThreadingHTTPServer(('127.0.0.1',0),Handler)
        thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
        try:
            client=IntegrationClient('127.0.0.1','TEST_ONLY',port=server.server_port)
            client.feed(0,'22.5','Room','C');client.notify('PRIVATE')
            self.assertEqual(received[0][0],'/api/v1/feeds/0');self.assertEqual(received[0][2],'TEST_ONLY')
            self.assertNotIn('PRIVATE',json.dumps(received))
            with self.assertRaises(ValueError):client.feed(3,'0')
            with self.assertRaises(HTTPError):client._post('/redirect',{})
            self.assertNotIn('/leak',[r[0] for r in received])
        finally:server.shutdown();server.server_close();thread.join(timeout=2)
