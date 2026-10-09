// Rainbow mode, a fresh tour: raw RGB frames (30 fps), a WAV, captions, and the keys held on each frame.
//   node tools/video/record_tour.mjs build/emu/felucca.wasm OUT_DIR   then compose_tour.py in OUT_DIR
import fs from "fs";
const [wasmPath, out] = process.argv.slice(2);
const B = { FX: 0, SCL: 1, ENV: 2, LFO: 3, EDIT: 4, GLO: 5, HOME: 6, SAVE: 7, ARP: 8, SEQ: 9, PLAY: 10, REC: 11, OCTDN: 12, OCTUP: 13 };
const EN = { SELECT: 0, ALGO: 1, PRESETS: 2, K1: 3, K2: 4, K3: 5, K4: 6 };
const K = { F3: 0, G3: 2, A3: 4, B3: 6, C: 7, D: 9, E: 11, F: 12, G: 14, A: 16, B: 18, C2: 19, D2: 21, E2: 23, F2: 24, G2: 26 };
const { instance } = await WebAssembly.instantiate(fs.readFileSync(wasmPath), {});
const ex = instance.exports, mem = ex.memory;
ex._initialize(); ex.web_nor_erase(); ex.web_boot(); ex.web_master(1023);
let keysNow = 0;
const setKeys = (m) => { keysNow = m; ex.web_keys(m); };
const ev = [], cap = [];
const at = (t, f) => ev.push([t, f]);
const C = (t, a, b) => cap.push([t, a, b]);
const note = (t, k, len) => { at(t, () => setKeys(1 << k)); at(t + len, () => setKeys(0)); };
const hold = (t, keys, len) => { at(t, () => setKeys(keys.reduce((a, k) => a | (1 << k), 0))); at(t + len, () => setKeys(0)); };
const tap = (t, b) => { at(t, () => ex.web_buttons(1 << b)); at(t + 80, () => ex.web_buttons(0)); };
const turn = (t, r, n, gap = 120) => { for (let i = 0; i < Math.abs(n); i++) at(t + i * gap, () => ex.web_enc(r, Math.sign(n))); };
const tune = (t, keys, step) => keys.forEach((k, i) => k !== null && note(t + i * step, k, step - 60));
let navTo = null;
const go = (t, f) => at(t, () => { navTo = f; });
const Q = 500;                                                     // a beat at 120 BPM

// cold open: a tune, straight away
go(0, 4);                                                          // UNICORN
C(0, "Rainbow mode", "a free music toy for little hands, on the M-VAVE FM-1");
tune(700, [K.E, K.E, K.F, K.G, K.G, K.F, K.E, K.D, K.C, K.C, K.D, K.E, K.E, K.D, K.D], 290);
// 1. see the music
let t = 5400;
C(t, "1. See the music", "every note has its color, and the staff writes it down");
[[K.C, 1], [K.E, 1], [K.G, 1], [K.E, 1], [K.C2, 2], [K.G, 2]].reduce((s, [k, b]) => { note(s, k, Q * b * 0.95 - 10); return s + Q * b; }, t + 300);
C(t + 2900, "1. See the music", "long notes look long, and every four beats a barline");
t += 6600;
// 2. the friends
const FR = [[40, "a T-Rex with a growl", (s) => tune(s, [K.C, K.E, K.G, K.E], 260)],
            [50, "an elephant with Prophet-style brass", (s) => { hold(s, [K.C, K.E, K.G], 500); hold(s + 600, [K.D, K.F, K.A], 700); }],
            [51, "a race car with a sync sweep", (s) => tune(s, [K.C, K.G, K.C2, K.E2], 300)],
            [54, "a snowman with glassy bells", (s) => tune(s, [K.E2, K.C2, K.G, K.C2], 300)],
            [55, "a crab with a snappy clav", (s) => tune(s, [K.C, null, K.E, K.G, null, K.A], 190)]];
