"""Read-only local staging proves the channel and immutable artifact contract."""
import hashlib
import importlib.util
import json
from functools import partial
from http.server import ThreadingHTTPServer
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from urllib.request import build_opener, ProxyHandler

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'windows_agent'))
sys.path.insert(0,str(root/'firmware/tools'))
from publish_update import stage
spec=importlib.util.spec_from_file_location('staging_server',root/'packaging/ota/serve_staging.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)


class StagingTests(unittest.TestCase):
    def test_channel_is_fresh_and_artifact_is_immutable_and_hash_matches(self):
        with tempfile.TemporaryDirectory() as folder:
            path=Path(folder);image=bytearray(4096);image[0]=0xe9;image[12]=9
            marker=b'mimi-public-firmware-v1:2.15.0-rc.1\0';image[64:64+len(marker)]=marker
            binary=path/'firmware.bin';binary.write_bytes(image)
            stage(binary,path/'server',version='2.15.0-rc.1',channel='preview',sequence=215001,
                  release_id='preview-2.15.0-rc.1',artifact_url='https://example.com/releases/preview-2.15.0-rc.1/firmware.bin')
            server=ThreadingHTTPServer(('127.0.0.1',0),partial(module.Handler,directory=str(path/'server')))
            worker=threading.Thread(target=server.serve_forever,daemon=True);worker.start()
            try:
                client=build_opener(ProxyHandler({}));base=f'http://127.0.0.1:{server.server_port}'
                with client.open(base+'/channels/preview.json') as response:
                    self.assertIn('no-cache',response.headers['Cache-Control']);manifest=json.load(response)
                with client.open(base+'/releases/preview-2.15.0-rc.1/firmware.bin') as response:
                    self.assertIn('immutable',response.headers['Cache-Control'])
                    self.assertEqual(hashlib.sha256(response.read()).hexdigest(),manifest['sha256'])
                with self.assertRaises(Exception):client.open(base+'/')
            finally:server.shutdown();server.server_close();worker.join(timeout=2)


if __name__=='__main__':unittest.main()
