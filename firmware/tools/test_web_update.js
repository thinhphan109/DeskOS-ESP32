const assert=require('assert'),fs=require('fs'),vm=require('vm');
const nodes={};
function node(){return {value:'',checked:true,disabled:false,textContent:'',handlers:{},addEventListener(name,fn){this.handlers[name]=fn;},removeAttribute(name){delete this[name];},after(){}};}
const $=id=>nodes[id]||(nodes[id]=node());
let answer=null;const calls=[],details={open:false};
const context={$,pair:{value:'demo-key'},AbortController,console,Date,Math,
 document:{createElement:node,querySelector:()=>details},localStorage:{getItem:()=>null,setItem(){},removeItem(){}},
 setTimeout:()=>1,clearTimeout(){},fetch:async(path,options)=>{calls.push({path,options});return {status:answer.status||200,ok:!answer.status||answer.status===200,json:async()=>answer.data};}};
const code=fs.readFileSync(require('path').join(__dirname,'update_studio.js'),'utf8');
vm.runInNewContext(code+'\nglobalThis.api={pull,pullRequest,pullReceive,pullRender};',context);
const api=context.api;
const sample=revision=>({schema:1,supported:true,policy:{revision,manifest_url:'https://example.com/a.json',mode:'notify',channel:'preview',interval_s:120,defer_focus:true},update:{phase:'available',release_id:'next-release',current_version:'2.13.0',available_version:'2.14.0',code:'release_available'}});
(async()=>{
 answer={data:sample(1)};await api.pullRequest('load');assert.equal(api.pull.revision,1);
 $('pullUrl').value='https://example.com/draft.json';$('pullUrl').handlers.input();
 api.pullReceive({update_policy:{revision:2},update_state:sample(2).update,capabilities:['ota_pull_v1']});
 assert.equal($('pullUrl').value,'https://example.com/draft.json');assert.equal(api.pull.revision,1);
 assert.match($('pullNotice').textContent,/Bản nháp/);
 answer={status:409,data:{error:'policy_conflict'}};
 await api.pullRequest('configure',{expected_revision:1});
 assert.equal(api.pull.dirty,true);assert.match($('pullNotice').textContent,/Đọc lại/);
 answer={data:sample(2)};await api.pullRequest('load');assert.equal(api.pull.revision,2);assert.equal(api.pull.dirty,false);
 await api.pullRequest('start',{release_id:'next-release',expected_revision:2});
 const sent=JSON.parse(calls.at(-1).options.body);assert.match(sent.command_id,/^mimi-/);assert.equal(sent.release_id,'next-release');
 api.pullRender({...sample(2).update,phase:'downloading',expected:100,written:50,notes:'<img src=x onerror=bad()>',active_version:'2.14.0'});
 assert.equal($('pullProgress').value,50);assert.equal($('pullStart').disabled,true);assert.equal($('pullNotes').textContent,'<img src=x onerror=bad()>');
 answer={status:404};await api.pullRequest('load');assert.equal(api.pull.supported,false);assert.equal(details.open,true);
 console.log('OK: server OTA policy conflicts, draft preservation, progress, safe notes and legacy fallback');
})().catch(error=>{console.error(error);process.exitCode=1;});