FR.forEach(([f, sub, play], i) => { go(t, f); C(t, "2. Meet 58 friends", sub); play(t + 400); t += 1900; });
// 3. chords, one hand
go(t, 0);                                                          // DUCKY: an electric piano
turn(t + 100, EN.ALGO, 1);                                         // 3 FRIENDS
C(t, "3. Chords with one finger", "3 FRIENDS: one key, a whole chord, and its name");
note(t + 600, K.C, 600); note(t + 1300, K.F, 600); note(t + 2000, K.G, 700);
t += 3000;
turn(t, EN.ALGO, 1);                                               // STRUM
C(t, "3. Chords with one finger", "STRUM: black keys pick a chord, white keys strum it");
const sweep = (s, step) => [0, 2, 4, 6, 7, 9, 11, 12, 14, 16, 18, 19].forEach((k, i) => note(s + i * step, k, step + 30));
note(t + 400, 8, 300); sweep(t + 700, 70);
note(t + 1700, 3, 300); sweep(t + 2000, 70);
t += 3200;
turn(t, EN.ALGO, 1);                                               // ACCORDION
C(t, "3. Chords with one finger", "ACCORDION: chords on the black keys, jazz ones higher up");
hold(t + 400, [8], 700);                                           // C
hold(t + 1200, [22], 700); hold(t + 2000, [15], 700); hold(t + 2800, [20], 900);   // Dm9 G9 Cmaj9
tap(t + 3900, B.ARP);                                              // SPARKLE TUNE
C(t + 3900, "3. Chords with one finger", "SPARKLE TUNE: the tune twinkles, the chord holds");
hold(t + 4300, [8, K.C2], 700); hold(t + 5000, [8, K.E2], 700); hold(t + 5700, [1, K.F2], 900);
tap(t + 6800, B.ARP); tap(t + 7000, B.ARP);                        // (SPARKLE ALL, then off)
t += 7400;
// 4. games
turn(t, EN.ALGO, 1);                                               // GUESS
C(t, "4. Games for the ears", "GUESS: a friend sings a note. Can you find it?");
at(t + 2600, () => setKeys(1 << ex.web_kid_guess())); at(t + 2900, () => setKeys(0));
C(t + 2700, "4. Games for the ears", "found it!");
t += 4200;
turn(t, EN.ALGO, 1);                                               // FOLLOW
C(t, "4. Games for the ears", "FOLLOW: the next key lights up, at her own pace");
tune(t + 900, [K.C, K.C, K.G, K.G, K.A, K.A, K.G, null], 380);
t += 4300;
// 5. write a song
turn(t, EN.ALGO, -5, 80);
tap(t + 600, B.SAVE);
C(t, "5. Write a song", "SAVE: each key puts a friend on the staff");
tune(t + 1000, [K.C, K.E, K.G, K.E, K.F, K.A, K.G, null], 330);
tap(t + 4000, B.PLAY);
C(t + 4000, "5. Write a song", "PLAY: the friends sing it back, four beats a bar");
t += 7600;
tap(t, B.PLAY); tap(t + 300, B.SAVE);
// the summary, over a beat and three friends
turn(t + 500, EN.ALGO, 1);
tap(t + 900, B.PLAY);
C(t, "New in the last few builds", "__LIST__");
const items = 7, gap = 1500;
tune(t + 1200, [K.C, null, K.E, null, K.F, null, K.G, null, K.A, null, K.G, null, K.E, null, K.C], 400);
t += 1000 + items * gap + 1500;
C(t, "What should we add next?", "__END__");
tune(t + 300, [K.E, K.G, K.C2, null, K.G, K.C2], 330);
tap(t + 3600, B.PLAY);
const END = t + 5200;

ev.sort((a, b) => a[0] - b[0]);
const FPS = 30, SPF = 1470, frames = Math.ceil(END / 1000 * FPS);
const raw = fs.openSync(`${out}/frames.rgb`, "w"), pcm = [], keys = [];
let e = 0;
for (let f = 0; f < frames; f++) {
  const tt = f * 1000 / FPS;
  while (e < ev.length && ev[e][0] <= tt) ev[e++][1]();
  if (navTo !== null) {
    const cur = ex.web_kid_friend();
    if (cur === navTo) navTo = null; else ex.web_enc(EN.PRESETS, (navTo - cur + 58) % 58 <= 29 ? 1 : -1);
  }
  for (let k = 0; k < SPF; k += 147) {
    ex.web_render(147);
    const l = new Float32Array(mem.buffer, ex.web_out_l(), 147), r = new Float32Array(mem.buffer, ex.web_out_r(), 147);
    for (let i = 0; i < 147; i++) pcm.push(l[i], r[i]);
  }
  keys.push([keysNow, ex.web_lit_keys()]);
  const fb = new Uint16Array(mem.buffer, ex.web_screen(), 240 * 240), b = Buffer.alloc(240 * 240 * 3);
  for (let i = 0; i < fb.length; i++) {
    const v = ((fb[i] & 255) << 8) | (fb[i] >> 8);
    b[i * 3] = ((v >> 11) << 3) | (v >> 13); b[i * 3 + 1] = (((v >> 5) & 63) << 2) | ((v >> 9) & 3); b[i * 3 + 2] = ((v & 31) << 3) | ((v >> 2) & 7);
  }
  fs.writeSync(raw, b);
}
fs.closeSync(raw);
let pk = 0; for (const v of pcm) pk = Math.max(pk, Math.abs(v));
const g = 0.89 / pk, wav = Buffer.alloc(44 + pcm.length * 2);
wav.write("RIFF", 0); wav.writeUInt32LE(36 + pcm.length * 2, 4); wav.write("WAVEfmt ", 8); wav.writeUInt32LE(16, 16);
wav.writeUInt16LE(1, 20); wav.writeUInt16LE(2, 22); wav.writeUInt32LE(44100, 24); wav.writeUInt32LE(44100 * 4, 28);
wav.writeUInt16LE(4, 32); wav.writeUInt16LE(16, 34); wav.write("data", 36); wav.writeUInt32LE(pcm.length * 2, 40);
pcm.forEach((v, i) => wav.writeInt16LE(Math.max(-32767, Math.min(32767, Math.round(v * g * 32767))), 44 + i * 2));
fs.writeFileSync(`${out}/audio.wav`, wav);
fs.writeFileSync(`${out}/captions.json`, JSON.stringify({ fps: FPS, frames, captions: cap, keys, list_gap: gap }));
console.log(`frames ${frames}, audio ${(pcm.length / 2 / 44100).toFixed(1)} s, peak ${pk.toFixed(3)} (gain ${g.toFixed(2)})`);
