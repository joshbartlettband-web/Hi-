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
| Keys | Play the friend's sound. The note's letter shows big in the band at the bottom, in its own color, and the friend hops. A music staff drops in at the top and writes the note (see below). |
| PRESETS | Next or previous friend (50 of them, each with its own picture, sound, home sky and favorite beat). |
| ALGORITHM | How the keys play, one step per turn (below): 1 FRIEND, 3 FRIENDS, STRUM, ACCORDION, GUESS, FOLLOW. |
| SELECT | The beat slower or faster (SLOW, WALK, FAST). |
| KNOB 1 | Big and small. Low notes make a big friend, high notes a small one. OCT- and OCT+ do the same. |
| KNOB 2 | Day to night. The sky darkens, the sun sets, the moon comes out, and the sound gets softer and darker. |
| KNOB 3 | Echo. The sound repeats, and copies of the friend follow it around. |
| KNOB 4 | Wiggle. The sound wobbles (vibrato) and so does the friend. |
| FX | Tap: HICCUP. The sound stutters and the friend shakes. |
| SCL | Tap: BACKWARDS. The sound plays in reverse and the friend turns round. |
| ENV | Tap: SLEEPY. The sound winds down like a record player each time she plays a note, and the friend droops and dozes. It wakes up by itself after 3 seconds. |
| LFO | Tap: SQUEAKY. Everything an octave up, and the friend shrinks. |
| EDIT | Tap: GIANT. Everything an octave down, and the friend grows. |
| GLO | Tap: FREEZE. The sound freezes in place and the friend turns to ice. |
| | The top row's effects turn on with a tap and off with another (one at a time; the lit button shows which). HICCUP, BACKWARDS, SLEEPY and FREEZE start again on every new note, so a key pressed after the tap always sounds. |
| PLAY | Starts and stops the beat: drums and a bass line that is always in key, so anything she plays fits. The friend dances. |
| SEQ | The next beat: DANCE, MARCH, SPOOKY, ROCK, DISCO, HIP HOP, TRAIN, SAMBA, REGGAE, LULLABY, HOEDOWN. |
| ARP | Sparkle: a held key plays up and down by itself. |
| REC | The next sky (rainbow, stars, hearts, bubbles, flowers, confetti), with confetti. |
| HOME | A surprise friend, with confetti. Held for 1 second it opens the grown-ups' VOLUME instead (below). |
| SAVE | WRITE: she writes her own song (below). SAVE again goes back to playing. |
| HOME held 1 s, then OCT- / OCT+ | The grown-ups' VOLUME: the most the MASTER knob can give (below). |
| MASTER | Volume, up to the VOLUME the grown-ups chose (about half, unless changed). |

## ALGORITHM: how the keys play

| Setting | What the keys do |
| --- | --- |
| 1 FRIEND | Each key plays its note. |
| 3 FRIENDS | Each key plays a chord that is always in key (C major), and three friends sing. The band shows the chord: its letters in their colors and its name. |
| STRUM | Like an Omnichord (an idea from u/veecheech, after dxsloop's arp mode). A black key picks a chord, the one on the white key just below it: F#: F, G#: G, A#: A minor, C#: C, D#: D minor (C to begin with), and plays its root low. The white keys, left to right, play that chord's notes upward, so a hand drawn across them strums it. The band shows the chord. (Not with Scissors, whose keys are drums.) |
| ACCORDION | A black key plays a whole chord while it is held (the same chords as STRUM: F, G, A minor, C, D minor, from F3 up), and the white keys play their own notes, so one hand can play a tune over the chords. The band shows the chord and the staff writes it. |
| GUESS | An ear game (an idea from u/nutty_cartoon). A friend sings a note and the band shows ?. The right key: YES! and confetti. Another key: TRY AGAIN, with no sound of its own, and the friend sings the note again; after two tries the right key blinks. It starts with C and G and adds a note after three right in a row (C G E D A F B, then high C). PLAY: hear it again. |
| FOLLOW | Songs to learn (u/nutty_cartoon). The next note's key lights and the band shows its letter; she plays the song at her own pace, and a wrong key changes nothing. At the end, a party, and it starts again. SEQ picks the song: Twinkle Twinkle, Mary Had a Little Lamb, Hot Cross Buns, Row Your Boat, Old MacDonald, Frere Jacques, London Bridge, Ode to Joy (all old and free). |

Each setting has a picture, for children who do not read yet: a smiling face, three singing friends, a harp, an accordion, a
speech bubble with a note and a question mark, and piano keys with one lit. It shows beside the setting's name when ALGORITHM turns,
and stays at the left of the band while any setting but 1 FRIEND is on.

In every setting, keys held together show their letters in their colors and, when it has one, the chord's name (MAJOR, MINOR,
SEVEN, DIM, SUS), whatever their order (u/nutty_cartoon). GUESS and FOLLOW keep the keys at their own notes (OCT 0).

