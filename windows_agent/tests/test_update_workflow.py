"""Update policy conflicts, restart confirmation, and UI draft preservation."""
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import threading
import unittest
from unittest.mock import Mock, patch
from urllib.error import HTTPError

os.environ.setdefault("QT_QPA_PLATFORM","offscreen")
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
import update_client
import ota_client


def snapshot(revision=1, mode="notify", **state):
    return {"schema":1,"supported":True,"policy":{"revision":revision,"manifest_url":"https://example.com/preview.json",
            "mode":mode,"channel":"preview","interval_s":120,"defer_focus":True},
            "update":{"phase":"available","release_id":"preview-next","current_version":"2.13.0-dev","available_version":"2.14.0",**state}}


class UpdateWorkflowTests(unittest.TestCase):
    def test_old_firmware_conflict_and_command_identity(self):
        with patch.object(update_client,"json_request",side_effect=HTTPError("http://device",404,"",{},None)):
            self.assertFalse(update_client.request("192.168.0.107","test")["supported"])
        error=HTTPError("http://device",409,"",{},io.BytesIO(b'{"error":"policy_conflict"}'))
        with patch.object(update_client,"json_request",side_effect=error),self.assertRaises(update_client.UpdateError) as raised:
            update_client.request("192.168.0.107","test","configure",{})
        self.assertEqual(raised.exception.code,"policy_conflict")
        payload={"release_id":"next","command_id":"fixed-command"}
        with patch.object(update_client,"json_request",return_value=snapshot()) as request:
            update_client.request("192.168.0.107","test","start",payload)
            self.assertEqual(request.call_args.args[3]["command_id"],"fixed-command")

    def test_journal_excludes_secrets_and_survives_reopen(self):
        with tempfile.TemporaryDirectory() as folder:
            journal=update_client.UpdateJournal(Path(folder)/"job.json")
            job=journal.begin("192.168.0.107",{"inactive_slot":"app1","device_id":"known","state_boot_id":"old"},
                              {"sha256":"a"*64,"firmware_version":"2.14.0"})
            job.update(phase="uploaded",pairing_key="never-write-this",wifi_password="secret")
            journal.write(job)
            loaded=update_client.UpdateJournal(journal.path).read()
            self.assertEqual(loaded["job_id"],job["job_id"])
            self.assertEqual(loaded["phase"],"uploaded")
            self.assertNotIn("secret",journal.path.read_text())
            self.assertNotIn("never-write-this",journal.path.read_text())

    def test_boot_confirmation_requires_same_device_and_new_boot_and_can_rediscover(self):
        states=[{"device_id":"other","state_boot_id":"new"},
                {"device_id":"known","state_boot_id":"old"},
                {"device_id":"known","state_boot_id":"new"}]
        def read(host,**_):
            return {"running_slot":"app1","boot_health":"passed","ready":True,"version":"2.14.0",**states.pop(0)}
        with patch.object(ota_client,"read_ota_status",side_effect=read) as request, \
             patch.object(ota_client,"discover",return_value=[{"host":"meodesk.local","device_id":"known"}]), \
             patch.object(ota_client.time,"sleep"):
            result=ota_client.wait_for_boot("192.168.0.107","app1",expected_device_id="known",previous_boot_id="old",expected_version="2.14.0")
        self.assertEqual(result["host"],"meodesk.local")
        self.assertEqual(request.call_count,3)
        stop=threading.Event();stop.set()
        with patch.object(ota_client,"read_ota_status") as request,self.assertRaises(InterruptedError):
            ota_client.wait_for_boot("192.168.0.107","app1",stop_event=stop)
        request.assert_not_called()


