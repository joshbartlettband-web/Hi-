# Rainbow mode

This fork of Felucca turns the FM-1 into a music toy for a four-year-old. It starts in Rainbow mode every
time it powers on, with a hello in bouncing rainbow letters and confetti that greets her by name, for a few
seconds or until she presses something. A grown-up can get back to the full Felucca by holding **HOME and
SAVE together for 3 seconds**. That brings back the music of your last session: Rainbow mode never
writes Felucca's autosave, so playing with the toy can't overwrite your own work. Rainbow mode comes back
at the next power-on.

## What everything does

| Control | What happens |
| --- | --- |
| Keys | Play the friend's sound. The note's letter shows big in the band at the bottom, in its own colour, and the friend hops. |
| PRESETS | Next or previous friend (40 of them, each with its own picture, sound, home sky and favourite beat). |
| ALGORITHM | Right: three friends sing (one key plays a chord that is always in key). Left: back to one. |
| SELECT | The beat slower or faster (SLOW, WALK, FAST). |
| KNOB 1 | Big and small. Low notes make a big friend, high notes a small one. OCT- and OCT+ do the same. |
| KNOB 2 | Day to night. The sky darkens, the sun sets, the moon comes out, and the sound gets softer and darker. |
| KNOB 3 | Echo. The sound repeats, and copies of the friend follow it around. |
| KNOB 4 | Wiggle. The sound wobbles (vibrato) and so does the friend. |
| FX | Tap: HICCUP. The sound stutters and the friend shakes. |
| SCL | Tap: BACKWARDS. The sound plays in reverse and the friend turns round. |
| ENV | Tap: SLEEPY. The sound winds down like a record player and the friend droops and dozes, then wakes up by itself after 3 seconds. |
| LFO | Tap: SQUEAKY. Everything an octave up, and the friend shrinks. |
| EDIT | Tap: GIANT. Everything an octave down, and the friend grows. |
| GLO | Tap: FREEZE. The sound freezes in place and the friend turns to ice. |
| | The top row's effects turn on with a tap and off with another (one at a time; the lit button shows which). |
| PLAY | Starts and stops the beat: drums and a bass line that is always in key, so anything she plays fits. The friend dances. |
| SEQ | The next beat: DANCE, MARCH, SPOOKY, ROCK, DISCO, HIP HOP, TRAIN, SAMBA, REGGAE, LULLABY. |
| ARP | Sparkle: a held key plays up and down by itself. |
| REC | The next sky (rainbow, stars, hearts, bubbles, flowers, confetti), with confetti. |
| HOME | A surprise friend, with confetti. |
| SAVE | A confetti party. |
| MASTER | Volume. It is capped at about half in Rainbow mode, for small ears. |

Note colours follow the coloured tubes and bells used in many early music classes: C red, D orange,
E yellow, F green, G teal, A purple, B pink. Sharps get the colour in between.

## The friends

Ducky, Pink Ducky, Cool Ducky, Axolotl, Unicorn, Giraffe, Goo, Goobert, Blue Pup, Red Monster,
Blue Monster, April (the family dog), Scissors, Ghost, Web Hero, Butterfly, Kitty, Frog, Robot, Rainbow,
Princess Ducky, Red Pup, Yellow Bird, Slimy, Bunny, Panda, Penguin, Owl, Bee, Ladybug, Dino, Whale,
Octopus, Fish, Turtle, Ice Cream, Cupcake, Rocket, Strawberry, Star.

Each one uses one of Felucca's own factory sounds (listed in `firmware/src/kid.c`, `KID_SOUND`), with its
level set so they all play at about the same loudness. Each also has a home sky and a favourite beat
(Ghost and Slimy: SPOOKY, a ghost-hunting funk; Frog and Axolotl: REGGAE; Butterfly and Rainbow: LULLABY). Scissors
plays the drum kit: every key is a different drum.

The pictures are drawn on a roomy canvas, then trimmed, centred and stood on the bottom of a 48 x 48 grid;
a picture that would not fit stops the build, so none is ever cut off. On the screen they are drawn at 1x,
1.5x, 2x, 2.5x or 3x (KNOB 1) and kept inside the screen even at their biggest, hopping and wiggling.

The characters are original drawings in the spirit of the ones she loves, not copies, because this
repository and its website are public.

## Where it lives

- `tools/gen_kid_art.py`: the 40 pictures, drawn from shapes on a 48 x 48 grid and outlined
  automatically. `python3 tools/gen_kid_art.py /tmp/kid_art.h --png /tmp/sheet.png` writes a contact
  sheet to look at. The build runs it (`tools/build.py` generate).
- `firmware/src/kid.c`: everything else (sounds, knobs, buttons, the screen). It takes over the main
  loop's frame while it is on (`firmware/src/main.c`). Build with `FELUCCA_KID=0` to leave it out.
- `web/emu/kid_test.mjs`: checks it in the browser build (`node web/emu/kid_test.mjs build/emu/felucca.wasm`).
- `landing/`: the site's front page (the one link to share): a demo video, screenshots, the friends and how to try it. The workflow
  copies it to the site's root and fills in the site's address.
- `.github/workflows/rainbow.yml`: GitHub builds the firmware and the browser version on every push to
  `main`, tests the browser version, and publishes both to this fork's GitHub Pages site:
  `/webapp/try/` plays it in the browser, `/webapp/installer/` puts it on the FM-1.

## The name in the hello, and the private installer

The name is not in this repository, and the installer on the public site has none in it: its hello is a plain HI!, the same as the
browser demo. Two repository secrets (**Settings → Secrets and variables → Actions**) add a second, named build:

- `KID_NAME`: the name the hello greets, capitals only, at most 10 letters (`tools/gen_kid_art.py` reads it).
- `PRIVATE_PATH`: a long random word, 12 to 64 letters, digits, `-` or `_`.

With both set, the workflow builds a second firmware package that greets by name and puts its installer at
`https://USER.github.io/REPO/PRIVATE_PATH/`. That address is not linked from anywhere and is marked noindex, but it is not password
protected: anyone who learns it can open it, so it is for your own use, not for posting. The name is held scrambled inside the
firmware package (the FM-1's package format encodes the app), not as readable text, and GitHub hides both secrets in its logs.
With only one of them set, there is no private build.

## Not checked yet on a real FM-1

- Drawing speed. The screen has no frame buffer, so moving pictures are drawn in strips. Animated skies
  (stars, hearts, bubbles, confetti) redraw the whole picture area, about 60 ms a frame over the screen's
  connection. If it feels slow on the device, the first thing to try is drawing only the friend's area.
- The volume cap (`KID_VOL_MAX` in `kid.c`): half of the MASTER curve. Raise or lower it to taste.
