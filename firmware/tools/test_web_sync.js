// Behavior checks for the actual embedded state reducer and local pack decoder.
const fs=require('fs'),vm=require('vm'),assert=require('assert');
const raw=fs.readFileSync(require('path').join(__dirname,'../src/web_ui.h'),'utf8');
new vm.Script(raw.match(/<script>([\s\S]*?)<\/script>/)[1]);
function extract(begin,end){return raw.slice(raw.indexOf(begin),raw.indexOf(end,raw.indexOf(begin)));}
const updates=[],media=[],ports=[];
const ctx={stateBoot:'',stateRevision:-1,update:data=>updates.push(data),renderMedia:data=>media.push(data),connectState:port=>ports.push(port),Number,Array};
vm.createContext(ctx);
vm.runInContext(extract('function receiveState(data){','function connectState(port){')+'this.receive=receiveState;',ctx);
const state=(boot,revision,theme)=>({device:'MEO DESK',state_boot_id:boot,state_revision:revision,theme,capabilities:['state_websocket_v1'],state_ws_port:81});
ctx.receive(state('a',2,1));ctx.receive(state('a',1,0));
assert.equal(updates.length,1);assert.equal(updates[0].theme,1);
ctx.receive({...state('a',3,2),media:{available:false}});assert.equal(media.length,1);
ctx.receive(state('b',1,0));assert.equal(updates.length,3);
ctx.receive({device:'OTHER'});assert.equal(updates.length,3);
assert(ports.every(port=>port===81));
const decode=vm.runInNewContext(extract('function packFramePixels(','function paintPackPreview(')+'packFramePixels;', {DataView,Uint8ClampedArray});
const bytes=new Uint8Array(1990656);let view=new DataView(bytes.buffer);
// Third frame of the second pose: RGB565 red, green, blue, little endian.
let offset=(1*3+2)*144*144*2;view.setUint16(offset,0xf800,true);view.setUint16(offset+2,0x07e0,true);view.setUint16(offset+4,0x001f,true);
const pixels=decode(bytes,1,2,3);
assert.deepEqual(Array.from(pixels.slice(0,12)),[255,0,0,255,0,255,0,255,0,0,255,255]);
console.log('OK: shared state ordering, reboot epoch, media state, pack pose/frame RGB565 preview');
