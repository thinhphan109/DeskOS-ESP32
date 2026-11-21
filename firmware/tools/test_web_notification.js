const assert=require('assert'),fs=require('fs'),vm=require('vm');
const source=fs.readFileSync(require('path').join(__dirname,'../src/web_ui.h'),'utf8').split('// NOTIFICATION_CONTRACT_BEGIN')[1].split('// NOTIFICATION_CONTRACT_END')[0];
const prepare=vm.runInNewContext(source+';prepareNotification;');
assert(!JSON.stringify(prepare('PRIVATE',true)).includes('PRIVATE'));
assert.equal(prepare('Desk reminder',false).text,'Desk reminder');
for(const text of ['', 'x'.repeat(61), 'Mèo'])assert.throws(()=>prepare(text,false));
console.log('OK: browser notification privacy strips content and validates TFT text');
