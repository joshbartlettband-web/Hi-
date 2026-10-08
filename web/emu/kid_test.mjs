// SPDX-License-Identifier: GPL-3.0-only
// Rainbow mode (firmware/src/kid.c) in the browser build, in Node.
//   node web/emu/kid_test.mjs build/emu/felucca.wasm [OUT_DIR]
// Checks: it is on from power-up (the hello, then the friend's name in the band), a key sounds and shows its letter in the bubble,
// PRESETS changes the friend (the band), every friend sounds at about the same level, MASTER is capped, PLAY starts
// the beat, the top row's held effects, ten beats on SEQ, REC's skies, and HOME + SAVE held 3 s leaves for the full Felucca. With OUT_DIR, a screenshot of each step (.ppm).
import fs from "fs";

const wasmPath = process.argv[2] || "build/emu/felucca.wasm", outDir = process.argv[3];
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
let fails = 0;
const check = (what, ok) => { console.log(`kid: ${what.padEnd(76)} ${ok ? "ok" : "FAIL"}`); if (!ok) fails++; };

const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {});
const ex = instance.exports, mem = ex.memory;
ex._initialize();
ex.web_nor_erase();
ex.web_boot();
ex.web_master(1023);
let peak = 0;
const render = (ms) => {
  const n = Math.round(ms * 44.1);
  for (let k = 0; k < n; k += 128) {
    const m = Math.min(128, n - k);
    ex.web_render(m);
    const l = new Float32Array(mem.buffer, ex.web_out_l(), m), r = new Float32Array(mem.buffer, ex.web_out_r(), m);
    for (let i = 0; i < m; i++) peak = Math.max(peak, Math.abs(l[i]), Math.abs(r[i]));
  }
};
const px = (x, y) => { const v = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240)[y * 240 + x]; return ((v & 255) << 8) | (v >> 8); };
const band = () => { const fb = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240); return fb.slice(190 * 240, 236 * 240).join(","); };
const shot = (name) => {
  if (!outDir) return;
  const fb = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240), b = Buffer.alloc(240 * 240 * 3);
  for (let i = 0; i < fb.length; i++) {
    const v = ((fb[i] & 255) << 8) | (fb[i] >> 8);
    b[i * 3] = (v >> 11) << 3; b[i * 3 + 1] = ((v >> 5) & 63) << 2; b[i * 3 + 2] = (v & 31) << 3;
  }
  fs.writeFileSync(`${outDir}/kid_${name}.ppm`, Buffer.concat([Buffer.from("P6 240 240 255\n"), b]));
};
const WHITE = 0xFFFF;

render(1200);
shot("hello");
const hiRed = () => { for (let y = 10; y < 100; y++) for (let x = 40; x < 120; x++) if (px(x, y) === 0xE9A8) return true; return false; };
check("on from power-up, with the hello (HI in rainbow letters: H red)", hiRed());
check("silent at rest", peak === 0);

ex.web_keys(1 << 12);
render(200);
shot("key");
check(`a key sounds (peak ${peak.toFixed(3)}) and lights its LED`, peak > 0.05 && (ex.web_lit_keys() >> 12) & 1);
const ring = px(186, 8);
check("its letter shows in the bubble (white inside, the note's colour ring)", px(186, 13) === WHITE && ring !== WHITE);
ex.web_keys(0);
render(1200);
check("let go: the bubble goes", px(186, 8) !== ring);
check("the key ended the hello: the friend's name in the band", !hiRed() && new Set(band().split(",")).size >= 3);
const name0 = band();

ex.web_enc(EN.PRESETS, 1);
render(1700);
shot("friend2");
check("PRESETS: another friend (the band shows its name)", band() !== name0);

const levels = [];
for (let f = 0; f < 20; f++) {
  peak = 0;
  for (const k of [12, 14, 16]) { ex.web_keys(1 << k); render(500); ex.web_keys(0); render(150); }
  levels.push(peak);
  ex.web_enc(EN.PRESETS, 1);
  render(400);
}
const lo = Math.min(...levels), hi = Math.max(...levels);
check(`all 20 friends sound, alike (peaks ${lo.toFixed(3)} .. ${hi.toFixed(3)})`, lo > 0.08 && hi / lo < 2);
check("MASTER all the way up stays at about half (no friend peaks over 0.25)", hi < 0.25);

