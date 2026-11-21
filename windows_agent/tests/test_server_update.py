"""Release compatibility and immutable publication without network or flash."""
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest

sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from update_contract import validate_manifest, version_key, valid_url
tool=Path(__file__).resolve().parents[2]/"firmware/tools/publish_update.py"
spec=importlib.util.spec_from_file_location("publish_update",tool)
publisher=importlib.util.module_from_spec(spec);spec.loader.exec_module(publisher)


class ServerUpdateTests(unittest.TestCase):
    def test_semver_order_and_malformed_identifiers(self):
        ordered=["1.0.0-alpha","1.0.0-alpha.1","1.0.0-alpha.beta","1.0.0-beta",
                 "1.0.0-beta.2","1.0.0-beta.11","1.0.0-rc.1","1.0.0","2.9.0","2.10.0"]
        self.assertEqual(sorted(ordered,key=version_key),ordered)
        self.assertEqual(version_key("1.0.0+one"),version_key("1.0.0+two"))
        for invalid in ("01.0.0","1.2","1.0.0-01","1.0.0-a..b","4294967296.0.0"):
            with self.subTest(invalid=invalid),self.assertRaises(ValueError):version_key(invalid)

    def test_urls_keep_lan_debug_and_https(self):
        for url in ("https://example.com/firmware.bin","http://192.168.0.8:8000/channels/preview.json","http://meodesk.local/a"):
            self.assertTrue(valid_url(url))
        for url in ("http://example.com/file","file:///tmp/firmware","http://127.0.0.1/a","https://user:secret@example.com/a"):
            self.assertFalse(valid_url(url))

    def test_immutable_release_and_monotonic_channel(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder);image=root/"firmware.bin"
            binary=bytearray(4096);binary[0]=0xe9;binary[12]=9
            marker=b"mimi-public-firmware-v1:2.14.0\0";binary[64:64+len(marker)]=marker
            image.write_bytes(binary)
            params=dict(version="2.14.0",channel="preview",sequence=14,release_id="preview-2.14.0",
                        artifact_url="https://example.com/releases/2.14.0/firmware.bin",notes=["Cập nhật thử nghiệm"])
            first=publisher.stage(image,root/"server",**params)
            self.assertEqual(publisher.stage(image,root/"server",**params),first)
            self.assertEqual(validate_manifest(first,current_version="2.13.0-dev").version,"2.14.0")
            with self.assertRaises(ValueError):validate_manifest(first,current_version="2.12.0")
            with self.assertRaises(ValueError):validate_manifest(dict(first,partition_layout="wrong"))
            with self.assertRaises(ValueError):validate_manifest(dict(first,size=7*1024*1024))
            with self.assertRaises(ValueError):validate_manifest(dict(first,release_id="../outside"))
            with self.assertRaises(ValueError):publisher.stage(image,root/"server",**dict(params,sequence=13))
            image.write_bytes(bytes(binary[:-1])+b"x")
            with self.assertRaises(ValueError):publisher.stage(image,root/"server",**params)
            self.assertEqual(json.loads((root/"server/channels/preview.json").read_text(encoding="utf8")),first)
            private=bytes(binary).replace(b"mimi-public-firmware-v1:",b"mimi-private-bridge-v1:")
            image.write_bytes(private)
            with self.assertRaises(ValueError):publisher.stage(image,root/"server",**params)


if __name__=="__main__":unittest.main()
