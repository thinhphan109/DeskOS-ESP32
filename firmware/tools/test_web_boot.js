// Offline browser OTA-Guardian state-machine tests, no hardware or DOM package.
// Run with: node firmware/tools/test_web_boot.js
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const raw = fs.readFileSync(path.join(__dirname, '..', 'src', 'web_ui.h'), 'utf8');
const script = raw.match(/<script>([\s\S]*?)<\/script>/);
assert(script, 'Embedded HTML needs one app script');
new vm.Script(script[1]); // Validate all embedded script syntax.

const begin = raw.indexOf('async function verifyOtaBoot(targetSlot){');
const end = raw.indexOf('async function uploadOta(){', begin);
assert(begin > 0 && end > begin, 'OTA Guardian function missing');
const source = raw.slice(begin, end) + '\nverifyOtaBoot;';

async function run(states) {
  let time = 0;
  let index = 0;
  const nodes = {otaResult: {textContent: ''}};
  const context = {
    Date: {now: () => time},
    Promise,
    AbortController,
    $: name => nodes[name],
    setTimeout(fn, duration) {
      if (duration === 2000) {time += 2000; fn();}
      return 1;
    },
    clearTimeout() {},
    async fetch() {
      const answer = states[Math.min(index++, states.length - 1)];
      if (answer instanceof Error) throw answer;
      return {ok: true, json: async () => ({board: 'meodesk-esp32s3-n16r8', ready: true, ...answer})};
    },
  };
  const verify = vm.runInNewContext(source, context);
  return verify('app1');
}

(async () => {
  const ok = await run([
    new Error('rebooting'),
    {running_slot: 'app0', boot_health: 'passed'},
    {running_slot: 'app1', boot_health: 'checking'},
    {running_slot: 'app1', boot_health: 'passed'},
  ]);
  assert.equal(ok.running_slot, 'app1');
  await assert.rejects(
    run([{running_slot: 'app1', boot_health: 'checking'},
         {running_slot: 'app0', boot_health: 'passed'}]), /previous firmware slot/,
  );
  await assert.rejects(run([{running_slot: 'app1', boot_health: 'failed'}]), /failed device/);
  await assert.rejects(run([{running_slot: 'app0', boot_health: 'passed'}]), /85 s/);
  process.stdout.write('OK: embedded JS syntax, OTA new slot, rollback, failed check, timeout\n');
})().catch(e => {process.stderr.write(String(e.stack || e));process.exitCode = 1;});