ex.web_buttons(1 << B.PLAY); render(60); ex.web_buttons(0); render(1500);
check("PLAY: the beat runs", ex.web_playing() === 1);
ex.web_buttons(1 << B.PLAY); render(60); ex.web_buttons(0); render(200);
check("PLAY again stops it", ex.web_playing() === 0);

// the top row: a tap turns each effect on (the sound of a held key changes, its button lights, the band names it),
// a second tap off; SLEEPY wakes up by itself
{
  let sig = 0;
  const listen = (btns) => {
    ex.web_keys(1 << 12); render(300);
    if (btns) { ex.web_buttons(btns); render(60); ex.web_buttons(0); }
    sig = 0;
    const n = Math.round(600 * 44.1);
    for (let k = 0; k < n; k += 128) {
      const m = Math.min(128, n - k); ex.web_render(m);
      const l = new Float32Array(mem.buffer, ex.web_out_l(), m);
      for (let i = 0; i < m; i++) sig = (sig * 31 + Math.round(l[i] * 30000)) % 1000000007;
    }
    const r = { sig, band: band(), lit: ex.web_lit_buttons() };
    ex.web_keys(0);
    if (btns && (ex.web_lit_buttons() & btns)) { ex.web_buttons(btns); render(60); ex.web_buttons(0); }   // off again
    render(3500);
    r.off = !(ex.web_lit_buttons() & btns);
    return r;
  };
  const plain = listen(0);
  let ok = 0;
  for (const b of [B.FX, B.SCL, B.ENV, B.LFO, B.EDIT, B.GLO]) {
    const r = listen(1 << b);
    if (r.sig !== plain.sig && r.band !== plain.band && ((r.lit >> b) & 1) && r.off) ok++;
  }
  check(`the top row tapped: ${ok} of 6 effects change the sound, light up, show their word, and go off again`, ok === 6);
}

{
  const tap = (b) => { ex.web_buttons(1 << b); render(60); ex.web_buttons(0); render(200); };
  tap(B.FX); tap(B.SCL);
  const lit = ex.web_lit_buttons();
  check("tapping another effect switches to it (one at a time)", ((lit >> B.SCL) & 1) && !((lit >> B.FX) & 1));
  tap(B.SCL);
  tap(B.ENV); render(3300);
  check("SLEEPY wakes up by itself (its light goes off)", !((ex.web_lit_buttons() >> B.ENV) & 1));
}

// SEQ: ten beats, each named in the band; the friend's bass line plays with them
{
  const seen = new Set();
  for (let i = 0; i < 10; i++) { ex.web_buttons(1 << B.SEQ); render(60); ex.web_buttons(0); render(200); seen.add(band()); }
  check(`SEQ steps through ten beats (${seen.size} different names)`, seen.size === 10);
}

// REC: the next sky
{
  render(1700);
  const sky0 = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240).slice(0, 240 * 180).join(",");
  ex.web_buttons(1 << B.REC); render(60); ex.web_buttons(0); render(2000);
  const sky1 = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240).slice(0, 240 * 180).join(",");
  check("REC: another sky", sky0 !== sky1);
}

for (const r of [EN.K1, EN.K2, EN.K3, EN.K4, EN.ALGO, EN.SELECT]) { ex.web_enc(r, 3); render(200); }
for (const b of [B.FX, B.SCL, B.ENV, B.LFO, B.EDIT, B.GLO, B.ARP, B.SEQ, B.REC, B.HOME]) { ex.web_buttons(1 << b); render(60); ex.web_buttons(0); render(300); }
shot("knobs");
check("every knob and button turned and pressed: still running", ex.web_now_ms() > 0);

ex.web_buttons((1 << B.HOME) | (1 << B.SAVE));
render(3300);
ex.web_buttons(0);
render(400);
shot("exit");
check("HOME + SAVE held 3 s: the full Felucca (HOME's LED lit)", (ex.web_lit_buttons() >> B.HOME) & 1);

console.log(fails ? `kid: ${fails} FAILED` : "kid: all ok");
process.exit(fails ? 1 : 0);
