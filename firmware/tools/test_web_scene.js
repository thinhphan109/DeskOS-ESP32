const assert=require('assert'),fs=require('fs'),vm=require('vm');
const raw=fs.readFileSync(require('path').join(__dirname,'../src/web_ui.h'),'utf8');
const source=raw.split('// SCENE_CONTRACT_BEGIN')[1].split('// SCENE_CONTRACT_END')[0];
const validate=vm.runInNewContext(source+';validateScene;');
const valid={schema:1,widgets:[{kind:0,x:0,y:0,w:162,h:40,label:'Clock'}]};
assert.equal(validate(valid).widgets.length,1);
const external=structuredClone(valid);external.widgets[0].kind=10;assert.equal(validate(external).widgets[0].kind,10);
external.widgets[0].kind=11;assert.throws(()=>validate(external));
for(const [key,value] of [['x',1],['x',-1],['kind',true],['label','Mèo'],['label','x'.repeat(32)]]){
 const candidate=structuredClone(valid);candidate.widgets[0][key]=value;assert.throws(()=>validate(candidate));
}
const overlap=structuredClone(valid);overlap.widgets.push({kind:1,x:0,y:38,w:80,h:36,label:'CPU'});assert.throws(()=>validate(overlap));
const clean=validate(valid);clean.widgets[0].label='Changed';assert.equal(valid.widgets[0].label,'Clock');
console.log('OK: browser scene validation, geometry, overlap, ASCII and independent draft');