@unittest.skipUnless(importlib.util.find_spec("PySide6"),"Qt optional")
class UpdatePanelTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        from PySide6.QtWidgets import QApplication
        cls.app=QApplication.instance() or QApplication([])

    def test_policy_live_sync_preserves_dirty_draft_and_conflict(self):
        from update_panel import UpdatePanel
        panel=UpdatePanel();requests=[]
        panel.requested.connect(lambda op,data:requests.append((op,data)))
        panel.complete(snapshot())
        panel.url.setText("https://example.com/my-draft.json");panel._edited()
        panel.receive_remote({"revision":2},snapshot(2,mode="auto")["update"])
        self.assertIn("my-draft",panel.url.text());self.assertEqual(panel.revision,1)
        self.assertIn("Bản nháp",panel.notice.text())
        panel.save_policy();self.assertEqual(requests[-1][1]["expected_revision"],1)
        self.assertFalse(panel.url.isEnabled())
        panel.reject(update_client.MESSAGES["policy_conflict"])
        self.assertIn("my-draft",panel.url.text())
        panel.reload.click();panel.complete(snapshot(2,mode="auto"))
        self.assertFalse(panel.dirty);self.assertEqual(panel.mode.currentData(),"auto")
        self.assertEqual(panel.revision,2)
        panel.receive_remote({"revision":3},snapshot(3)["update"])
        self.assertEqual(requests[-1][0],"load")
        panel.close()

    def test_app_resume_only_verifies_and_quit_waits_for_upload(self):
        from companion import MainWindow
        from companion_core import Settings
        with tempfile.TemporaryDirectory() as folder, \
             patch("companion.read_settings",return_value=Settings(host="192.168.0.107",device_id="known")), \
             patch("companion.secret_store.load",return_value="test"):
            win=MainWindow(start_services=False)
            try:
                win._update_journal=update_client.UpdateJournal(Path(folder)/"job.json")
                win._update_journal.begin(win.settings.host,{"inactive_slot":"app1","device_id":"known","state_boot_id":"old"},{"sha256":"a"*64})
                with patch("companion.threading.Thread") as thread,patch("companion.ota_client.wait_for_boot",return_value={"host":win.settings.host,"version":"2.14.0","running_slot":"app1"}) as verify, \
                     patch("companion.ota_client.upload_firmware") as upload:
                    thread.side_effect=lambda **kwargs:Mock(start=kwargs["target"])
                    win.resume_update_confirmation()
                    verify.assert_called_once();upload.assert_not_called()
                    self.assertEqual(win._update_journal.read()["phase"],"confirmed")
                win._upload_active=True;win.quit_app()
                self.assertFalse(win._closing);self.assertTrue(win._quit_after_update)
                win.update_ota({"type":"uploaded","data":{}})
                self.assertTrue(win._closing)
            finally:
                win._upload_active=False;win.quit_app()

    def test_scene_and_rule_conflicts_keep_draft_base_revision(self):
        from companion import MainWindow
        from companion_core import Settings
        from scene_layout import DEFAULT_LAYOUT
        with patch("companion.read_settings",return_value=Settings(host="192.168.0.107")), \
             patch("companion.secret_store.load",return_value="test"):
            win=MainWindow(start_services=False)
            try:
                win._studio_cas=True
                win._scene_base={"expected_revision":1,"expected_boot_id":"boot"}
                win.scene_editor.label.setText("Draft")
                win.scene_editor.label.textEdited.emit("Draft")
                before=json.dumps(win.scene_editor.data,sort_keys=True)
                win.update_from_worker({"type":"scene_layout","host":win.settings.host,"data":DEFAULT_LAYOUT,"force":False,"before":before,
                                        "base":{"expected_revision":2,"expected_boot_id":"boot"}})
                self.assertTrue(win.scene_editor.dirty);self.assertEqual(win._scene_base["expected_revision"],1)
                with patch("companion.threading.Thread") as thread,patch("companion.json_request",side_effect=HTTPError("http://device",409,"conflict",{},None)) as request:
                    thread.side_effect=lambda **kwargs:Mock(start=kwargs["target"])
                    win.save_widget_layout(win.scene_editor.data)
                    self.assertEqual(request.call_args.args[3]["expected_revision"],1)
                self.assertTrue(win.scene_editor.dirty);self.assertIn("Bản nháp",win.scene_editor.hint.text())
                win._rules_base={"expected_revision":1,"expected_boot_id":"boot"}
                win.rule_editor.editor.setPlainText('{"schema":1,"rules":[]}')
                win.update_from_worker({"type":"rules_result","host":win.settings.host,"data":{"schema":1,"rules":[]},
                    "saved":False,"force":False,"before":win.rule_editor.editor.toPlainText(),
                    "base":{"expected_revision":2,"expected_boot_id":"boot"}})
                self.assertTrue(win.rule_editor.dirty);self.assertEqual(win._rules_base["expected_revision"],1)
            finally:win.quit_app()


if __name__=="__main__":unittest.main()
