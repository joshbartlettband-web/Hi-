// SPDX-License-Identifier: GPL-3.0-only
// Rainbow mode (firmware/src/kid.c) in the browser build, in Node.
//   node web/emu/kid_test.mjs build/emu/felucca.wasm [OUT_DIR]
// Checks: it is on from power-up (the hello, then the friend's name in the band), a key sounds and shows its letter in the bubble,
// PRESETS changes the friend (the band), every friend sounds at about the same level, MASTER is capped, PLAY starts
// the beat, the top row's tapped effects (and that a key after the tap still sounds), the staff, the grown-ups' VOLUME and the limiter,
// ten beats on SEQ, REC's skies, and HOME + SAVE held 3 s leaves for the full Felucca. With OUT_DIR, a screenshot of each step (.ppm).
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
const KID_Y = (step) => 54 - 3 * step;
const FXB = [B.FX, B.SCL, B.ENV, B.LFO, B.EDIT, B.GLO];

render(1200);
shot("hello");
const hiRed = () => { for (let y = 10; y < 100; y++) for (let x = 40; x < 120; x++) if (px(x, y) === 0xE9A8) return true; return false; };
check("on from power-up, with the hello (HI in rainbow letters: H red)", hiRed());
check("silent at rest", peak === 0);

ex.web_keys(1 << 12);
render(200);
shot("key");
check(`a key sounds (peak ${peak.toFixed(3)}) and lights its LED`, peak > 0.05 && (ex.web_lit_keys() >> 12) & 1);
const F_GREEN = ((130 >> 3) << 11) | ((214 >> 2) << 5) | (56 >> 3);   // kid.c KID_NOTE_COL: F
const bandHas = (c) => { for (let y = 186; y < 240; y++) for (let x = 0; x < 240; x++) if (px(x, y) === c) return true; return false; };
check("its letter shows big in the band, in its colour (F green)", bandHas(F_GREEN));
ex.web_keys(0);
render(1200);
check("let go: the letter goes", !bandHas(F_GREEN));
check("the key ended the hello: the friend's name in the band", !hiRed() && new Set(band().split(",")).size >= 3);
const name0 = band();

ex.web_enc(EN.PRESETS, 1);
render(1700);
shot("friend2");
check("PRESETS: another friend (the band shows its name)", band() !== name0);

// the staff: a clef and five lines come in when she plays, her notes are coloured heads, a chord is stacked, the octave
// is marked (8VA), and it goes after about 6 s of quiet
{
  const INK = ((52 >> 3) << 11) | ((26 >> 2) << 5) | (58 >> 3);               // kid.c KID_INK
  const col = (r, g, b) => ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
  const C_RED = col(236, 40, 52), A_PUR = col(146, 84, 226), F_GR = col(130, 214, 56), G_TEAL = col(30, 176, 150);
  const lines = () => px(150, 24) === INK && px(150, 36) === INK && px(150, 48) === INK;
  const press = (k, ms = 120) => { ex.web_keys(1 << k); render(ms); ex.web_keys(0); render(250); };
  render(7000);
  check("no staff while she is not playing", !lines());
  press(12);                                                                   // F4: step 3, on the second space
  shot("staff1");
  check("a key: the staff comes in (five lines)", lines() && px(14 + 6, 40) !== 0);
  check("its note is a head in the note's colour (F green) on the staff", px(56, KID_Y(3)) === F_GR);
  press(14);                                                                   // G4
  check("the next key writes the next head to its right (G teal)", px(77, KID_Y(4)) === G_TEAL && px(56, KID_Y(3)) === F_GR);
  ex.web_enc(EN.ALGO, 3); render(100);
  press(12);                                                                   // F4 with CHORD: F A C stacked
  shot("staff2");
  check("a chord is stacked (F, A and C in their colours)", px(98, KID_Y(3)) === F_GR && px(98, KID_Y(5)) === A_PUR && px(98, KID_Y(7)) === C_RED);
  ex.web_enc(EN.ALGO, -3); render(100);
  press(0);                                                                    // F3: step -4, a ledger line under the staff
  check("a low note has its ledger lines (C4 below the staff)", px(125, KID_Y(0)) === INK && px(125, KID_Y(-2)) === INK && px(125, KID_Y(-4)) === INK);
  ex.web_buttons(1 << B.OCTUP); render(60); ex.web_buttons(0); render(250);
  press(12);
  shot("staff3");
  let mark = false;
  for (let y = 7; y < 16; y++) for (let x = 31; x < 50; x++) if (px(x, y) === INK) mark = true;
  check("an octave up: 8VA is written over the staff", mark);
  ex.web_buttons(1 << B.OCTDN); render(60); ex.web_buttons(0); render(250);
  render(6500);
  check("after about 6 s of quiet the staff goes", !lines());
  press(14);
  check("and starts again from the left when she plays again", lines() && px(56, KID_Y(4)) === G_TEAL && px(77, KID_Y(4)) !== G_TEAL);
  render(6500);
}

