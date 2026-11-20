// Run with: node firmware/tools/test_web_sha.js
// Ensures the LAN/HTTP SHA-256 fallback matches a reference digest.
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const crypto = require('crypto');
const assert = require('assert');

const page = fs.readFileSync(path.join(__dirname, '..', 'src', 'web_ui.h'), 'utf8');
const start = page.indexOf('function sha256Fallback(bytes){');
const end = page.indexOf('async function firmwareSha256(buffer){', start);
assert(start >= 0 && end > start, 'Web Studio SHA-256 implementation missing');
const sha256Fallback = vm.runInNewContext(page.slice(start, end) + '\nsha256Fallback;', {Uint8Array, Uint32Array, DataView, Math});
for (const input of [Buffer.from(''), Buffer.from('abc'), Buffer.from('hello world'),
                     Buffer.from('a'.repeat(65)), Buffer.alloc(1024 * 1024, 0xe9)]) {
  const expected = crypto.createHash('sha256').update(input).digest('hex');
  assert.equal(sha256Fallback(input), expected);
}
process.stdout.write('OK: browser HTTP-LAN SHA-256 fallback matches Node crypto\n');
