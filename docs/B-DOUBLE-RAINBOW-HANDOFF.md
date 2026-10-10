# B Double Rainbow: what Rainbow mode taught us

A hand-off for the next project: **B Double Rainbow**, a firmware for the M-VAVE FM-1 aimed at kids around eight, built on
**Melodee** (Kerem Kilic's Felucca fork, https://github.com/keremimo/melodee) from a fresh start. Nothing here has been built
yet. This file collects what we learned making Rainbow mode (this repo) so the new project starts ahead.

Read it with `RAINBOW.md` (how Rainbow mode works) and `CREDITS.md` (who suggested what) in this repo.

---

## 1. Working with Josh

- **Writing:** plain and human, American spelling, few em dashes. Short paragraphs, real numbers, no hype.
- **Talk before building** anything open-ended (a new mode, a new character set, a new project direction). Small, clear fixes
  can just be built, then reported.
- **Privacy:** his daughter's name never goes in the repo, commits, public site or public installer. It lives only in a
  repository secret (`KID_NAME`) and comes out only in a private build at a private address (`PRIVATE_PATH`, also a secret).
  Never print either value. Do the same in the new project from day one.
- **Credit the community.** Every idea from r/MVaveFM1 gets the person's username in `CREDITS.md`, on the website and on the
  installer page. People notice and come back.
- **Characters:** original drawings only. If someone asks for a known character, draw an original in its spirit, and keep a
  grown-ups' switch that swaps look-alikes for unrelated originals (see section 6).
- **He tests on a real FM-1 and an Android phone.** He can hear things the tests can't. Ask him to listen when the change is
  about sound, and believe him when he says something sounds wrong (the vibrato bug below was his catch).
- **Video sells it.** Reddit posts with a short demo video got far more engagement than text posts.

## 2. The hardware and its limits

- JieLi AC79-family chip. 240 x 240 RGB565 screen with **no frame buffer**: the screen is drawn in row strips, one being
  computed while the other is sent. Every pixel comes from a function of (x, y). Big animated areas cost drawing time.
- 27 keys, F3 to G5 (key k plays MIDI 53 + k at octave 0). 11 black keys: F#3 G#3 A#3 C#4 D#4, F#4 G#4 A#4 C#5 D#5, F#5.
- Buttons: FX SCL ENV LFO EDIT GLO (top row), HOME, SAVE, ARP, SEQ, PLAY, REC, OCT- and OCT+. Encoders: SELECT, ALGORITHM,
  PRESETS, KNOB 1 to 4. MASTER is a pot. Every key and button has an LED.
- USB: class-compliant MIDI and a stereo USB audio input. TRS MIDI in. **Bluetooth works only on M-VAVE's own firmware**;
  Felucca and its forks have no Bluetooth code (the radio needs JieLi's closed SDK).
- **Space is the real limit.** Rainbow mode on Felucca: image 566,028 B of a 581,564 B app slot (97% full); RAM .data+.bss
  92,436 B of 98,304; pool 331,204 B of 344,064.
- **Melodee's app slot is smaller: 540,604 B** (`APP_SLOT = 0x83FBC` in its `tools/fm1pkg_make.py`), and Melodee carries more
  (PROPHET, CZ-1, Dexed-based FM6). **Measure Melodee's image size before planning anything.** Its build prints
  `image N B` and checks against the slot.
- The friend pictures were Rainbow mode's biggest own cost: 69 pictures at 1,152 B each (48 x 48, 4 bits a pixel) = 79 KB.
  Simple run-length coding shrinks them to about 31 KB. Decode the shown friend into a small RAM buffer (or a cache of four
  for WRITE's band). Do this from the start in the new project.
- The browser emulator's CPU time says nothing about the device. Watch for heavy engines (PROPHET costs three voice units per
  note) and test on the FM-1.

## 3. Melodee: what to know before forking

- **License:** GPL-3.0-only, same as Felucca. Credit "Melodee by Kerem Kilic (Ellic Studio), a modified version of Felucca by
  Leo Kuroshita, Hügelton Instruments" everywhere Rainbow mode credits Felucca.
- **Sequential's Prophet-5 factory programs** (`assets/prophet5-factory/prophet5-v1.03.syx`, 200 programs) are in Melodee's
  tree with **no license granted** (Melodee's own LICENSING.md says the rights stay with Sequential). A public fork inherits
  that file. Decide with Josh before the first push: keep it as Melodee does, or strip it and ship our own programs. Either
  way, do not make new copies of Sequential's sounds.
- **Engines:** PROPHET replaces ANALOG in browsing (ANALOG still renders old sounds). **TRIO, WHEEL and PHYS are retired and
  silent.** 18 of Rainbow mode's 58 friends used them (Goo, Goobert, Kitty, Cowboy, Space Hero and more), so any port of
  Rainbow sounds needs new voicings.
- **Voices:** a shared budget of 16 units (`VBUDGET`). A PROPHET voice costs 3 units, so five PROPHET voices use 15. Three-note
  chords plus a bass and drums will steal voices. Plan chord features around this.
- **Settings bytes:** Melodee already uses `favorites.factory[14]` (bytes 0 to 31: scale favorites, `favorites.c`) and
  `favorites.factory[15][31]` (layers seen, `ui_layer.c`). **Rainbow mode stored its WRITE song in exactly those bytes**
  (`factory[14][0..31]` and `[15][0..25]`), its VOLUME and look-alike switch in `[15][26]`. Audit every spare byte in
  Melodee before storing anything, and validate it in `settings_persist.c` like Rainbow mode does (unknown values fall back
  to defaults, so old installs never break).
- **USB name and identity:** Melodee's USB MIDI port is named **"Melodee"**, and its package identity is `FM-1_9xx`. Our
  installer only found FM-1s whose port name matched `fm-1|felucca|ota|...`; we added `melodee`. The new project needs its
  own name in the regex (`web/fm1ota.js`, `Updater.find`) or installs will report "FM-1 not found".
- **No browser emulator and no GitHub Actions build** in Melodee (it builds with `build.sh` locally). Rainbow mode's browser
  build (`web/emu/`) and CI workflow (`.github/workflows/rainbow.yml`) are worth porting first; they made everything else
  testable from the cloud.
- Melodee releases often. Keep the kid code in as few files as possible (Rainbow mode is one file, `kid.c`, plus small hooks)
  so merging Melodee updates stays easy.

## 4. How Rainbow mode is put together (the parts worth copying)

- **One compilation unit** (`felucca.c` includes everything). `kid.c` is included after `ui.c`; seq.c and fx.c come earlier.
- **Hooks into the firmware, kept small:** `kid_frame()` takes the main loop's frame while the toy is on; `kid_master()` runs
  after `master_poll()` (volume cap and limiter ceiling); `seq.c` has tiny hooks (`kb_strum`, `kb_accord`, `accord_chord`,
  `kb_accord_trk`). Everything else lives in `kid.c`.
- **Friends** are data: `KID_SOUND[]` names an engine, a factory preset by name, a level, a home sky, a favorite beat, and
  optional vibrato, glide, drive, drum kit and a list of parameter tweaks (`TW_*`). Pictures come from
  `tools/gen_kid_art.py`, which draws shapes, outlines them, trims them and **stops the build if a picture would be cut
  off**. It also emits the look-alike stand-ins (`STAND_INS` -> `KID_ALT`).
- **Modes** on ALGORITHM (1 FRIEND, 3 FRIENDS, STRUM, ACCORDION, GUESS, FOLLOW), each with a picture for children who do not
  read. WRITE is on SAVE (a Mario Paint style composer). VOLUME and LOOK-ALIKES are grown-up settings behind HOME held 1 s.
- **Settings that survive power-off** are packed into spare bytes of the saved settings, never into the user's projects.

## 5. Bugs and lessons (each one cost a round trip)

1. **Mono presets swallow chords.** Presets flagged mono load as LEGATO, so chord notes collapse to one. Force POLY whenever a
   mode plays chords; keep the preset's own mode for single notes (its slides are part of its character).
2. **Engines with a small voice cap.** PHYS and GRAIN play three notes at once. Four-note chords lost a note. Check
   `engine_t.poly` and fall back to a three-note shell (root, 3rd, 7th).
3. **Vibrato scale.** One step of `P_LD_PIT` is about +-19 cents. Values of 5 to 12 gave a vibrato of one to two semitones.
   Use 1 or 2 for a singer, 3 for a theremin. Measure it (pitch-track a held note) instead of guessing.
4. **Loudness by ear, not by peak.** A pluck and an organ with the same peak are far apart to the ear. Rainbow mode levels
   every friend by measured loudness (BS.1770 style, K-weighted); the spread went from 12 dB to under 4. `P_LEVEL` steps are
   half a dB. Re-measure after every sound change, and keep a test that fails if a friend drifts.
5. **The limiter.** Felucca's output scale is internal/65536, not /32768. The ceiling is 15 x the MASTER level; attack must
   be instant in kid mode or drums overshoot.
6. **Drum friends.** Engines with their own key map (`e->keys`, DRUM) bypass the keyboard hooks (strum, accordion). Handle
   them separately everywhere: no staff, no chord names, one drum a key, and WRITE stores their lanes, not pitches.
7. **Installer on Android.** Links opened inside Reddit or Gmail open an in-app browser that cannot reach USB MIDI. The error
   message now says to open the page in Chrome itself. The installer also logs every MIDI port it sees and what each one
   answered, which found the Melodee name problem in one try. Keep that diagnostic.
8. **Editing a big C file.** Use exact-match replacements that assert the match count. Never write a file with
   `open(path, "w").write(open(path).read())` in one line: it empties the file before reading it (this truncated `kid.c` once).
9. **Test the test.** After writing a check, break the feature on purpose and watch the check fail.
10. **Recording videos:** turning an encoder more than once per emulator frame drops steps. Step one detent a frame until the
    target is reached (see `go()` in `tools/video/record_tour.mjs`).

## 6. Characters and the look-alike rule

- Draw originals. Rainbow mode's look-alikes (two pups, two monsters, Web Hero, Slimy, Yellow Bird, Skeleton, Cowboy,
  Cowgirl, Space Hero) are "in the spirit of" characters kids love, and a parent asked for a way to turn that off.
- The answer that worked: a grown-ups' switch that **swaps** each look-alike for an unrelated original with the same sound,
  sky and beat (Spotty Pup, Fox, Fuzzy, Big Blue, Spider, Pumpkin, Chick, Bat, Horse, Cactus, Alien). Hiding them instead
  lost sounds people liked. Build the swap in from the start.
- Watch for accidental look-alikes in new art: a red race car with a face reads as Lightning McQueen; a round blue creature
  with big ears reads as Stitch. Change color and features until it reads as its own thing.

## 7. Testing and tools to bring along

- `web/emu/` builds the firmware for the browser with Emscripten 3.1.74 (`EMU_PREVIEW=0 sh web/emu/build.sh`) and exports
  test hooks (`web_keys`, `web_buttons`, `web_enc`, `web_render`, `web_screen`, `web_voices`, `web_kid_friend` and more).
- `web/emu/kid_test.mjs` drives the toy like a child would and checks the screen pixels and the audio (about 100 checks:
  every friend sounds and stays within the loudness spread, chords have their voices, settings survive a simulated
  power-off by copying the emulated flash into a fresh instance).
- `.github/workflows/rainbow.yml` builds the public firmware, the private one (only with both secrets set), the browser
  build, runs the tests, and publishes the landing page, the browser version and the installer to GitHub Pages. It caches
  the JieLi toolchain and SDK (`AC79NN_SDK_V1.2.1_2023-12-13`) and tolerates a messy `DONATE_URL` secret.
- **Demo videos** are made from the emulator, not a camera: a script presses keys on a timeline and saves 30 fps frames, the
  audio and the keys held; a compositor adds titles and a keyboard that lights up in note colors, then ffmpeg encodes it.
  This takes minutes and always matches the current build. The scripts are `tools/video/record_tour.mjs` and
  `tools/video/compose_tour.py`.

## 8. Ideas for B Double Rainbow (from the planning chat, not yet agreed)

For a kid of about eight, roughly in order of delight per effort:

1. **Sound Lab:** knobs with pictures (wave shape, a mouth for the filter, a hill for the envelope) to build her own
   creature's sound, name it on the keys, and keep it as her friend. Real synthesis in disguise. On Melodee, PROPHET's
   two oscillators, filter and envelopes are a natural fit, if the voice budget allows.
2. **Beat Maker:** a 16-step drum grid on the keys with lights, then jam over the loop.
3. **Band Builder:** record a tune, it loops, add a bass, add chords.
4. **Copy-Me:** a friend plays a short melody or rhythm, she repeats it; it grows each round and earns stars.
5. **Unlocks:** new friends, skies or chord rows earned by playing.
6. **Theory in color:** major and minor as happy and sad skies, a scale picker, measures and note values (already in Rainbow
   mode's staff).
7. **A chord course (agreed: "both"):** Rainbow mode has a first version, CHORDS on ALGORITHM (u/Yablan's request for
   "really basic lofi chords"): five 4-chord lessons from FIRST CHORDS to LOFI JAZZ, the chord's keys lit, its name and
   letters in the band, YES on exactly its keys, and a swung LOFI beat whose bass plays each chord's root, a bar a chord.
   See `KID_LESSONS` and `kid_lesson_chord` in `kid.c`. Grow it here: fingering numbers, longer progressions, a score for
   chords played in time, voice-leading hints (which finger moves), inversions as their own lessons, and a "find this
   chord" quiz. Adults asked for this too, so keep it usable without the kid layer.

Questions still open for Josh: what the girls like (songs, characters, games), whether each has her own FM-1, and which two
or three ideas to build first.

## 9. Suggested first steps

1. Fork Melodee. Decide about Sequential's `.syx` before the first public push.
2. Port the CI workflow and get an unmodified Melodee build installing from the website. Confirm Josh can flash it.
3. Port the browser emulator and a first test. Check the installer finds the new port name.
4. Measure Melodee's image size against its 540,604 B slot. Decide what the new project can spend.
5. Audit spare settings bytes. Write the privacy-safe `KID_NAME` hello.
6. Only then start on the first feature, after talking it through with Josh.

## 10. Community and outreach notes

- Contributors so far (credit them if their ideas carry over): u/veecheech, u/theskyisfalling1, u/lastapoc, u/nutty_cartoon,
  u/ReallyLongLake.
- Good places for news: r/MVaveFM1, the Elektronauts FM-1 thread, Felucca's GitHub Discussions, Synthtopia's submit-a-story
  page, MatrixSynth, Sonicstate (news@sonicstate.com), Hackaday tips.
- Music teachers: Midnight Music (Katie Wardrobe, Amy Burns), the Elementary Music Teachers Facebook group, r/MusicEd, Orff
  (AOSA and the Great Plains chapter), Kodály (OAKE), the Bob Moog Foundation's Dr. Bob's SoundSchool, and the Nebraska Music
  Educators Association conference (proposals usually January to March).
- A video, the browser link (no FM-1 needed to try it) and a one-page lesson idea get teachers' attention.