// note values (by how long a key is held, against the beat): a tap a sixteenth (a stem, two flags), about a beat a
// quarter (a stem, a filled head), two beats a half (an open head), longer a whole (an open head, no stem)
{
  const INK = ((52 >> 3) << 11) | ((26 >> 2) << 5) | (58 >> 3);
  render(7000);
  const hold = (k, ms) => { ex.web_keys(1 << k); render(ms); ex.web_keys(0); render(250); };
  hold(12, 90);                                                                // F4 (step 3), slot 0
  hold(12, 540);                                                               // slot 1 (DANCE: a beat is 536 ms)
  hold(12, 1150);                                                              // slot 2
  hold(12, 1900);                                                              // slot 3
  shot("values");
  const y = KID_Y(3), stem = (cx) => px(cx + 4, y - 12) === INK;
  const flags = (cx) => { let n = 0; for (let yy = y - 21; yy < y - 6; yy++) if ((yy - 24) % 6 && px(cx + 6, yy) === INK) n++; return n; };
  check(`a tap: a sixteenth (a stem and two flags: ${flags(56)} flag rows)`, stem(56) && flags(56) >= 4);
  check("about a beat: a quarter (a stem, no flag, a filled head)", stem(77) && flags(77) === 0 && px(77, y) !== WHITE);
  check("about two beats: a half (a stem, an open head)", stem(98) && px(98, y) === WHITE);
  check("longer: a whole (an open head, no stem)", !stem(119) && px(119, y) === WHITE);
  ex.web_keys(1 << 14); render(700);
  const q = px(140, KID_Y(4)) !== WHITE;
  render(800);
  check("held, the note grows as she holds it (a quarter, then a half)", q && px(140, KID_Y(4)) === WHITE);
  ex.web_keys(0); render(7000);
}

const levels = [];
for (let f = 0; f < 45; f++) {
  peak = 0;
  for (const k of [12, 14, 16]) { ex.web_keys(1 << k); render(500); ex.web_keys(0); render(150); }
  levels.push(peak);
  ex.web_enc(EN.PRESETS, 1);
  render(400);
}
const lo = Math.min(...levels), hi = Math.max(...levels);
check(`all 45 friends sound, alike (peaks ${lo.toFixed(3)} .. ${hi.toFixed(3)})`, lo > 0.08 && hi / lo < 2);
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

// a key pressed after an effect was tapped on (the effect heard silence): the note must still sound. HICCUP, BACKWARDS,
// SLEEPY and FREEZE keep what they last heard, which in a silence is nothing, unless each new key sets them again
{
  const wait = (ms) => render(ms);
  const heard = (btn) => {
    wait(1500);
    if (btn !== null) { ex.web_buttons(1 << btn); render(60); ex.web_buttons(0); render(500); }
    peak = 0;
    ex.web_keys(1 << 14); render(700);
    const p = peak;
    ex.web_keys(0); render(300);
    if (btn !== null) { ex.web_buttons(1 << btn); render(60); ex.web_buttons(0); }
    wait(3200);
    return p;
  };
  const plain = heard(null);
  const names = ["HICCUP", "BACKWARDS", "SLEEPY", "SQUEAKY", "GIANT", "FREEZE"];
  const r = [FXB.map((b) => heard(b))][0];
  r.forEach((p, i) => check(`${names[i]} tapped in a silence, then a key: the note sounds (${p.toFixed(3)} of ${plain.toFixed(3)})`, p > plain * 0.4));
}