## The staff

When she plays, a treble clef and five lines drop in at the top of the screen and write her notes as colored heads (the color of the
letter in the band), left to right, up to 8 of them; the oldest drops off. Keys played together, or a key with ALGORITHM on (three friends), stack as a chord. A
black key gets a sharp sign, notes below the staff get their ledger lines, and **8VA / 8VB** (or 15MA / 15MB) shows over or under the
staff when OCT or KNOB 1 has moved her an octave or two up or down. Each note also shows how long it was held, against the beat: a quick tap is a sixteenth (a stem and two flags), then an eighth, a
quarter (about one beat), a half (an open head, about two beats) and a whole (an open head with no stem). The note grows through them
while she holds the key, so she can see a long sound become a long note (suggested by u/theskyisfalling1 on r/MVaveFM1). It goes after 6 seconds of quiet and starts again from the left. The big
letter stays in the band. The clef is the treble clef of the FreeSerif font (GNU FreeFont, GPL-3.0 or later), scaled down to a bitmap in
`tools/gen_kid_art.py`.

## WRITE: her own song

An idea from u/lastapoc on r/MVaveFM1 ("Mario Paint music mode"): SAVE opens a big staff where she writes a song with her
friends, the way Mario Paint's composer did, and plays it back.

| In WRITE | What happens |
| --- | --- |
| Keys | Write a note in the yellow column (a drum friend's keys write its drums, as they play them): the friend she is writing with, in a ball of the note's color. It plays as it lands, and the column moves on when she lets go. Two keys together make a chord (two notes in a column). The same key again takes its note out. |
| PRESETS | The friend she writes with (shown in the band). A song has a band of up to three friends; a fourth takes the place of the one used least (its notes become the new friend's). |
| OCT- / OCT+ | The yellow column back and forward (32 columns, 4 pages of 8: the dots). |
| HOME | Wipes the column (held 1 s it is the grown-ups' VOLUME, as always). |
| PLAY | Plays her song from the start, in a loop: the column turns green and the friend of each note hops along over it. PLAY again stops. |
| SEQ | A beat under it: none, then each of the eleven (drums only). |
| SELECT | Slower or faster. |
| REC held 2 s | A new, empty song. |
| SAVE | Back to playing. |

The song is kept when the FM-1 is switched off: in 58 bytes of the settings that no engine uses (`favorites.factory[14]` and
`[15][0..25]`), so it never touches your projects or the autosave. It is written about 4 seconds after her last change (while
stopped) and when she leaves WRITE. A column is a beat (a quarter note); each friend plays on its own track (tracks 1 to 3, the
drums on 4), through Felucca's own sequencer.

## Volume, for grown-ups

Two guards, both in `kid.c` (`kid_master`):

- **Your maximum.** Hold **HOME for 1 second**: the band says VOLUME and shows eight dots. While HOME is still down, **OCT-** steps it
  down and **OCT+** up. Step 6 is the old "about half" and is where it starts; step 8 lets the MASTER knob go
  all the way, step 1 is very quiet. It is kept when the FM-1 is switched off. Let go of HOME and nothing else happens (no surprise
  friend). A shorter HOME press, and OCT- / OCT+ without HOME, do what they always did.
- **A limiter that follows it.** The FM-1 already had a peak limiter; in Rainbow mode its ceiling now moves with the volume (15 times the
  MASTER level it is at, never above the old fixed ceiling, and it catches every peak at once), a little above what a plain chord over a beat peaks at. So an effect that makes the sound louder
  (GIANT, FREEZE, echo, a chord on a beat) is held to the level she was already hearing. Outside Rainbow mode the limiter is as it was.

Note colors follow the colored tubes and bells used in many early music classes: C red, D orange,
E yellow, F green, G teal, A purple, B pink. Sharps get the color in between.

## The friends

Ducky, Pink Ducky, Cool Ducky, Axolotl, Unicorn, Giraffe, Goo, Goobert, Blue Pup, Red Monster,
Blue Monster, April (the family dog), Scissors, Ghost, Web Hero, Butterfly, Kitty, Frog, Robot, Rainbow,
Princess Ducky, Red Pup, Yellow Bird, Slimy, Bunny, Panda, Penguin, Owl, Bee, Ladybug, Dino, Whale,
Octopus, Fish, Turtle, Ice Cream, Cupcake, Rocket, Strawberry, Star, and six suggested by u/veecheech on r/MVaveFM1:
T-Rex (a growly bass), Skeleton (a theremin: a sine with a wide vibrato that slides between notes), Cowboy and Cowgirl (twangy
plucks, and their own beat, HOEDOWN) Vacuum (the rave "hoover" sound, a joke for the grown-ups) and Space Hero (three buzzy saws that swoop into each note). See CREDITS.md.
Then four more drum kits, every key a different drum like Scissors, each one of Felucca's model kits: Beat Bot (the 80 kit, a
classic drum machine), Toy Drum (the 55 kit), Bongo (the 66 kit, congas) and Monkey (the 10 kit, a cymbal on the bell).

Each one uses one of Felucca's own factory sounds (listed in `firmware/src/kid.c`, `KID_SOUND`), with its
level set so they all play at about the same loudness. Each also has a home sky and a favorite beat
(Ghost and Slimy: SPOOKY, a ghost-hunting funk; Frog and Axolotl: REGGAE; Butterfly and Rainbow: LULLABY). Scissors
plays the drum kit: every key is a different drum.

The pictures are drawn on a roomy canvas, then trimmed, centred and stood on the bottom of a 48 x 48 grid;
a picture that would not fit stops the build, so none is ever cut off. On the screen they are drawn at 1x,
1.5x, 2x, 2.5x or 3x (KNOB 1) and kept inside the screen even at their biggest, hopping and wiggling.

The characters are original drawings in the spirit of the ones she loves, not copies, because this
repository and its website are public.

## Where it lives

- `tools/gen_kid_art.py`: the 50 pictures, drawn from shapes on a 48 x 48 grid and outlined
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

## A tip link on the website

The landing page has a "Say thanks" card with a tip button when the repository has a variable (or a secret) called `DONATE_URL`
(**Settings → Secrets and variables → Actions → Variables tab → New repository variable**), set to an `https://` link such as a
Ko-fi or Buy Me a Coffee page. Without it, or with anything that is not an `https://` link, the card is left out. Change it any time
and re-run the workflow (or push anything) to update the site.

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

## The greeting on a real FM-1

It was choppy on the first real FM-1 (a full-screen redraw every frame, with outline lookups for every pixel). Now the letters skip rows
they are nowhere near, and the whole greeting is computed in 2 x 2 blocks: in the browser build it costs about 7 times less (roughly 45
ms of CPU per second of device time on top of an idle screen, against about 345 before). It needs a check on the device. If it is
still not smooth: draw the still parts (sky, rainbow, grass) once and redraw only the letters, the friend and the confetti, or fewer confetti.

## Not checked yet on a real FM-1

- Drawing speed. The screen has no frame buffer, so moving pictures are drawn in strips. Animated skies
  (stars, hearts, bubbles, confetti) redraw the whole picture area, about 60 ms a frame over the screen's
  connection. If it feels slow on the device, the first thing to try is drawing only the friend's area.
- The volume steps (`KID_VOL_CAP` in `kid.c`, 8 steps of about 3 dB) and the limiter's ceiling (`KID_LIM_X2`): check that the middle steps feel right on the speaker and the headphones.
- The staff drawing on the animated skies (it adds the panel's pixels to each redraw).
