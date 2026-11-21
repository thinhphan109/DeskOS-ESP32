// Device-owned server OTA. Auth stays in the existing pairing field/session.
const pull = {revision:0,dirty:false,pending:false,supported:false,loaded:false,state:{},demo:false};
const pullMessages = {
  idle:'Chưa có lượt cập nhật đang chạy.',checking:'Đang kiểm tra bản mới…',up_to_date:'Đang dùng bản mới nhất trong kênh đã chọn.',
  release_available:'Có phiên bản mới sẵn sàng.',waiting_for_focus:'Chờ kết thúc phiên tập trung.',waiting_for_device:'Chờ thiết bị sẵn sàng.',
  reboot_notice:'Sắp bắt đầu cập nhật.',downloading:'Thiết bị đang tải firmware…',verified_rebooting:'Đã xác minh. Đang khởi động lại…',
  update_confirmed:'Đã xác nhận firmware mới hoạt động.',returned_previous_firmware:'Thiết bị đang dùng bản trước; đã tạm ngừng tự thử bản vừa cài.',
  policy_conflict:'Chính sách vừa thay đổi ở giao diện khác. Đọc lại trước khi lưu.',release_changed:'Bản mới đã thay đổi. Kiểm tra lại trước khi cập nhật.',
  server_not_configured:'Nhập địa chỉ manifest rồi lưu để kiểm tra.',update_busy:'Thiết bị đang xử lý lượt cập nhật khác.',
  invalid_policy:'Kiểm tra địa chỉ manifest, kênh và chu kỳ.',incompatible_manifest:'Manifest không phù hợp với thiết bị hoặc kênh.',
  incompatible_release:'Bản phát hành không phù hợp; kiểm tra phiên bản cầu nối và dung lượng.',release_mutated:'Nội dung release đã đổi. Cần mã release mới.',
  stale_manifest:'Server đang trả manifest cũ. Thiết bị sẽ không hạ phiên bản.',http_connect_failed:'Chưa kết nối được server. Kiểm tra Wi-Fi, địa chỉ và giờ thiết bị.',
  http_status_failed:'Server chưa cung cấp file cần tải.',content_size_mismatch:'Dung lượng khác manifest. Thiết bị đã giữ firmware hiện tại.',
  image_verification_failed:'Firmware không qua xác minh. Thiết bị đã giữ bản hiện tại.',download_interrupted:'Tải bị gián đoạn; sẽ thử lại theo lịch.'
};
const pullFields=['pullUrl','pullMode','pullChannel','pullInterval','pullDefer'];
function pullControls(){
  const allowed=pull.supported&&!pull.pending;
  pullFields.forEach(id=>$(id).disabled=!allowed);
  $('pullReload').disabled=pull.pending;
  $('pullSave').disabled=!allowed||!pull.dirty;
  $('pullCheck').disabled=!allowed||pull.dirty;
  $('pullStart').disabled=!allowed||pull.dirty||!pull.state.release_id||!['available','error'].includes(pull.state.phase);
}
function pullRender(state){
  pull.state={...state};
  const phase=state.phase||'idle',active=state.manual_request||['downloading','rebooting'].includes(phase);
  const target=(active?state.active_version:state.available_version)||'chưa có bản mới';
  $('pullVersion').textContent=(state.current_version||'—')+' → '+target;
  if(active&&state.available_version&&state.available_version!==target)$('pullVersion').textContent+=' · Đang chờ thêm bản '+state.available_version;
  let message=pullMessages[state.code]||'Đang chờ trạng thái cập nhật từ thiết bị.';
  if(pull.demo)message='DEMO · '+message;
  if(phase==='countdown')message+=' Bắt đầu sau '+(state.countdown_s||0)+' giây.';
  if(state.quarantined)message+=' Đã ngừng thử tự động; có thể chủ động thử lại.';
  $('pullStatus').textContent=message;$('pullNotes').textContent=state.notes||'';
  $('pullProgress').hidden=!['checking','downloading','rebooting'].includes(phase);
  if(['checking','rebooting'].includes(phase))$('pullProgress').removeAttribute('value');
  else $('pullProgress').value=state.expected?Math.round(100*Math.min(state.written||0,state.expected)/state.expected):0;
  pullControls();
}
function pullReceive(data){
  if(data.demo||data.device==='MEO DESK / MOCK')pull.demo=true;
  if(data.update_state)pullRender(data.update_state);
  const policy=data.update_policy;
  if(!policy)return;
  const capable=(data.capabilities||[]).includes('ota_pull_v1')||pull.supported;
  if(!capable){$('pullNotice').textContent='Thiết bị cần firmware cầu nối cho OTA server. Dùng cập nhật bằng file bên dưới.';pull.supported=false;pullControls();return;}
  if(policy.revision!==pull.revision){
    if(pull.dirty)$('pullNotice').textContent='Chính sách đã đổi ở giao diện khác. Bản nháp được giữ; đọc lại trước khi lưu.';
    else if(!pull.pending&&pair.value.trim())pullRequest('load',{},false);
  }
}
function pullCommand(){return 'mimi-'+Date.now().toString(16)+'-'+Math.random().toString(16).slice(2);}
async function pullRequest(operation,payload={},force=true){
  if(pull.pending)return;
  const key=pair.value.trim();if(!key){$('pullNotice').textContent='Nhập và lưu mã ghép nối để quản lý cập nhật.';return;}
  const paths={load:'/api/v1/update-policy',configure:'/api/v1/update-policy',check:'/api/v1/update/check',start:'/api/v1/update/start'};
  if(!paths[operation])return;
  if(['check','start'].includes(operation))payload={...payload,command_id:pullCommand()};
  pull.pending=true;pullControls();$('pullNotice').textContent='Đang trao đổi với thiết bị…';
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);
  try{
    const response=await fetch(paths[operation],{method:operation==='load'?'GET':'POST',cache:'no-store',signal:controller.signal,
      headers:{'Content-Type':'application/json','X-MEO-KEY':key},...(operation==='load'?{}:{body:JSON.stringify(payload)})});
    if(response.status===404){pull.supported=false;$('pullNotice').textContent='Thiết bị cần firmware cầu nối hỗ trợ OTA server. Dùng cập nhật bằng file bên dưới.';const details=document.querySelector('.manual-tools');if(details)details.open=true;return;}
    const data=await response.json();
    if(!response.ok)throw Error(pullMessages[data.error]||(response.status===401?'Kiểm tra lại mã ghép nối.':'Thiết bị chưa nhận thay đổi. Kiểm tra kết nối rồi thử lại.'));
    if(data.schema!==1||!data.policy||!data.update)throw Error('Phản hồi cập nhật chưa hợp lệ.');
    pull.supported=true;pull.loaded=true;pull.demo=pull.demo||data.demo===true;
    if(force||!pull.dirty){
      const p=data.policy;$('pullUrl').value=p.manifest_url||'';$('pullMode').value=p.mode;$('pullChannel').value=p.channel;
      $('pullInterval').value=p.interval_s;$('pullDefer').checked=p.defer_focus;pull.revision=p.revision;pull.dirty=false;
    }
    pullRender(data.update);$('pullNotice').textContent=pull.demo?'DEMO · chính sách và tiến độ mô phỏng.':operation==='configure'?'Đã lưu chính sách trên thiết bị.':'Chính sách lấy từ thiết bị.';
  }catch(error){$('pullNotice').textContent=error.name==='AbortError'?'Thiết bị chưa phản hồi. Đọc lại để kiểm tra kết quả trước khi thử tiếp.':error.message;}
  finally{clearTimeout(timer);pull.pending=false;pullControls();}
}
$('pullForm').addEventListener('submit',event=>{
  event.preventDefault();const seconds=Number($('pullInterval').value),url=$('pullUrl').value.trim();
  if(!Number.isInteger(seconds)||seconds<60||seconds>86400){$('pullNotice').textContent='Chu kỳ kiểm tra từ 60 đến 86.400 giây.';return;}
  pullRequest('configure',{expected_revision:pull.revision,manifest_url:url,mode:$('pullMode').value,channel:$('pullChannel').value,
    interval_s:seconds,defer_focus:$('pullDefer').checked});
});
pullFields.forEach(id=>$(id).addEventListener('input',()=>{pull.dirty=true;pullControls();}));
$('pullReload').addEventListener('click',()=>pullRequest('load'));
$('pullCheck').addEventListener('click',()=>pullRequest('check'));
$('pullStart').addEventListener('click',()=>pullRequest('start',{release_id:pull.state.release_id,expected_revision:pull.revision}));
pullControls();
let otaExpected=null,otaResuming=false;
try{const saved=JSON.parse(localStorage.getItem('mimi-ota-job')||'null');if(saved&&['app0','app1'].includes(saved.target_slot))otaExpected=saved;}catch(_){}
function saveOtaExpectation(targetSlot,phase){
  otaExpected={schema:1,target_slot:targetSlot,device_id:otaDevice?.device_id||'',previous_boot_id:otaDevice?.state_boot_id||'',phase};
  try{localStorage.setItem('mimi-ota-job',JSON.stringify(otaExpected));}catch(_){}
}
function finishOtaExpectation(){otaExpected=null;try{localStorage.removeItem('mimi-ota-job');}catch(_){} }
async function resumeWebOta(){
  if(!otaExpected||otaResuming||otaUploading)return;
  otaResuming=true;$('otaResult').textContent='Đang kiểm tra lượt cập nhật trước; không gửi lại firmware.';
  try{await verifyOtaBoot(otaExpected.target_slot,otaExpected);finishOtaExpectation();$('otaResult').textContent='Đã xác nhận firmware mới hoạt động.';}
  catch(error){$('otaResult').textContent='Chưa xác nhận được lượt trước. Kiểm tra nguồn, kết nối và địa chỉ thiết bị.';$('otaResult').title=error.message;}
  finally{otaResuming=false;}
}
const resumeButton=document.createElement('button');resumeButton.type='button';resumeButton.textContent='Kiểm tra lượt cập nhật trước';resumeButton.addEventListener('click',resumeWebOta);$('otaResult').after(resumeButton);
if(otaExpected)setTimeout(resumeWebOta,1500);
$('setupPair').addEventListener('click',async()=>{
  $('setupResult').textContent='Đang lấy mã. Máy tính cần kết nối Wi-Fi thiết lập của thiết bị.';
  const controller=new AbortController(),timer=setTimeout(()=>controller.abort(),4000);
  try{const response=await fetch('/api/v1/pairing',{signal:controller.signal,cache:'no-store'});const data=await response.json();
    if(!response.ok||typeof data.pairing_key!=='string'||!data.pairing_key)throw Error('setup not available');
    pair.value=data.pairing_key;$('setupResult').textContent='Đã lấy mã từ thiết bị. Bấm Lưu mã để hoàn tất.';
  }catch(_){$('setupResult').textContent='Chưa lấy được mã. Kết nối Wi-Fi thiết lập của thiết bị hoặc nhập mã đang có.';}
  finally{clearTimeout(timer);}
});