// the grown-ups' VOLUME: HOME held 1 s opens it (VOLUME and its dots in the band), OCT- / OCT+ step it (eight steps),
// MASTER can go no higher, and the limiter's ceiling goes down with it
{
  const lvl = () => ex.web_master_q12();
  const capOf = { 1: 362, 2: 512, 3: 724, 4: 1024, 5: 1448, 6: 2048, 7: 2896, 8: 4080 };   // (the pot's top: 4080)
  const holdHome = () => { ex.web_buttons(1 << B.HOME); render(1300); };
  const step = (b) => { ex.web_buttons((1 << B.HOME) | (1 << b)); render(80); ex.web_buttons(1 << B.HOME); render(150); };
  render(2500);
  check("VOLUME 6 to begin with: MASTER all up gives 2048", lvl() === 2048);
  ex.web_buttons(1 << B.HOME); render(300);
  ex.web_buttons((1 << B.HOME) | (1 << B.OCTDN)); render(80); ex.web_buttons(0); render(300);
  check("HOME held a short while: OCT- is still the size, the volume stays", lvl() === 2048);
  render(3500);
  const name = band();
  holdHome();
  check("HOME held 1 s: the band shows VOLUME", band() !== name);
  shot("volume");
  step(B.OCTDN);
  check("..OCT-: one step down (1448)", lvl() === 1448);
  step(B.OCTUP);
  step(B.OCTUP);
  check("..OCT+ twice: two up (2896)", lvl() === 2896);
  for (let i = 0; i < 9; i++) step(B.OCTUP);
  check("..the top step is the full MASTER (4080 at the pot's top), and no more", lvl() === 4080);
  for (let i = 0; i < 12; i++) step(B.OCTDN);
  check("..the bottom step is 362, and no less", lvl() === 362);
  ex.web_buttons(0);
  render(2600);
  check("HOME let go after VOLUME: no surprise friend, the name stays", band() === name);
  // the limiter: a chord over the beat at every step stays under its ceiling, and is not squashed by it
  ex.web_enc(EN.ALGO, 3); render(100);
  ex.web_buttons(1 << B.PLAY); render(60); ex.web_buttons(0); render(300);
  const peaks = [];
  for (let L = 1; L <= 8; L++) {
    render(2200);
    const cap = lvl(), lim = ex.web_lim_t();
    check(`VOLUME ${L}: MASTER ${cap}, the limiter's ceiling ${(lim / 65536).toFixed(3)}`, cap === capOf[L] && lim <= 18000 && lim >= 400);
    peak = 0;
    for (const ks of [[12, 14, 16], [5, 7, 9], [20, 22, 24]]) { ex.web_keys(ks.reduce((a, x) => a | (1 << x), 0)); render(500); ex.web_keys(0); render(200); }
    peaks.push(peak);
    if (lim < 18000)    // (at the full Felucca's ceiling its own limiter works as it always has: a fast attack, not instant)
      check(`VOLUME ${L}: a chord over the beat peaks at ${peak.toFixed(3)}, under the ceiling`, peak <= lim / 65536 * 1.08);
    ex.web_buttons(1 << B.HOME); render(1300);
    step(B.OCTUP);
    ex.web_buttons(0); render(2200);
  }
  check(`the levels rise with the steps (${peaks.map((p) => p.toFixed(2)).join(" ")})`, peaks.every((p, i) => !i || p >= peaks[i - 1] * 0.95));
  // an effect that raised the level (here the gain after MASTER, 8 times) is held to the ceiling, at the quietest step and the middle
  for (const L of [1, 4]) {
    ex.web_buttons(1 << B.HOME); render(1300);
    for (let i = 0; i < 8; i++) step(B.OCTDN);
    for (let i = 1; i < L; i++) step(B.OCTUP);
    ex.web_buttons(0); render(2200);
    const lim = ex.web_lim_t() / 65536;
    ex.web_boost_q12(8 * 4096); render(300);
    peak = 0;
    for (const ks of [[12, 14, 16], [5, 7, 9], [20, 22, 24]]) { ex.web_keys(ks.reduce((a, x) => a | (1 << x), 0)); render(500); ex.web_keys(0); render(200); }
    const hot = peak;
    ex.web_boost_q12(4096); render(300);
    check(`VOLUME ${L}, the level 8 times too high: the limiter holds it to ${hot.toFixed(3)} (ceiling ${lim.toFixed(3)})`, hot <= lim * 1.15);
  }
  ex.web_buttons(1 << B.PLAY); render(60); ex.web_buttons(0); render(300);
  ex.web_enc(EN.ALGO, -3); render(100);
  // back to 6, and kept over a power-off
  ex.web_buttons(1 << B.HOME); render(1300);
  for (let i = 0; i < 8; i++) step(B.OCTUP);
  for (let i = 0; i < 2; i++) step(B.OCTDN);
  ex.web_buttons(0); render(2200);
  check("VOLUME back to 6 (2048)", lvl() === 2048);
  ex.web_buttons(1 << B.HOME); render(1300);
  step(B.OCTDN); step(B.OCTDN);
  ex.web_buttons(0); render(2200);
  check("VOLUME 4 (1024)", lvl() === 1024);
  const nor = new Uint8Array(mem.buffer, ex.web_nor(), ex.web_nor_size()).slice();
  const inst2 = (await WebAssembly.instantiate(fs.readFileSync(wasmPath), {})).instance, e2 = inst2.exports;
  e2._initialize();
  new Uint8Array(e2.memory.buffer, e2.web_nor(), e2.web_nor_size()).set(nor);
  e2.web_boot();
  e2.web_master(1023);
  for (let k = 0; k < 400; k++) e2.web_render(128);
  check("kept over a power-off: the next start has the same VOLUME (1024)", e2.web_master_q12() === 1024);
}

