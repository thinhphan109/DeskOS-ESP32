#include "server_updater.h"
#include "app_version.h"
#include "ota_manager.h"
#include "cat_pack.h"
#include "core/update_contract.h"
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <mbedtls/sha256.h>
#include <time.h>
#include <ctype.h>
#include <new>

ServerUpdater serverUpdater;
static bool due(uint32_t now, uint32_t at) { return static_cast<int32_t>(now-at) >= 0; }
static bool textField(JsonVariantConst root, const char *name, char *out, size_t capacity) {
  if (!root[name].is<const char *>()) return false;
  const char *value = root[name];
  if (!*value || strlen(value) >= capacity) return false;
  strcpy(out, value); return true;
}

bool ServerUpdater::validUrl(const String &url) {
  if (url.length() < 10 || url.length() > 511 || url.indexOf('@') >= 0 || url.indexOf('#') >= 0) return false;
  for (size_t i=0;i<url.length();++i) if (static_cast<uint8_t>(url[i]) <= 32) return false;
  if (url.startsWith("https://")) return url.length() > 8;
  if (!url.startsWith("http://")) return false;
  String host = url.substring(7); int slash=host.indexOf('/'); if (slash>=0) host=host.substring(0,slash);
  int colon=host.indexOf(':'); if (colon>=0) host=host.substring(0,colon);
  if (host.endsWith(".local")) return true;
  unsigned a,b,c,d; char rest;
  if (sscanf(host.c_str(), "%u.%u.%u.%u%c", &a,&b,&c,&d,&rest) != 4 || a>255 || b>255 || c>255 || d>255) return false;
  return a==10 || (a==172 && b>=16 && b<=31) || (a==192 && b==168) || (a==169 && b==254);
}
const char *ServerUpdater::phaseName(Phase p) {
  switch(p) {
    case Checking:return "checking";case Available:return "available";case Waiting:return "waiting";
    case Countdown:return "countdown";case Downloading:return "downloading";case Rebooting:return "rebooting";
    case Confirmed:return "confirmed";case Error:return "error";default:return "idle";
  }
}
void ServerUpdater::begin(Progress progress, Reboot reboot) {
  progress_=progress; reboot_=reboot;
  queue_=xQueueCreate(4,sizeof(Packet));
  prefsReady_=prefs_.begin("mimiupdate",false);
  if (prefsReady_) {
    JsonDocument doc; const String saved=prefs_.getString("policy","");
    if (!deserializeJson(doc,saved) && doc["schema"]==1) {
      const String savedUrl=doc["manifest_url"]|"";
      const String savedChannel=doc["channel"]|"preview";
      if ((savedUrl.isEmpty() || validUrl(savedUrl)) && (savedChannel=="preview" || savedChannel=="stable")) {
        url_=savedUrl; channel_=savedChannel; automatic_=doc["mode"]=="auto";
        interval_=constrain(doc["interval_s"]|120U,60U,86400U);
        deferFocus_=doc["defer_focus"]|true; revision_=doc["revision"]|1U;
      }
    }
    highestSequence_=prefs_.getUInt("high_seq",0);
    seenId_=prefs_.getString("seen_id","");seenFingerprint_=prefs_.getString("seen_hash","");
    attemptId_=prefs_.getString("attempt_id","");attemptCount_=prefs_.getUInt("attempts",0);
    bootTarget_=prefs_.getString("boot_target","");
  }
  nextCheck_=millis()+45000U+(esp_random()%5000U);
  if (!queue_ || !prefsReady_) { phase_=Error; message_="update_storage_unavailable"; }
}
void ServerUpdater::writePolicy(JsonObject out, bool includeUrl) const {
  out["revision"]=revision_; out["mode"]=automatic_?"auto":"notify";
  out["channel"]=channel_;out["interval_s"]=interval_;out["defer_focus"]=deferFocus_;
  out["configured"]=!url_.isEmpty();if(includeUrl)out["manifest_url"]=url_;
}
void ServerUpdater::writeState(JsonObject out) const {
  out["phase"]=phaseName(phase_);out["code"]=message_;out["current_version"]=MEO_VERSION;
  out["release_id"]=pending_.id;out["available_version"]=pending_.version;out["notes"]=pending_.notes;
  out["active_release_id"]=active_.id;out["active_version"]=active_.version;
  out["manual_request"]=manualRequested_;
  out["written"]=downloading_?meoOta.written():0;out["expected"]=downloading_?meoOta.expected():0;
  out["last_check_epoch"]=lastCheckEpoch_;out["attempts"]=attemptCount_;
  out["quarantined"]=attemptId_==pending_.id && attemptCount_>=3;
  out["countdown_s"]=phase_==Countdown && !due(millis(),countdownAt_)?(countdownAt_-millis()+999U)/1000U:0;
}
void ServerUpdater::describe(JsonDocument &doc) const {
  doc["schema"]=1;doc["supported"]=true;
  writePolicy(doc["policy"].to<JsonObject>());writeState(doc["update"].to<JsonObject>());
}
bool ServerUpdater::configure(JsonVariantConst in, String &error, int &status) {
  status=400;
  if (!in["expected_revision"].is<uint32_t>() || in["expected_revision"].as<uint32_t>()!=revision_) {
    error="policy_conflict";status=409;return false;
  }
  if (!in["mode"].is<const char*>() || !in["channel"].is<const char*>() ||
      !in["manifest_url"].is<const char*>() || !in["interval_s"].is<uint32_t>() || !in["defer_focus"].is<bool>()) {
    error="invalid_policy";return false;
  }
  const String mode=in["mode"], channel=in["channel"], url=in["manifest_url"];
  const uint32_t interval=in["interval_s"];
  if ((mode!="auto" && mode!="notify") || (channel!="stable" && channel!="preview") ||
      (!url.isEmpty() && !validUrl(url)) || interval<60 || interval>86400 || revision_==UINT32_MAX) {
    error="invalid_policy";return false;
  }
  if (!prefsReady_) {error="update_storage_unavailable";status=503;return false;}
  if(url==url_ && channel==channel_ && (mode=="auto")==automatic_ && interval==interval_ &&
      in["defer_focus"].as<bool>()==deferFocus_)return true;
  JsonDocument saved; saved["schema"]=1;saved["revision"]=revision_+1;saved["mode"]=mode;
  saved["channel"]=channel;saved["manifest_url"]=url;saved["interval_s"]=interval;saved["defer_focus"]=in["defer_focus"];
  String raw;serializeJson(saved,raw);
  if (prefs_.putString("policy",raw)!=raw.length()) {error="policy_save_failed";status=503;return false;}
  const bool changedSource=url_!=url || channel_!=channel;
  url_=url;channel_=channel;automatic_=mode=="auto";interval_=interval;deferFocus_=in["defer_focus"];
  ++revision_;
  if (changedSource) {
    highestSequence_=0;seenId_="";seenFingerprint_="";pending_=Release{};manualRequested_=false;
    prefs_.putUInt("high_seq",0);prefs_.remove("seen_id");prefs_.remove("seen_hash");
  }
  if (!busy() && (!automatic_ || changedSource)) {phase_=Idle;message_="idle";}
  nextCheck_=millis()+1000;return true;
}
bool ServerUpdater::validCommand(const String &s) const {
  if (s.length()<8 || s.length()>64) return false;
  for(size_t i=0;i<s.length();++i)if(!MimiUpdate::identifier(s[i]))return false;
  return true;
}
bool ServerUpdater::duplicate(const String &command,const String &operation,String &error,int &status) {
  for(unsigned i=0;i<8;++i)if(commands_[i]==command) {
    if(operations_[i]!=operation){error="command_conflict";status=409;}return true;
  }
  return false;
}
void ServerUpdater::remember(const String &command,const String &operation) {
  commands_[commandCursor_]=command;operations_[commandCursor_]=operation;commandCursor_=(commandCursor_+1)%8;
}
bool ServerUpdater::check(const String &command,String &error,int &status) {
  status=400;if(!validCommand(command)){error="invalid_command_id";return false;}
  if(duplicate(command,"check",error,status))return error.isEmpty();
  if(url_.isEmpty()){error="server_not_configured";return false;}
  if(taskRunning_ || busy()){error="update_busy";status=409;return false;}
  if(!launch(false)){error="network_worker_unavailable";status=503;return false;}
  remember(command,"check");return true;
}
bool ServerUpdater::start(const String &release,uint32_t revision,const String &command,String &error,int &status) {
  status=400;const String operation="start:"+release;
  if(!validCommand(command)){error="invalid_command_id";return false;}
  if(duplicate(command,operation,error,status))return error.isEmpty();
  if(revision!=revision_){error="policy_conflict";status=409;return false;}
  if(taskRunning_ || busy()){error="update_busy";status=409;return false;}
  if(release.isEmpty() || release!=pending_.id){error="release_changed";status=409;return false;}
  int newer=0;if(!MimiUpdate::compareVersions(pending_.version,MEO_VERSION,newer) || newer<=0) {
    error="no_new_release";return false;
  }
  if(!meoOta.available()){error="ota_unavailable";status=503;return false;}
  active_=pending_;manualRequested_=true;phase_=Waiting;message_="waiting_for_device";
  remember(command,operation);return true;
}
bool ServerUpdater::acceptManifest(String &error) {
  JsonDocument doc;if(deserializeJson(doc,manifest_)){error="invalid_manifest_json";return false;}
  if(doc["schema"]!=1 || doc["board"]!=MeoOtaManager::BOARD_ID ||
      doc["partition_layout"]!="meodesk-ab-6m-assets-v1" || doc["channel"]!=channel_ ||
      !doc["release_sequence"].is<uint32_t>() || !doc["size"].is<uint32_t>()) {
    error="incompatible_manifest";return false;
  }
  Release release;
  if(!textField(doc,"release_id",release.id,sizeof(release.id)) || !textField(doc,"version",release.version,sizeof(release.version)) ||
     !textField(doc,"minimum_firmware",release.minimum,sizeof(release.minimum)) ||
     !textField(doc,"sha256",release.sha,sizeof(release.sha)) || !textField(doc,"url",release.url,sizeof(release.url))) {
    error="invalid_manifest_fields";return false;
  }
  if(!isalnum(static_cast<unsigned char>(release.id[0]))){error="invalid_release_id";return false;}
  for(const char *s=release.id;*s;++s)if(!MimiUpdate::identifier(*s) && *s!='.'){error="invalid_release_id";return false;}
  for(unsigned i=0;i<64;++i)if(!isxdigit(static_cast<unsigned char>(release.sha[i]))){error="invalid_hash";return false;}
  if(strlen(release.sha)!=64 || !validUrl(release.url)){error="invalid_artifact";return false;}
  release.size=doc["size"];release.sequence=doc["release_sequence"];
  int version=0,minimum=0;
  if(!release.sequence || release.size<MeoOtaManager::MIN_IMAGE_BYTES || release.size>6U*1024U*1024U ||
      !MimiUpdate::compareVersions(release.version,MEO_VERSION,version) ||
      !MimiUpdate::compareVersions(MEO_VERSION,release.minimum,minimum) || minimum<0) {
    error="incompatible_release";return false;
  }
  String canonical=String(release.id)+"|"+release.version+"|"+release.minimum+"|"+release.sha+"|"+release.url+"|"+release.size+"|"+release.sequence;
  uint8_t digest[32];mbedtls_sha256(reinterpret_cast<const uint8_t*>(canonical.c_str()),canonical.length(),digest,0);
  char hex[65];for(unsigned i=0;i<32;++i)snprintf(hex+i*2,3,"%02x",digest[i]);
  if((seenId_==release.id && seenFingerprint_!=hex) ||
      (release.sequence==highestSequence_ && !seenId_.isEmpty() && seenId_!=release.id)) {
    error="release_mutated";return false;
  }
  if(release.sequence<highestSequence_){error="stale_manifest";return false;}
  if(release.sequence>highestSequence_ || seenId_!=release.id) {
    highestSequence_=release.sequence;seenId_=release.id;seenFingerprint_=hex;
    prefs_.putUInt("high_seq",highestSequence_);prefs_.putString("seen_id",seenId_);prefs_.putString("seen_hash",seenFingerprint_);
  }
  String notes;
  if(doc["notes"].is<JsonArrayConst>())for(JsonVariantConst note:doc["notes"].as<JsonArrayConst>()) {
    if(note.is<const char*>()){if(!notes.isEmpty())notes+='\n';notes+=note.as<const char*>();}
    if(notes.length()>380)break;
  }
  if(notes.length()>380) {unsigned end=380;while(end && (static_cast<uint8_t>(notes[end])&0xc0)==0x80)--end;notes=notes.substring(0,end);}
  strlcpy(release.notes,notes.c_str(),sizeof(release.notes));
  pending_=release; // one coalesced slot, active_ is never edited by this path
  if(manualRequested_){phase_=Waiting;message_="waiting_for_device";}
  else if(version<=0){phase_=Idle;message_="up_to_date";}
  else {phase_=Available;message_="release_available";}
  return true;
}
bool ServerUpdater::launch(bool image) {
  if(taskRunning_ || !queue_ || url_.isEmpty())return false;
  imageTask_=image;cancelTask_=false;manifest_="";taskUrl_=image?String(active_.url):url_;
  taskPolicyRevision_=revision_;
  taskRunning_=true;
  if(!image){phase_=Checking;message_="checking";}
  TaskHandle_t handle=nullptr;
  if(xTaskCreate(taskEntry,"mimiUpdate",12288,this,1,&handle)!=pdPASS) {taskRunning_=false;return false;}
  return true;
}
void ServerUpdater::taskEntry(void *self) {static_cast<ServerUpdater*>(self)->fetch();vTaskDelete(nullptr);}
void ServerUpdater::fetch() {
  Packet *packet=new(std::nothrow) Packet;
  const char *error=packet?nullptr:"network_memory";
  esp_http_client_config_t config{};config.url=taskUrl_.c_str();config.timeout_ms=6000;
  config.disable_auto_redirect=true;config.buffer_size=4096;config.user_agent="mimi.desk/" MEO_VERSION;
  config.crt_bundle_attach=esp_crt_bundle_attach;
  esp_http_client_handle_t client=packet?esp_http_client_init(&config):nullptr;
  if(!client && !error)error="http_init_failed";
  bool opened=false;
  for(unsigned redirects=0;client && !error && redirects<6;++redirects) {
    if(esp_http_client_open(client,0)!=ESP_OK){error="http_connect_failed";break;}
    opened=true;
    const int64_t length=esp_http_client_fetch_headers(client);
    const int code=esp_http_client_get_status_code(client);
    if(length<0){error="http_headers_failed";break;}
    if(code==301 || code==302 || code==303 || code==307 || code==308) {
      if(redirects==5 || esp_http_client_set_redirection(client)!=ESP_OK){error="redirect_failed";break;}
      char *target=new(std::nothrow) char[2048];
      bool allowed=target && esp_http_client_get_url(client,target,2048)==ESP_OK;
      if(allowed){const String next(target);allowed=next.startsWith("https://") || (!taskUrl_.startsWith("https://") && validUrl(next));}
      delete[] target;
      if(!allowed){error="redirect_not_allowed";break;}
      esp_http_client_close(client);opened=false;continue;
    }
    if(code!=200){error="http_status_failed";break;}
    const uint32_t limit=imageTask_?active_.size:8192U;
    if(length>limit || (imageTask_ && length>0 && length!=limit)){error="content_size_mismatch";break;}
    uint32_t received=0;
    while(!cancelTask_) {
      const int count=esp_http_client_read(client,reinterpret_cast<char*>(packet->data),sizeof(packet->data));
      if(count<0){error="download_interrupted";break;}
      if(count==0) {
        if(!esp_http_client_is_complete_data_received(client))error="download_incomplete";
        break;
      }
      received+=count;if(received>limit){error="content_too_large";break;}
      packet->kind=0;packet->size=count;
      while(!cancelTask_ && xQueueSend(queue_,packet,pdMS_TO_TICKS(100))!=pdTRUE){}
    }
    if(!error && imageTask_ && received!=limit)error="content_size_mismatch";
    break;
  }
  if(client){if(opened)esp_http_client_close(client);esp_http_client_cleanup(client);}
  if(!packet) {
    // Small terminal packet allocated on stack; the data area is never read for size zero.
    Packet terminal{};terminal.kind=2;
    strlcpy(reinterpret_cast<char*>(terminal.data),"network_memory",sizeof(terminal.data));
    xQueueSend(queue_,&terminal,portMAX_DELAY);return;
  }
  packet->size=0;packet->kind=(error || cancelTask_)?2:1;
  if(error)strlcpy(reinterpret_cast<char*>(packet->data),error,sizeof(packet->data));
  else if(cancelTask_)strlcpy(reinterpret_cast<char*>(packet->data),"cancelled",sizeof(packet->data));
  xQueueSend(queue_,packet,portMAX_DELAY);delete packet;
}
void ServerUpdater::fail(const char *code) {
  cancelTask_=true;
  if(downloading_){meoOta.abort(code);if(progress_)progress_(-1);}
  downloading_=false;phase_=Error;message_=code;manualRequested_=false;
  prefs_.remove("boot_target");bootTarget_="";
  ++failures_;const uint32_t backoff=30U << (failures_>5?5:failures_-1);
  nextCheck_=millis()+backoff*1000U;
}
bool ServerUpdater::beginDownload(String &error) {
  if(!manualRequested_)active_=pending_;
  if(attemptId_!=active_.id){attemptId_=active_.id;attemptCount_=0;}
  if(manualRequested_)attemptCount_=0;
  ++attemptCount_;prefs_.putString("attempt_id",attemptId_);prefs_.putUInt("attempts",attemptCount_);
  bootTarget_=active_.version;
  if(prefs_.putString("boot_target",bootTarget_)!=bootTarget_.length()){error="update_storage_unavailable";return false;}
  lastProgress_=0;if(progress_)progress_(0);
  if(!meoOta.begin(active_.size,active_.sha,MeoOtaManager::BOARD_ID)) {
    if(progress_)progress_(-1);
    error="ota_begin_failed";return false;
  }
  downloading_=true;phase_=Downloading;message_="downloading";
  if(!launch(true)){error="network_worker_unavailable";return false;}
  return true;
}
void ServerUpdater::loop(bool connected,bool focusRunning) {
  const uint32_t now=millis();
  if(!bootSettled_ && strcmp(meoOta.bootHealth(),"checking")) {
    bootSettled_=true;
    if(!bootTarget_.isEmpty()) {
      if(bootTarget_==MEO_VERSION && !strcmp(meoOta.bootHealth(),"passed")) {
        phase_=Confirmed;message_="update_confirmed";attemptCount_=0;prefs_.putUInt("attempts",0);
      } else {phase_=Error;message_="returned_previous_firmware";attemptCount_=3;prefs_.putUInt("attempts",3);}
      prefs_.remove("boot_target");bootTarget_="";
    }
  }
  Packet packet;
  for(unsigned drained=0;queue_ && drained<2 && xQueueReceive(queue_,&packet,0)==pdTRUE;++drained) {
    if(packet.kind==0) {
      if(imageTask_) {
        if(downloading_ && !meoOta.append(packet.data,packet.size))fail("image_write_failed");
        if(downloading_ && progress_) {
          const int percent=100ULL*meoOta.written()/active_.size;
          if(percent!=lastProgress_){lastProgress_=percent;progress_(percent);}
        }
      } else if(phase_==Checking && manifest_.length()+packet.size<=8192) {
        manifest_.concat(reinterpret_cast<char*>(packet.data),packet.size);
      }
    } else {
      taskRunning_=false;
      if(packet.kind==2){if(phase_!=Error)fail(reinterpret_cast<char*>(packet.data));}
      else if(imageTask_ && downloading_) {
        if(!meoOta.finish())fail("image_verification_failed");
        else {downloading_=false;phase_=Rebooting;message_="verified_rebooting";if(progress_)progress_(100);if(reboot_)reboot_();}
      } else if(!imageTask_ && taskPolicyRevision_!=revision_) {
        // An in-flight old channel must never populate the newly saved policy.
        if(!busy()){phase_=Idle;message_="idle";}nextCheck_=now+1000U;
      } else if(!imageTask_ && phase_==Checking) {
        String error;lastCheckEpoch_=time(nullptr);nextCheck_=now+interval_*1000U+(esp_random()%5000U);
        if(!acceptManifest(error))fail(error.c_str());else failures_=0;
      }
    }
  }
  if(taskRunning_ || busy() || !queue_ || !prefsReady_)return;
  if(phase_==Error && !manualRequested_ && !due(now,nextCheck_))return;
  if(!url_.isEmpty() && connected && due(now,nextCheck_) && phase_!=Countdown) {
    if(!launch(false))fail("network_worker_unavailable");
    return;
  }
  const bool wanted=manualRequested_ || (automatic_ && !(attemptId_==pending_.id && attemptCount_>=3));
  int newer=0;const Release &selected=manualRequested_?active_:pending_;
  if(!wanted || !*selected.id || !MimiUpdate::compareVersions(selected.version,MEO_VERSION,newer) || newer<=0)return;
  if(!connected || !bootSettled_ || strcmp(meoOta.bootHealth(),"passed") || catPack.busy() || meoOta.busy() ||
      (deferFocus_ && focusRunning) || (!manualRequested_ && now<60000U)) {
    phase_=Waiting;message_=(deferFocus_ && focusRunning)?"waiting_for_focus":"waiting_for_device";return;
  }
  if(phase_!=Countdown){phase_=Countdown;message_="reboot_notice";countdownAt_=now+10000U;return;}
  if(due(now,countdownAt_)){String error;if(!beginDownload(error))fail(error.c_str());}
}
