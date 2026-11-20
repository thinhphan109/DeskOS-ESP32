// Browser Cat Pack reboot/confirmation state machine, no ESP32 or DOM needed.
const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');

const html = fs.readFileSync(path.join(__dirname, '..', 'src', 'web_ui.h'), 'utf8');
const first = html.indexOf('async function verifyCatPackBoot(expectedHash){');
const last = html.indexOf('async function uploadCatPack(){', first);
assert(first > 0 && last > first, 'Cat Pack confirmation logic missing');
const sizeStart = html.indexOf('function catPackAllowed(file){');
const sizeEnd = html.indexOf('function updateCatPackButton(){', sizeStart);
assert(sizeStart > 0 && sizeEnd > sizeStart, 'Motion pack Web preflight missing');
function permitted(sizes, bytes, name='cats.meocat') {
  return vm.runInNewContext(
    `let catPackDevice={pack_bytes:663552,accepted_pack_bytes:${JSON.stringify(sizes)}};`+
    html.slice(sizeStart,sizeEnd)+`catPackAllowed({name:${JSON.stringify(name)},size:${bytes}})`,
  );
}
assert(permitted([663552,1990656],663552));
assert(permitted([663552,1990656],1990656));
assert(!permitted([663552,1990656],1990657));
assert(!permitted([663552],1990656)); // Older/low-memory firmware.
assert(!permitted([663552,1990656],663552,'not_a_pack.bin'));

async function simulate(values) {
  let time = 0, iteration = 0;
  const hash = 'f'.repeat(64), node = {textContent: ''};
  const scope = {
    Date: {now: () => time}, Promise, AbortController, $: () => node,
    setTimeout(f, ms) {if (ms === 1700) {time += ms;f();}return 1;},
    clearTimeout() {},
    async fetch() {
      const item = values[Math.min(iteration++, values.length - 1)];
      if (item instanceof Error) throw item;
      return {ok: true, json: async () => item};
    },
  };
  const verify = vm.runInNewContext(html.slice(first, last) + '\nverifyCatPackBoot', scope);
  return verify(hash);
}

(async () => {
  const sha = 'f'.repeat(64);
  const result = await simulate([
    new Error('router reconnect'),
    {source: 'builtin', sha256: ''},
    {source: 'custom', sha256: sha},
  ]);
  assert.equal(result.sha256, sha);
  await assert.rejects(simulate([{source: 'custom', sha256: '0'.repeat(64)}]), /not confirmed/);
  process.stdout.write('OK: Cat Pack browser reboot, digest confirmation, timeout\n');
})().catch(error => {process.stderr.write(String(error.stack || error));process.exitCode = 1;});