// WRITE (SAVE): her own song. A key writes a note (a friend in a ball of the note's color) and the column moves on, the
// same key again takes it out, HOME wipes the column, PLAY plays it back, it is kept over a power-off, REC held 2 s
// starts a new one, and SAVE goes back to playing
{
  const col = (r, g, b) => ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
  const C_RED = col(236, 40, 52), G_TEAL = col(30, 176, 150), A_PUR = col(146, 84, 226);
  const ball = (c, step) => px(46 + c * 23 + 10, 128 - 6 * step + 9);         // a pixel inside the ball, under the friend
  const tapb = (b, ms = 60) => { ex.web_buttons(1 << b); render(ms); ex.web_buttons(0); render(250); };
  const key = (k, ms = 150) => { ex.web_keys(1 << k); render(ms); ex.web_keys(0); render(200); };
  render(7000);
  const name = band();
  tapb(B.SAVE); render(300);
  shot("write0");
  check("SAVE: WRITE, a big staff and its word (SAVE lit)", band() !== name && ((ex.web_lit_buttons() >> B.SAVE) & 1) === 1);
  for (let i = 0; i < 3; i++) { tapb(B.REC, 2300); }                           // (a new song, whatever was there)
  key(7); key(7); key(14); key(16);                                           // C C G A
  shot("write1");
  check("keys write their notes, a column each (C red, C red, G teal, A purple)",
        ball(0, 0) === C_RED && ball(1, 0) === C_RED && ball(2, 4) === G_TEAL && ball(3, 5) === A_PUR);
  tapb(B.OCTDN);                                                              // back to column 3 (the A)
  key(16);
  check("the same key again takes its note out", ball(3, 5) !== A_PUR);
  key(14);                                                                    // a G there instead, then column 4
  tapb(B.OCTDN); tapb(B.HOME);                                                // HOME wipes column 3
  check("HOME wipes the column", ball(3, 4) !== G_TEAL && ball(2, 4) === G_TEAL);
  ex.web_keys((1 << 7) | (1 << 11)); render(150); ex.web_keys(0); render(250);   // C and E together: one column
  check("two keys together: a chord in one column", ball(3, 0) === C_RED && ball(3, 2) !== ball(3, 1));
  peak = 0;
  tapb(B.PLAY); render(2200);
  shot("write2");
  check(`PLAY: her song plays (peak ${peak.toFixed(3)})`, ex.web_playing() === 1 && peak > 0.05);
  tapb(B.PLAY); render(5000);
  const nor = new Uint8Array(mem.buffer, ex.web_nor(), ex.web_nor_size()).slice();
  const e2 = (await WebAssembly.instantiate(fs.readFileSync(wasmPath), {})).instance.exports;
  e2._initialize();
  new Uint8Array(e2.memory.buffer, e2.web_nor(), e2.web_nor_size()).set(nor);
  e2.web_boot(); e2.web_master(1023);
  const r2 = (ms) => { for (let k = 0; k < Math.round(ms * 44.1); k += 128) e2.web_render(128); };
  r2(1500); e2.web_keys(1 << 20); r2(100); e2.web_keys(0); r2(1500); e2.web_buttons(1 << B.SAVE); r2(60); e2.web_buttons(0); r2(600);
  const p2 = (x, y) => { const v = new Uint16Array(e2.memory.buffer, e2.web_screen(), 240 * 240)[y * 240 + x]; return ((v & 255) << 8) | (v >> 8); };
  check("kept over a power-off: the next start's WRITE has her song", p2(46 + 10, 128 + 9) === C_RED && p2(46 + 2 * 23 + 10, 128 - 24 + 9) === G_TEAL);
  tapb(B.REC, 2300);
  check("REC held 2 s: a new, empty song", ball(0, 0) !== C_RED && ball(2, 4) !== G_TEAL);
  tapb(B.SAVE); render(1800);
  check("SAVE again: back to playing (the friend's name, SAVE dark)", !((ex.web_lit_buttons() >> B.SAVE) & 1) && band() !== "");
  key(12); render(300);
  check("and the keys play as before", peak > 0.05);
}

// SEQ: ten beats, each named in the band; the friend's bass line plays with them
{
  const seen = new Set();
  for (let i = 0; i < 11; i++) { ex.web_buttons(1 << B.SEQ); render(60); ex.web_buttons(0); render(200); seen.add(band()); }
  check(`SEQ steps through eleven beats (${seen.size} different names)`, seen.size === 11);
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
