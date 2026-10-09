/* SPDX-License-Identifier: GPL-3.0-only */
/* RAINBOW MODE (FELUCCA_KID): the FM-1 as a toy for a small child. It is on from power-up; holding HOME and SAVE
 * together for 3 s leaves it for the full Felucca until the next power-up.
 *   Keys        play the friend's sound; the band shows the note's letter, big, in its colour (C red, D orange,
 *               E yellow, F green, G teal, A purple, B pink, as the coloured bells and tubes of music classes),
 *               and the friend hops
 *   PRESETS     the next / previous friend (58, each a picture, a sound, a home sky and a favourite beat:
 *               tools/gen_kid_art.py, KID_SOUND)
 *   ALGORITHM   how the keys play, a step a turn, each with a picture: 1 FRIEND, 3 FRIENDS (a key plays a chord in
 *               key), STRUM (an Omnichord: black keys pick a chord, white keys strum it), ACCORDION (a black key plays
 *               its chord, white keys their own notes), GUESS (an ear game: a friend sings a note,
 *               she finds it), FOLLOW (songs to learn: the next key lights, SEQ the next song). Keys held together
 *               show their letters in their colours and the chord's name
 *   SELECT      the beat slower / faster
 *   KNOB 1      big / small: the octave (low notes, a big friend; high notes, a small one); OCT- / OCT+ too
 *   KNOB 2      day / night: the sky, the sun and the moon, and the sound gets darker (FX filter)
 *   KNOB 3      echo: delay and reverb, and copies of the friend that follow it
 *   KNOB 4      wiggle: vibrato, and the friend wobbles
 *   FX SCL ENV LFO EDIT GLO   a tap turns one on (its light lit), another tap off: HICCUP (repeat), BACKWARDS,
 *               SLEEPY (tape stop; it wakes up by itself), SQUEAKY (an octave up), GIANT (an octave down), FREEZE:
 *               Felucca's performance effects (perform.c), and the friend shakes, turns round, droops and dozes,
 *               shrinks, grows, turns to ice
 *   PLAY        the beat: drums (track 4) and a bass line in key (track 2), the friend dances to it; SEQ: the next
 *               of eleven beats
 *   ARP         sparkle: a held key plays up and down
 *   REC         the next sky (rainbow, stars, hearts, bubbles, flowers, confetti, clouds, snow, leaves, desert), with
 *               confetti
 *   HOME        a surprise friend
 *   SAVE        WRITE: she writes her own song on a big staff, each note a friend (kw_*, below); SAVE again, back
 *   THE STAFF   a treble clef drops in at the top when she plays, and writes her notes as coloured noteheads, up to 8,
 *               chords stacked, sharps with a #, ledger lines, 8VA / 8VB over the staff for the octave (OCT); each
 *               note's value by how long it was held against the beat (a sixteenth .. a whole, growing while held);
 *               it goes after 6 s of quiet. The big letter stays in the band
 *   HOME held 1 s, then OCT- / OCT+   the grown-ups' VOLUME: how loud MASTER can go (8 steps, kept over power-off)
 *   HOME held 1 s, then PRESETS   the grown-ups' LOOK-ALIKES: left shows original stand-ins for the friends drawn
 *               after TV and film characters (the same sounds), right the originals (kept over power-off)
 * MASTER is capped at that level (the default is about half) for small ears, and the output limiter's ceiling
 * follows it, so an effect cannot make the toy louder than a plain note (kid_master). Everything else (USB, MIDI,
 * the editor, the web installer) works as in the full Felucca.
 * The screen has no frame buffer: a changed area is drawn row strip by row strip into the two halves of the
 * canvas (gfx.c cv_px), one strip computed while the other goes to the panel. */
#ifndef FELUCCA_KID
#define FELUCCA_KID 1
#endif
#if FELUCCA_KID
#include "kid_art.h"                    /* KID_N friends: KID_NAME, KID_PAL, KID_PIX (tools/gen_kid_art.py); and
                                         * KID_HELLO_NAME, the child's name in the hello (from the build's environment,
                                         * KID_NAME: kept out of the source; none, a plain HI!) */

#define KID_BAND_Y 184u                 /* the band at the bottom (the name, the note, a knob's word) */
#define KID_GROUND 182                  /* the friend stands here */
#define KID_VOL_BYTE (favorites.factory[15][26])   /* the VOLUME level, ^ 6 (0 = 6, the default; settings_persist.c), in
                                                    * bits 0..6; bit 7: the look-alikes' stand-ins shown (kid_art) */
#define KID_HIDE_BIT 0x80u
#define KID_VOL_DEF 6u
#define KID_VOL_HOLD_MS 1000u           /* HOME held this long opens the VOLUME (then OCT- / OCT+) */
#define KID_LIM_X2 30u                  /* the limiter's ceiling is song.master_q12 * 15 (a plain chord over the beat peaks
                                         * at about 0.8 of it, any friend: web/emu/kid_test.mjs); below the full Felucca's
                                         * ceiling it catches every peak at once (fx.c master_out) */
/* MASTER at most this at VOLUME 1 .. 8 (song.master_q12, 4096 = full): steps of about 3 dB; 6 is the old half */
static const uint16_t KID_VOL_CAP[8] = {362, 512, 724, 1024, 1448, 2048, 2896, 4096};
#define KID_EXIT_MS 3000u
#define KID_HELLO_MS 4000u              /* the hello at power-up, at most this long (any key or knob ends it) */
#define KID_SLEEP_MS 3000u              /* SLEEPY: winds down, dozes, then wakes up by itself after this */

enum { KS_RAINBOW, KS_STARS, KS_HEARTS, KS_BUBBLES, KS_FLOWERS, KS_CONFETTI, KS_CLOUDS, KS_SNOW, KS_LEAVES, KS_DESERT,
       KS_COUNT };
enum { KB_DANCE, KB_MARCH, KB_SPOOKY, KB_ROCK, KB_DISCO, KB_HIPHOP, KB_TRAIN, KB_SAMBA, KB_REGGAE, KB_LULLABY,
       KB_HOEDOWN, KB_COUNT };

/* each friend's sound, in the order of KID_NAME: an engine and one of its factory presets (by name), its level
 * (P_LEVEL, 1/2 dB steps: the friends measured alike, about 0.14 peak with MASTER up), its home sky and its
 * favourite beat (PLAY) */
typedef struct {
    uint8_t eng;
    const char *preset;
    uint8_t level, sky, beat;
    int8_t vib;                         /* a vibrato of its own (P_LD_PIT; WIGGLE adds to it), 0 none */
    uint8_t glide;                      /* P_GLIDE: notes slide into each other, 0 none */
    uint8_t dist;                       /* P_DIST: its own drive (a growl), 0 the preset's */
    uint8_t kit;                        /* DRUM: its KIT + 1 (eng_drum.c DK_*: 5 = 80, 6 = 10, 7 = 66, 8 = 55), 0 STD */
    const int16_t *tw;                  /* its own tweaks on the preset: parameter, value, .., -1; 0 none */
} kid_sound_t;

/* the Prophet-style friends' tweaks: the Sequential Prophet-5's classic sounds (its brass, sync lead, strings, bass,
 * bells, clav and leads), made with Felucca's own engines and presets (Melodee's PROPHET engine was the nudge) */
static const int16_t TW_ELEPHANT[] = {P_E1, 9, P_E4, 36, P_E5, 22, P_ED_FLT, 52, P_ATK, 24, P_DEC, 72, P_SUS, 88, -1};
static const int16_t TW_RACECAR[] = {P_E1, 4, P_ATK, 30, P_DEC, 80, P_SUS, 90,   /* the sync sweep: the envelope */
                                     P_M1SRC, MS_ENV, P_M1DST, MD_E1 + 1, P_M1AMT, 22, -1};   /* raises osc 2 */
static const int16_t TW_JELLYFISH[] = {P_ATK, 50, P_E4, 64, -1};
static const int16_t TW_BEAR[] = {P_E0, 0, P_E1, 7, P_E2, 64, P_E4, 40, P_E5, 40, P_E6, 25, P_ED_FLT, 50,
                                  P_DEC, 58, P_SUS, 45, -1};
static const int16_t TW_SNOWMAN[] = {P_E0, 12, P_E1, 17, P_E4, 0, P_E5, 110, P_DEC, 95, P_REL, 85, -1};
static const int16_t TW_CRAB[] = {P_E0, 1, P_E1, 0, P_E4, 50, P_E5, 45, P_ED_FLT, 55, P_DEC, 72, P_REL, 36, -1};
static const int16_t TW_WOLF[] = {P_E4, 34, P_E5, 55, P_ED_FLT, 40, P_ATK, 55, P_SUS, 110, P_REL, 60, -1};
static const int16_t TW_CAMEL[] = {P_E1, 22, P_E4, 62, P_E5, 18, P_E6, 25, P_ATK, 8, -1};
static const kid_sound_t KID_SOUND[KID_N] = {
    {12, "TINE EP", 97, KS_BUBBLES, KB_DANCE},       /* DUCKY: FM6 */
    {7, "SOFT FLUTE", 104, KS_HEARTS, KB_DISCO},      /* PINK DUCKY: WHEEL */
    {0, "SAW LEAD", 97, KS_CONFETTI, KB_HIPHOP},     /* COOL DUCKY: ANALOG */
    {9, "KALIMBA", 122, KS_BUBBLES, KB_REGGAE},       /* AXOLOTL: PHYS */
    {8, "SHIMMER", 108, KS_RAINBOW, KB_DISCO},        /* UNICORN: GRAIN */
    {12, "MARIMBA", 122, KS_FLOWERS, KB_SAMBA},       /* GIRAFFE */
    {5, "WOW BASS", 94, KS_HEARTS, KB_DANCE},        /* GOO: VOICE */
    {6, "FAT BASS", 113, KS_CONFETTI, KB_HIPHOP},     /* GOOBERT: TRIO */
    {3, "PULSE LD", 95, KS_FLOWERS, KB_ROCK},        /* BLUE PUP: LOFI */
    {5, "VOX LEAD", 89, KS_CONFETTI, KB_MARCH},       /* RED MONSTER */
    {7, "FULL ORGAN", 98, KS_RAINBOW, KB_DISCO},     /* BLUE MONSTER */
    {2, "BRASS", 86, KS_FLOWERS, KB_TRAIN},           /* APRIL: PHASE (the family dog) */
    {10, "DRUM KIT", 104, KS_CONFETTI, KB_ROCK},      /* SCISSORS: DRUM, every key another drum */
    {5, "CHOIR AAH", 84, KS_STARS, KB_SPOOKY},        /* GHOST: oooOOooo, and the spooky beat */
    {6, "SYNC LEAD", 96, KS_STARS, KB_ROCK},         /* WEB HERO */
    {12, "BELL", 107, KS_FLOWERS, KB_LULLABY},        /* BUTTERFLY */
    {9, "HARP", 121, KS_HEARTS, KB_SAMBA},            /* KITTY */
    {3, "WAVE BASS", 110, KS_BUBBLES, KB_REGGAE},     /* FROG */
    {3, "ARP 8BIT", 96, KS_STARS, KB_HIPHOP},        /* ROBOT */
    {8, "CLOUD PAD", 100, KS_RAINBOW, KB_LULLABY},    /* RAINBOW */
    {6, "CHIP CHOIR", 90, KS_HEARTS, KB_DISCO},      /* PRINCESS DUCKY: TRIO */
    {3, "STEP LEAD", 99, KS_FLOWERS, KB_ROCK},       /* RED PUP: LOFI */
    {0, "SINE KEY", 107, KS_FLOWERS, KB_MARCH},       /* YELLOW BIRD: ANALOG */
    {8, "GLITCH", 107, KS_STARS, KB_SPOOKY},          /* SLIMY: GRAIN, and the spooky beat */
    {9, "PLUCK", 127, KS_FLOWERS, KB_DANCE},          /* BUNNY: PHYS */
    {12, "PAD", 108, KS_RAINBOW, KB_HIPHOP},          /* PANDA: FM6 */
    {3, "WAVE LEAD", 108, KS_STARS, KB_MARCH},        /* PENGUIN: LOFI */
    {5, "WHISPER", 97, KS_STARS, KB_LULLABY},        /* OWL: VOICE, hoo */
    {6, "ARP LEAD", 96, KS_FLOWERS, KB_SAMBA},       /* BEE: TRIO, bzz */
    {9, "BELL TREE", 101, KS_FLOWERS, KB_DANCE},      /* LADYBUG: PHYS */
    {0, "ACID", 110, KS_CONFETTI, KB_ROCK},           /* DINO: ANALOG */
    {8, "FROZEN", 117, KS_BUBBLES, KB_LULLABY},       /* WHALE: GRAIN */
    {9, "MARIMBA", 127, KS_BUBBLES, KB_REGGAE},       /* OCTOPUS: PHYS */
    {12, "PLUCK", 118, KS_BUBBLES, KB_REGGAE},        /* FISH: FM6 */
    {9, "HAND DRUM", 113, KS_BUBBLES, KB_LULLABY},    /* TURTLE: PHYS */
    {6, "RING BELL", 116, KS_CONFETTI, KB_DISCO},     /* ICE CREAM: TRIO */
    {7, "JAZZ PERC", 101, KS_HEARTS, KB_DANCE},       /* CUPCAKE: WHEEL */
    {11, "ARCADE", 113, KS_STARS, KB_HIPHOP},         /* ROCKET: NOISE */
    {0, "PLUCK", 113, KS_HEARTS, KB_SAMBA},           /* STRAWBERRY: ANALOG */
    {2, "BELL", 108, KS_STARS, KB_LULLABY},           /* STAR: PHASE */
    /* suggested by r/MVaveFM1's u/veecheech (CREDITS.md) */
    {0, "SAW", 92, KS_FLOWERS, KB_ROCK, 0, 0, 80},   /* T-REX: a growly bass: a saw, driven hard (DIST) */
    {0, "SINE KEY", 107, KS_STARS, KB_SPOOKY, 12, 24},  /* SKELETON: a theremin, a wide vibrato and a slide */
    {9, "PLUCK", 125, KS_FLOWERS, KB_HOEDOWN},        /* COWBOY: a twangy string */
    {0, "PLUCK", 113, KS_HEARTS, KB_HOEDOWN},         /* COWGIRL: a twangy pluck */
    {0, "RAVE", 95, KS_CONFETTI, KB_DANCE},          /* VACUUM: the rave "hoover", a joke for the grown-ups */
    {6, "SAW3", 113, KS_STARS, KB_DISCO, 6, 14},      /* SPACE HERO: buzzy saws that swoop (a glide) and shimmer */
    /* more drum kits: every key another drum, as SCISSORS (Felucca's model kits) */
    {10, "DRUM KIT", 103, KS_CONFETTI, KB_HIPHOP, 0, 0, 0, 5},    /* BEAT BOT: the 80 kit, a classic drum machine */
    {10, "DRUM KIT", 105, KS_FLOWERS, KB_MARCH, 0, 0, 0, 8},     /* TOY DRUM: the 55 kit, small and tight */
    {10, "DRUM KIT", 101, KS_HEARTS, KB_SAMBA, 0, 0, 0, 7},       /* BONGO: the 66 kit, congas */
    {10, "DRUM KIT", 106, KS_STARS, KB_ROCK, 0, 0, 0, 6},        /* MONKEY: the 10 kit, a cymbal on the bell */
    /* Prophet-style (TW_*) */
    {0, "BRASS", 100, KS_DESERT, KB_MARCH, 0, 0, 0, 0, TW_ELEPHANT},      /* ELEPHANT: poly brass, a trumpet */
    {6, "SYNC LEAD", 96, KS_CLOUDS, KB_ROCK, 0, 8, 0, 0, TW_RACECAR},    /* RACE CAR: the sync sweep, zoom */
    {0, "STRINGS", 93, KS_BUBBLES, KB_LULLABY, 3, 0, 0, 0, TW_JELLYFISH},   /* JELLYFISH: strings, floating */
    {0, "SQR BASS", 118, KS_LEAVES, KB_HIPHOP, 0, 0, 0, 0, TW_BEAR},     /* BEAR: a punchy saw bass */
    {6, "RING BELL", 101, KS_SNOW, KB_DISCO, 0, 0, 0, 0, TW_SNOWMAN},    /* SNOWMAN: glassy bells (ring, like poly-mod) */
    {0, "PLUCK", 117, KS_BUBBLES, KB_REGGAE, 0, 0, 0, 0, TW_CRAB},       /* CRAB: a snappy clav */
    {0, "SAW LEAD", 97, KS_STARS, KB_SPOOKY, 10, 30, 0, 0, TW_WOLF},     /* WOLF: a howl, swelling and sliding */
    {0, "SAW LEAD", 96, KS_DESERT, KB_TRAIN, 5, 18, 0, 0, TW_CAMEL},     /* CAMEL: a big, loping lead */
};

/* the beats: drums on track 4 (DRUM), a bass line on track 2, 16 steps of 1/16. Drums: a string per lane
 * (eng_drum.c: kick, snare, clap, closed hat, open hat, tom, rim, cowbell), 'x' a hit. Bass: MIDI notes, 0 a rest,
 * 1 holds the note before. Every bass line keeps to C major / A minor: the white keys always fit. The friend
 * dances its way (KD_*) at the beat's tempo (PLAY sets it; SELECT changes it) */
enum { KD_HOP, KD_SWAY, KD_BOTH, KD_SLOW };
typedef struct {
    const char *name;
    uint8_t bpm, dance, dlvl, blvl;     /* tempo, the dance, the kit's and the bass's level */
    const char *lane[NLANE];
    uint8_t bass[16];
} kid_beat_t;
static const kid_beat_t KID_BEATS[KB_COUNT] = {
    {"DANCE", 112, KD_HOP, 92, 100,
     {"x...x...x...x...", "....x.......x...", "............x...", "..x...x...x...x.", "..............x.", 0, 0, 0},
     {36, 0, 48, 0, 36, 0, 48, 0, 36, 0, 48, 0, 43, 0, 48, 0}},
    {"MARCH", 100, KD_HOP, 91, 99,
     {"x.......x.......", "....x.......x.x.", 0, 0, 0, "..............xx", "x...x...x...x...", 0},
     {36, 0, 0, 0, 43, 0, 0, 0, 36, 0, 0, 0, 43, 0, 0, 0}},
    {"SPOOKY", 116, KD_BOTH, 90, 99,                 /* a ghost-hunting funk (in the spirit, not the tune) */
     {"x..x...x..x.....", "....x.......x...", "....x.......x..x", "x.xxx.xxx.xxx.x.", "......x.......x.", 0, 0,
      "x.....x...x....."},
     {33, 0, 0, 45, 0, 33, 0, 0, 36, 0, 38, 0, 40, 0, 38, 36}},
    {"ROCK", 120, KD_HOP, 92, 100,
     {"x.....x.x.......", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", "..............x.", 0, 0, 0},
     {36, 0, 36, 0, 36, 0, 36, 0, 41, 0, 41, 0, 43, 0, 43, 0}},
    {"DISCO", 118, KD_BOTH, 89, 99,
     {"x...x...x...x...", "....x.......x...", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", 0, 0, 0},
     {36, 0, 48, 0, 36, 0, 48, 0, 41, 0, 53, 0, 43, 0, 55, 0}},
    {"HIP HOP", 90, KD_SWAY, 97, 109,
     {"x......x..x.....", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", 0, 0, "...........x....", 0},
     {33, 0, 0, 0, 0, 0, 0, 33, 0, 0, 31, 0, 0, 0, 0, 0}},
    {"TRAIN", 132, KD_HOP, 90, 102,                   /* choo choo */
     {"x.......x.......", "..x...x...x...x.", 0, "xxxxxxxxxxxxxxxx", 0, 0, 0, 0},
     {36, 0, 0, 0, 43, 0, 0, 0, 41, 0, 0, 0, 43, 0, 0, 0}},
    {"SAMBA", 100, KD_BOTH, 88, 98,
     {"x..xx..xx..xx..x", 0, 0, "xxxxxxxxxxxxxxxx", 0, 0, "x.x..x.x.x..x.x.", "..x...x...x...x."},
     {36, 0, 0, 43, 36, 0, 0, 43, 36, 0, 0, 43, 36, 0, 0, 43}},
    {"REGGAE", 80, KD_SWAY, 98, 113,
     {"........x.......", 0, 0, "x.x.x.x.x.x.x.x.", 0, 0, "........x.......", 0},
     {0, 0, 33, 0, 36, 0, 0, 0, 40, 0, 0, 0, 38, 0, 36, 0}},
    {"LULLABY", 70, KD_SLOW, 92, 106,
     {"x.......x.......", 0, 0, 0, 0, 0, "....x.......x...", 0},
     {36, 1, 1, 1, 1, 1, 1, 1, 31, 1, 1, 1, 1, 1, 1, 1}},
    {"HOEDOWN", 126, KD_HOP, 93, 105,                 /* boom-chick, a walking bass (the cowboy and cowgirl's) */
     {"x.......x.......", "....x.......x...", 0, "..x...x...x...x.", 0, 0, "..x...x...x...x.", 0},
     {36, 0, 0, 0, 0, 0, 0, 0, 43, 0, 0, 0, 40, 0, 41, 0}},
};

/* the top row, tapped on and off: Felucca's performance effects (perform.c), and what the friend does meanwhile */
enum { KX_HICCUP, KX_BACK, KX_SLEEPY, KX_SQUEAKY, KX_GIANT, KX_FREEZE, KX_COUNT };
static const uint8_t KID_FX_BTN[KX_COUNT] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO};
static const uint8_t KID_FX_PF[KX_COUNT] = {PF_R16, PF_REV, PF_TAPE, PF_OUP, PF_ODN, PF_FRZ};
static const char *const KID_FX_NAME[KX_COUNT] = {"HICCUP", "BACKWARDS", "SLEEPY", "SQUEAKY", "GIANT", "FREEZE"};

/* the twelve notes from C: their letters and colours */
static const char *const KID_NOTE[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
static const uint16_t KID_NOTE_COL[12] = {
    RGB(236, 40, 52), RGB(246, 92, 40), RGB(255, 140, 26), RGB(255, 184, 30), RGB(255, 218, 36), RGB(130, 214, 56),
    RGB(70, 196, 110), RGB(30, 176, 150), RGB(70, 120, 210), RGB(146, 84, 226), RGB(196, 84, 210), RGB(242, 86, 176),
};
static const uint16_t KID_RAINBOW[7] = {
    RGB(236, 52, 64), RGB(255, 140, 30), RGB(255, 216, 46), RGB(80, 200, 90), RGB(56, 128, 244), RGB(150, 88, 226),
    RGB(255, 120, 190),
};
#define KID_INK RGB(52, 26, 58)          /* outlines (the pictures' too) */
#define KID_WHITE RGB(255, 255, 255)

/* 5 x 7 capitals, digits 1, 3, 5 and 8, '#', '!' and '-': a row a byte, bit 4 the left column */
static const uint8_t KID_FONT[][7] = {
    {0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11}, {0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E},   /* A B */
    {0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E}, {0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E},   /* C D */
    {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F}, {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10},   /* E F */
    {0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F}, {0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11},   /* G H */
    {0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E}, {0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C},   /* I J */
    {0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11}, {0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F},   /* K L */
    {0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11}, {0x11, 0x11, 0x19, 0x15, 0x13, 0x11, 0x11},   /* M N */
    {0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10},   /* O P */
    {0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D}, {0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11},   /* Q R */
    {0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E}, {0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04},   /* S T */
    {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E}, {0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04},   /* U V */
    {0x11, 0x11, 0x11, 0x15, 0x15, 0x15, 0x0A}, {0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11},   /* W X */
    {0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04}, {0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F},   /* Y Z */
    {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E}, {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E},   /* 1 3 */
    {0x0A, 0x0A, 0x1F, 0x0A, 0x1F, 0x0A, 0x0A}, {0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04},   /* # ! */
    {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E}, {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},   /* 5 8 */
    {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00}, {0x0E, 0x11, 0x01, 0x06, 0x04, 0x00, 0x04},   /* - ? */
};
static int32_t kid_glyph_of(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    return c == '1' ? 26 : c == '3' ? 27 : c == '#' ? 28 : c == '!' ? 29 : c == '5' ? 30 : c == '8' ? 31 : c == '-' ? 32 : c == '?' ? 33 : -1;
}

static const int8_t KID_SIN[32] = {0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49, 25,
                                   0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117, -106, -90, -71, -49, -25};

enum { KH_NONE, KH_SIZE, KH_NIGHT, KH_ECHO, KH_WIGGLE, KH_SPEED, KH_CHORD, KH_SPARKLE, KH_BEAT, KH_VOLUME, KH_NEWSONG, KH_SONG,
       KH_LOOK };
enum { KB_NAME, KB_NOTE, KB_HINT, KB_CHORD };
/* ALGORITHM: how the keys play (kid_mode_set). CHOIR: three friends sing a chord; STRUM: an Omnichord (seq.c
 * kb_strum_map); GUESS: an ear game; FOLLOW: songs to learn, the next key lit */
enum { KM_ONE, KM_CHOIR, KM_STRUM, KM_ACCORD, KM_GUESS, KM_FOLLOW, KM_COUNT };
static const char *const KID_MODE_NAME[KM_COUNT] = {"1 FRIEND", "3 FRIENDS", "STRUM", "ACCORDION", "GUESS", "FOLLOW"};
_Static_assert(KID_ICON_N == KM_COUNT, "a picture for each of ALGORITHM's modes (tools/gen_kid_art.py ICONS)");

/* the staff: what she played, as the heads of one entry (a key, or the keys of a chord pressed together) */
#define KID_ST_N 8                      /* entries on the staff at most */
#define KID_ST_MS 6000u                 /* it goes after this long without a note */
#define KID_ST_X0 9                     /* its panel */
#define KID_ST_X1 231
#define KID_ST_Y0 2
#define KID_ST_Y1 74
#define KID_ST_Y(step) (54 - 3 * (step))   /* a step: a line or a space; C4 is 0, the bottom line (E4) 2, the top (F5) 10 */
typedef struct {
    int8_t step[6];
    uint8_t sharp;                      /* bit i: head i has a sharp */
    uint8_t n;
    uint16_t ms;                        /* how long its keys were held (ms): its note value (KID_ST_TYPE) */
    uint32_t keys;                      /* its keys (a bit a key) */
} kid_st_t;
/* a note's value by how long it was held, against the beat (a quarter note): a sixteenth, an eighth, a quarter, a half,
 * a whole (suggested by r/MVaveFM1's u/theskyisfalling1, CREDITS.md) */
enum { KT_16TH, KT_8TH, KT_4TH, KT_HALF, KT_WHOLE };

static struct {
    uint8_t on, ready, fr, scene, beat, chord, arp, night, echo, wiggle;
    uint8_t mode;                       /* KM_* (chord: mode is CHOIR) */
    uint32_t chord_keys, chord_ms;      /* the keys of the last chord she held (2 or more), when */
    uint8_t g_level, g_target, g_wrong, g_streak, g_state, g_again, g_note;   /* GUESS */
    uint32_t g_ms, g_yes_ms, g_try_ms;
    uint8_t f_song, f_pos;              /* FOLLOW: the song, the note she is on */
    uint32_t f_done_ms;
    int8_t oct;                         /* -2 .. 2: song.octave; the friend's size follows */
    int8_t acc;                         /* KNOB 1: detents toward the next octave */
    uint8_t hint, home_down;
    uint8_t fx, fxk;                    /* the top row's effect that is on (a KX_* bit, or 0), the one tapped last */
    uint8_t rearm;                      /* the looping effects let go for a new note, to be pressed again (kid_input) */
    uint8_t st_n;                       /* the staff's entries (st), oldest first */
    kid_st_t st[KID_ST_N];
    uint32_t st_ms, st_t0, st_sig;      /* the last note; the last entry's start; what the panel last showed */
    uint8_t vol_open, vol_hold;         /* the VOLUME is open (HOME held 1 s); HOME is down, since vol_ms */
    uint32_t vol_ms;
    uint32_t sleep_ms;                  /* SLEEPY on since (fm1_ms | 1), 0 none */
    uint8_t zz_was;                     /* SLEEPY's Z was drawn last frame, at zx0, zy0 */
    int16_t zx0, zy0;
    uint8_t bass_ready;
    int8_t key;                         /* the key whose letter shows, -1 none */
    uint8_t rev0, dly0;                 /* the friend's sound's own sends (the echo adds to them) */
    uint8_t voice0;                     /* the friend's sound's own voice mode (a mono friend: LEGATO) */
    uint8_t full;                       /* draw everything next frame */
    uint32_t hint_ms, key_ms, hop_ms, party_ms, exit_ms, play_ms, bg_ms;
    uint32_t hello_ms;                  /* the power-up hello since (fm1_ms | 1), 0 none */
    /* (the times stored as fm1_ms | 1 are compared from now | 1: never "in the future") */
    uint32_t band_sig, pic_sig;         /* what the band and the picture last showed */
    int16_t bx0, by0, bx1, by1;         /* the area the friends covered last frame */
} kid = {1};

/* WRITE (SAVE): her own song, as Mario Paint's composer did it. A column a beat, 32 of them (4 pages of 8), at most two
 * notes a column; each note is one of the band's three friends (a friend a track: trk[0..2], the drums on trk[3]),
 * the one PRESETS picked when she wrote it. PLAY plays the song in a loop (Felucca's own sequencer, a step a beat).
 * Kept in bytes of the settings no engine uses (kw_pack: favorites.factory[14][0..31], [15][0..25]) */
#define KW_COLS 32u
#define KW_PAGE 8u
#define KW_SAVE_MS 4000u                /* saved this long after the last change (while stopped), and on leaving */
static struct {
    uint8_t on, cur, stamp, adv, dirty, beat, loaded, clear_on;
    uint8_t band[3];                    /* the friend of each track 0..2, 0xFF none */
    uint8_t note[KW_COLS][2];           /* (track << 5) | (key + 1), 0 empty */
    uint32_t edit_ms, clear_ms;
    uint32_t sig, col_ms;               /* the page drawn last; when the playing column changed */
    int16_t col, hx, hy;                /* the playing column drawn last (-1 none); where the hopping friend was */
    uint8_t hop_fr;                     /* the friend hopping along (the playing column's) */
} kw = {.col = -1};

/* the picture and name friend fr shows: a look-alike's stand-in (KID_ALT: an original friend with the same sound, sky and
 * beat) while the grown-ups' LOOK-ALIKES is off, else its own */
static uint32_t kid_art(uint32_t fr)
{
    fr %= KID_N;
    return (KID_VOL_BYTE & KID_HIDE_BIT) && KID_ALT[fr] ? KID_ALT[fr] : fr;
}

/* ---------------------------------------------------------------- sound */
static void kid_knobs(void)                       /* the knobs' state into the friend's track (track 1) */
{
    track_t *t = &trk[0];
    perf_k[0] = (int8_t)-(kid.night * 8);         /* the FX filter: night closes it to ~700 Hz */
    if (kw.on)                                    /* (WRITE: track 1 is the band's, as she wrote it) */
        return;
    t->p[P_DLY] = (int16_t)clamp(kid.dly0 + kid.echo * 14, 0, 127);
    t->p[P_REV] = (int16_t)clamp(kid.rev0 + kid.echo * 6, 0, 127);
    t->p[P_LD_PIT] = (int16_t)clamp(KID_SOUND[kid.fr % KID_N].vib + kid.wiggle * 5, -64, 63);
    t->p[P_LRATE] = 74;
    t->p[P_CHRD] = kid.chord && !drum_track(t) ? CH_DIA3 : CH_OFF;   /* (a drum friend: one drum a key) */
    t->p[P_VOICE] = kid.chord || kid.mode == KM_STRUM || kid.mode == KM_ACCORD ? V_POLY : kid.voice0;   /* (a mono
                                                  * friend sings one note at a time: its chords, strums and squeezes
                                                  * need its voices; 1 FRIEND keeps its legato slide) */
    t->p[P_LEVEL] = (int16_t)clamp(KID_SOUND[kid.fr % KID_N].level - (t->p[P_CHRD] ? 5 : 0), 0, 127);   /* (three voices:
                                                  * 2.5 dB down, about as loud as one: the loudness audit, RAINBOW.md) */
    t->p[P_AMODE] = kid.arp ? 3 : 0;              /* UPDN */
    t->p[P_ARATE] = 2;                            /* 1/16 */
    song.octave = kid.mode >= KM_GUESS || drum_track(t) ? 0 : kid.oct;   /* (GUESS, FOLLOW: the keys where the notes
                                                  * are; a drum friend: each key always its drum, big or small) */
}

/* friend fr's sound into track t */
static void kid_sound_to(track_t *t, uint32_t fr)
{
    const kid_sound_t *s = &KID_SOUND[fr % KID_N];
    const engine_t *e = ENGINES[s->eng % NENGINES];
    uint32_t i, p = 0;
    for (i = 0; i < e->npresets; i++)
        if (str_eq(e->presets[preset_orig(e, i)].name, s->preset)) {
            p = i;
            break;
        }
    if (t->eng_req != s->eng)
        set_engine_of(t, s->eng);
    apply_preset_to(t, p);
    t->p[P_LEVEL] = s->level;
    t->p[P_GLIDE] = s->glide;
    if (s->dist)
        t->p[P_DIST] = s->dist;
    if (s->kit)
        t->p[P_E0] = (int16_t)(s->kit - 1u);
    t->p[P_LD_PIT] = s->vib;
    for (i = 0; s->tw && s->tw[i] >= 0; i += 2)
        t->p[s->tw[i]] = s->tw[i + 1];
}

static void kid_sound(void)
{
    track_t *t = &trk[0];
    kid_sound_to(t, kid.fr);
    kid.rev0 = (uint8_t)t->p[P_REV];
    kid.dly0 = (uint8_t)t->p[P_DLY];
    kid.voice0 = (uint8_t)t->p[P_VOICE];
    kid_knobs();
}

/* the beat: its drums into track 4 (DRUM), its bass line into track 2 (ANALOG SQR BASS), its tempo */
static void kid_beat_load(void)
{
    const kid_beat_t *b = &KID_BEATS[kid.beat % KB_COUNT];
    track_t *t = &trk[3], *bt = &trk[1];
    uint8_t note[16], flags[16];
    uint32_t i, l;
    if (t->eng_req != ENGI_DRUM)
        set_engine_of(t, ENGI_DRUM);
    track_defaults_steps(t);
    t->p[P_SLEN] = 16;
    t->p[P_LEVEL] = b->dlvl;                      /* (the kit alone peaks about twice a friend) */
    for (l = 0; l < NLANE; l++)
        for (i = 0; b->lane[l] && i < 16u && b->lane[l][i]; i++)
            if (b->lane[l][i] == 'x')
                grid_hit(t, i, l, 1);
    if (kw.on) {                                  /* WRITE: the drums only (tracks 1 and 2 are the band's), and */
        t->p[P_MUTE] = !kw.beat;                  /* her song's tempo */
        return;
    }
    t->p[P_MUTE] = 0;
    if (!kid.bass_ready) {                        /* the bass sound, once */
        const engine_t *e = ENGINES[0];
        uint32_t p = 0;
        for (i = 0; i < e->npresets; i++)
            if (str_eq(e->presets[preset_orig(e, i)].name, "SQR BASS"))
                p = i;
        set_engine_of(bt, 0);
        apply_preset_to(bt, p);
        kid.bass_ready = 1;
    }
    for (i = 0; i < 16u; i++) {
        note[i] = b->bass[i] > 1u ? b->bass[i] : 0u;
        flags[i] = b->bass[i] == 1u ? 4u : 0u;    /* (1: a TIE, the note before held on) */
    }
    load_pat16(bt, note, flags);
    bt->p[P_LEVEL] = b->blvl;
    song.g[G_BPM] = b->bpm;
}

static void kid_friend(uint32_t f)
{
    kid.fr = (uint8_t)(f % KID_N);
    kid_sound();
    kid.scene = KID_SOUND[kid.fr].sky;
    if (kid.beat != KID_SOUND[kid.fr].beat) {
        kid.beat = KID_SOUND[kid.fr].beat;
        kid_beat_load();
    }
    kid.hop_ms = fm1_ms;
    kid.key = -1;
    kid.hint = KH_NONE;
    kid.full = 1;
}

/* the top row's effects: press / let go what changed (the ISR reads perf_held: change it with the IRQs off) */
static void kid_fx_set(uint32_t want)
{
    uint32_t k;
    kid.rearm = 0;
    for (k = 0; k < KX_COUNT; k++)
        if (((want ^ kid.fx) >> k) & 1u) {
            fm1_irq_off();
            perf_press(KID_FX_PF[k], (int)((want >> k) & 1u));
            fm1_irq_on();
        }
    kid.fx = (uint8_t)want;
}

/* the grown-ups' VOLUME, 1 .. 8 */
static uint32_t kid_vol_level(void)
{
    uint32_t l = ((uint32_t)KID_VOL_BYTE & 0x7Fu) ^ 6u;
    return l >= 1u && l <= 8u ? l : KID_VOL_DEF;
}

/* every main-loop pass (main.c, web_frame), after master_poll: MASTER at most the VOLUME's level, and the limiter's
 * ceiling a little over what a plain note makes at that MASTER: an effect that raises the volume (an octave down, a
 * frozen or repeated chord) is held to it. Out of Rainbow mode it is the full Felucca's again */
static void kid_master(void)
{
    int32_t c;
    if (!kid.on) {
        lim_t = LIM_T;
        return;
    }
    c = KID_VOL_CAP[kid_vol_level() - 1u];
    if (song.master_q12 > (uint32_t)c)
        song.master_q12 = (uint32_t)c;
    c = (int32_t)song.master_q12 * (int32_t)KID_LIM_X2 / 2;
    lim_t = c < 400 ? 400 : c > LIM_T ? LIM_T : c;
}

static void kid_enter(void)
{
    led_pos_init();
    song.sel = 0;
    kid.ready = 1;
    kid.key = -1;
    kid.beat = KID_SOUND[kid.fr].beat;
    kid_beat_load();
    kid_friend(kid.fr);
    kid.hello_ms = fm1_ms | 1u;
}

static int autosave_boot(int allowed);           /* (project.c) */

/* HOME + SAVE held: the full Felucca, with the music of the last session (the autosave, which the toy never
 * writes: main.c skips autosave_poll while Rainbow mode is on) */
static void kw_leave(void);
static void kid_mode_set(uint32_t m);

static void kid_leave(void)
{
    if (kw.on)
        kw_leave();
    kid_mode_set(KM_ONE);                         /* (the keys back to their own notes: STRUM off) */
    kid.on = 0;
    kid_fx_set(0);
    perf_k[0] = 0;
    trk[0].p[P_AMODE] = 0;
    if (song.playing)
        transport_req = 2;
    autosave_boot(1);
    lcd_fill(0, 0, 240, 240, T_BG);
    ui.home = 1;
    ui.force = 1;
}

/* ---------------------------------------------------------------- input */
static void kid_hint(uint32_t h)
{
    kid.hint = (uint8_t)h;
    kid.hint_ms = fm1_ms;
}

static void kid_size(int32_t d)
{
    int32_t o = clamp(kid.oct + d, -2, 2);
    if (o != kid.oct) {
        kid.oct = (int8_t)o;
        song.octave = kid.oct;
        kid.hop_ms = fm1_ms;
    }
    kid_hint(KH_SIZE);
}

/* the grown-ups' VOLUME one step (OCT- / OCT+ with HOME held 1 s): kept over power-off */
static void kid_vol_step(int32_t d)
{
    uint32_t l = (uint32_t)clamp((int32_t)kid_vol_level() + d, 1, 8);
    KID_VOL_BYTE = (uint8_t)((l ^ 6u) | (KID_VOL_BYTE & KID_HIDE_BIT));
    settings_save();
    kid_hint(KH_VOLUME);
}

/* the grown-ups' LOOK-ALIKES (PRESETS with HOME held 1 s, left off, right on; u/ReallyLongLake's idea): the friends
 * drawn after characters from TV and films, or their stand-ins (kid_art), kept over power-off */
static void kid_look_set(int hide)
{
    if (!(KID_VOL_BYTE & KID_HIDE_BIT) != !hide) {
        KID_VOL_BYTE ^= KID_HIDE_BIT;
        settings_save();
    }
    kid.full = 1;
    kid_hint(KH_LOOK);
}

/* the staff gets the keys pressed now (a bit a key: key k plays MIDI 53 + k, F3 .. G5 at OCT 0), with a chord's
 * third and fifth if CHORD is on; keys within 90 ms of the entry's start are one chord */
static void kid_staff_add(uint32_t keys, uint32_t now)
{
    static const uint8_t IDX[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};          /* a pitch class's letter, C = 0 */
    static const uint8_t SHARP[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};
    kid_st_t *e;
    uint32_t k, j;
    if (kid.st_n && now - kid.st_t0 < 90u && now - kid.st_ms < 90u) {
        e = &kid.st[kid.st_n - 1u];
    } else {
        if (kid.st_n == KID_ST_N) {
            for (j = 1; j < KID_ST_N; j++)
                kid.st[j - 1u] = kid.st[j];
            kid.st_n--;
        }
        e = &kid.st[kid.st_n++];
        e->n = 0;
        e->sharp = 0;
        e->ms = 0;
        e->keys = 0;
        kid.st_t0 = now;
    }
    e->keys |= keys;
    for (k = 0; k < 27u; k++) {
        uint32_t n = 53u + k, pc = n % 12u, h, acc = 0;
        int32_t st = ((int32_t)(n / 12u) - 5) * 7 + IDX[pc];
        if (!((keys >> k) & 1u))
            continue;
        if (kid.mode == KM_ACCORD && SHARP[pc]) {                 /* ACCORDION: a black key's chord, F3 .. E4 up */
            n = 53u + (pc - 1u + 7u) % 12u;
            st = ((int32_t)(n / 12u) - 5) * 7 + IDX[n % 12u];
            acc = 1;
        }
        for (h = 0; h < (kid.chord || acc ? 3u : 1u); h++) {
            uint32_t d = e->n, sh = h || acc ? 0u : SHARP[pc], i;
            int32_t s = st + (int32_t)h * 2;
            for (i = 0; i < d; i++)
                if (e->step[i] == s && ((e->sharp >> i) & 1u) == sh)
                    break;
            if (i < d || d >= 6u)
                continue;
            e->step[d] = (int8_t)s;
            e->sharp = (uint8_t)(e->sharp | sh << d);
            e->n++;
        }
    }
    kid.st_ms = now;
}

/* ---------------------------------------------------------------- WRITE: her song */
static int32_t kw_step(uint32_t key, uint32_t *sharp)          /* a key's staff step (C4 = 0), sharp or not */
{
    static const uint8_t IDX[12] = {0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6};
    uint32_t n = 53u + key, pc = n % 12u;
    *sharp = (0x54Au >> pc) & 1u;                                /* C# D# F# G# A# */
    return ((int32_t)(n / 12u) - 5) * 7 + IDX[pc];
}

static uint8_t *kw_byte(uint32_t i)                             /* the 58 bytes it is kept in */
{
    return i < 32u ? &favorites.factory[14][i] : &favorites.factory[15][i - 32u];
}

static void kw_bits_put(uint32_t *at, uint32_t v, uint32_t n)
{
    while (n--) {
        uint8_t *b = kw_byte(*at >> 3);
        *b = (uint8_t)((*b & ~(1u << (*at & 7u))) | (v & 1u) << (*at & 7u));
        v >>= 1;
        (*at)++;
    }
}

static uint32_t kw_bits_get(uint32_t *at, uint32_t n)
{
    uint32_t v = 0, i;
    for (i = 0; i < n; i++, (*at)++)
        v |= ((*kw_byte(*at >> 3) >> (*at & 7u)) & 1u) << i;
    return v;
}

/* the song into its bytes: a version (4 bits), the band (3 x 6: friend + 1), the tempo ((BPM - 60) / 5), the beat (0 none),
 * then each column as one number: ((track A * 3 + track B) * 28 + key A) * 28 + key B (keys + 1, 0 empty), 13 bits */
static void kw_pack(void)
{
    uint32_t at = 0, c, i;
    kw_bits_put(&at, 1, 4);
    for (i = 0; i < 3u; i++)
        kw_bits_put(&at, kw.band[i] == 0xFFu ? 0u : kw.band[i] + 1u, 6);
    kw_bits_put(&at, (uint32_t)clamp((song.g[G_BPM] - 60) / 5, 0, 24), 5);
    kw_bits_put(&at, kw.beat, 4);
    for (c = 0; c < KW_COLS; c++) {
        uint32_t a = kw.note[c][0], b = kw.note[c][1];
        kw_bits_put(&at, (((a >> 5) % 3u * 3u + (b >> 5) % 3u) * 28u + (a & 31u)) * 28u + (b & 31u), 13);
    }
}

_Static_assert(KID_N <= 62, "WRITE keeps a band friend in 6 bits (kw_pack): at most 62 friends");

static void kw_unpack(void)
{
    uint32_t at = 0, c, i, v;
    for (c = 0; c < KW_COLS; c++)
        kw.note[c][0] = kw.note[c][1] = 0;
    kw.band[0] = kw.band[1] = kw.band[2] = 0xFF;
    kw.beat = 0;
    if (kw_bits_get(&at, 4) != 1u)                               /* none yet (or another version): an empty song */
        return;
    for (i = 0; i < 3u; i++) {
        v = kw_bits_get(&at, 6);
        kw.band[i] = v && v <= KID_N ? (uint8_t)(v - 1u) : 0xFFu;
    }
    song.g[G_BPM] = (int16_t)(60 + 5 * (int32_t)kw_bits_get(&at, 5));
    v = kw_bits_get(&at, 4);
    kw.beat = (uint8_t)(v <= KB_COUNT ? v : 0u);
    for (c = 0; c < KW_COLS; c++) {
        uint32_t x = kw_bits_get(&at, 13), kb = x % 28u, ka = x / 28u % 28u, s = x / 784u, sa = s / 3u, sb = s % 3u;
        if (s > 8u || ka > 27u)
            continue;
        kw.note[c][0] = (uint8_t)(ka && kw.band[sa] != 0xFFu ? sa << 5 | ka : 0u);
        kw.note[c][1] = (uint8_t)(kb && kw.band[sb] != 0xFFu ? sb << 5 | kb : 0u);
    }
}

static uint32_t kw_len(void)                                    /* the song's length: to its last note, whole bars */
{
    uint32_t c, last = 0;
    for (c = 0; c < KW_COLS; c++)
        if (kw.note[c][0] | kw.note[c][1])
            last = c + 1u;
    last = (last + 3u) & ~3u;
    return last < 4u ? 4u : last;
}

/* the song into the band's tracks: a step a beat (DIV 1/4), its notes as the keys play them (seq.c kb_map: a drum
 * friend's keys are its drums, not their pitches) */
static void kw_commit(void)
{
    uint32_t s, c, j, len = kw_len();
    for (s = 0; s < 3u; s++) {
        track_t *t = &trk[s];
        for (c = 0; c < NSTEP; c++) {
            step_t *st = &t->step[c];
            step_clear(st);
            for (j = 0; c < KW_COLS && j < 2u; j++) {
                uint32_t n = kw.note[c][j], x;
                if (n && (n >> 5) == s && (x = kb_map(t, (n & 31u) - 1u)) != KB_SILENT) {
                    st->note[st->n++] = (uint8_t)x;
                    st->time = ST_NOTE;
                    st->vel = 100;
                }
            }
            if (drum_track(t))
                step_to_grid(st);                                 /* (a drum's own lane: a hit, as the grid has it) */
        }
        t->p[P_SLEN] = (int16_t)len;
        t->p[P_SDIV] = 0;                                        /* 1/4: a column a beat */
        t->p[P_SGATE] = 100;
    }
}

static void kw_changed(void)
{
    kw.dirty = 1;
    kw.edit_ms = fm1_ms;
    kw_commit();
}

static void kw_save(void)
{
    if (!kw.dirty)
        return;
    kw.dirty = 0;
    kw_pack();
    settings_save();                                             /* (written now, or once the transport stops) */
}

static void kw_track_sound(uint32_t s)                          /* track s gets its friend's sound, for chords */
{
    track_t *t = &trk[s];
    if (kw.band[s] == 0xFFu)
        return;
    kid_sound_to(t, kw.band[s]);
    t->p[P_VOICE] = V_POLY;
    t->p[P_CHRD] = CH_OFF;
    t->p[P_AMODE] = 0;
    t->p[P_MUTE] = 0;
}

/* friend fr is the stamp: its track if it is in the band, else a free track, else the track used least (its notes
 * become fr's) */
static void kw_stamp(uint32_t fr)
{
    uint32_t s, c, j, use[3] = {0, 0, 0}, best = 0;
    for (s = 0; s < 3u; s++)
        if (kw.band[s] == fr) {
            kw.stamp = (uint8_t)s;
            song.sel = s;
            return;
        }
    for (c = 0; c < KW_COLS; c++)
        for (j = 0; j < 2u; j++)
            if (kw.note[c][j])
                use[kw.note[c][j] >> 5]++;
    for (best = 3, s = 0; s < 3u && best == 3u; s++)
        if (kw.band[s] == 0xFFu)
            best = s;
    if (best == 3u)
        for (best = 0, s = 1; s < 3u; s++)
            if (use[s] < use[best])
                best = s;
    kw.band[best] = (uint8_t)fr;
    kw_track_sound(best);
    kw.stamp = (uint8_t)best;
    song.sel = best;
    kw_changed();
}

static void kw_enter(void)
{
    uint32_t s;
    if (song.playing || chain_busy())
        transport_req = 2;
    if (!kw.loaded) {
        int16_t bpm = song.g[G_BPM];
        kw_unpack();
        if (kw.band[0] == 0xFFu && kw.band[1] == 0xFFu && kw.band[2] == 0xFFu)
            song.g[G_BPM] = bpm;                                  /* (a new song: the tempo she had) */
        kw.loaded = 1;
    }
    kw.on = 1;
    kb_strum = kb_accord = 0;
    kw.col = -1;
    kw.sig = 0;
    kid.key = -1;
    kid.st_n = 0;
    song.octave = 0;                                              /* (written as she hears it) */
    for (s = 0; s < 3u; s++)
        kw_track_sound(s);
    kw_stamp(kid.fr);
    kw.dirty = 0;
    if (kw.beat)
        kid.beat = (uint8_t)(kw.beat - 1u);
    kid_beat_load();                                              /* (the drums only, muted without a beat) */
    kw_commit();
    kid.full = 1;
}

static void kw_leave(void)
{
    uint32_t s;
    if (song.playing || chain_busy())
        transport_req = 2;
    kw_save();
    kw.on = 0;
    kb_strum = kid.mode == KM_STRUM;
    kb_accord = kid.mode == KM_ACCORD;
    for (s = 0; s < 3u; s++) {
        track_defaults_steps(&trk[s]);
        trk[s].p[P_SDIV] = 2;
        trk[s].p[P_SLEN] = 16;
        trk[s].p[P_MUTE] = 0;
    }
    song.sel = 0;
    kid.bass_ready = 0;                                           /* the bass sound back on track 2 */
    kid.beat = KID_SOUND[kid.fr].beat;
    kid_beat_load();
    kid_sound();
    kid.full = 1;
}

/* WRITE's keys: a key writes its note into the column (with the stamp friend), the same key again takes it out; the
 * column is left when every key is up */
static void kw_notes(uint32_t notes)
{
    uint32_t k, j;
    if (song.playing)                                             /* (playing: the keys only play along) */
        return;
    for (k = 0; k < 27u; k++) {
        uint8_t *col = kw.note[kw.cur], n = (uint8_t)(kw.stamp << 5 | (k + 1u));
        if (!((notes >> k) & 1u))
            continue;
        for (j = 0; j < 2u && (col[j] & 31u) != k + 1u; j++)
            ;
        if (j < 2u) {                                             /* there already: out */
            col[j] = 0;
            if (!j) {
                col[0] = col[1];
                col[1] = 0;
            }
            kw.adv = 0;
        } else {
            col[col[0] ? 1 : 0] = n;                              /* (a third replaces the second) */
            kw.adv = 1;
        }
        kw_changed();
    }
}

static void kw_clear(void)
{
    uint32_t c;
    for (c = 0; c < KW_COLS; c++)
        kw.note[c][0] = kw.note[c][1] = 0;
    kw.cur = 0;
    kw_changed();
}

/* ---------------------------------------------------------------- ALGORITHM: how the keys play */
/* FOLLOW's songs (old ones, free to use), a note a letter and its octave; each fits the keys at OCT 0 (F3 .. G5) */
typedef struct { const char *name, *notes; } kid_song_t;
static const kid_song_t KID_SONGS[] = {
    {"TWINKLE", "C4 C4 G4 G4 A4 A4 G4 F4 F4 E4 E4 D4 D4 C4 G4 G4 F4 F4 E4 E4 D4 G4 G4 F4 F4 E4 E4 D4 "
                "C4 C4 G4 G4 A4 A4 G4 F4 F4 E4 E4 D4 D4 C4"},
    {"MARY", "E4 D4 C4 D4 E4 E4 E4 D4 D4 D4 E4 G4 G4 E4 D4 C4 D4 E4 E4 E4 E4 D4 D4 E4 D4 C4"},
    {"HOT CROSS BUNS", "E4 D4 C4 E4 D4 C4 C4 C4 C4 C4 D4 D4 D4 D4 E4 D4 C4"},
    {"ROW YOUR BOAT", "C4 C4 C4 D4 E4 E4 D4 E4 F4 G4 C5 C5 C5 G4 G4 G4 E4 E4 E4 C4 C4 C4 G4 F4 E4 D4 C4"},
    {"OLD MACDONALD", "G4 G4 G4 D4 E4 E4 D4 B4 B4 A4 A4 G4 D4 G4 G4 G4 D4 E4 E4 D4 B4 B4 A4 A4 G4"},
    {"FRERE JACQUES", "C4 D4 E4 C4 C4 D4 E4 C4 E4 F4 G4 E4 F4 G4 G4 A4 G4 F4 E4 C4 G4 A4 G4 F4 E4 C4 C4 G3 C4 C4 G3 C4"},
    {"LONDON BRIDGE", "G4 A4 G4 F4 E4 F4 G4 D4 E4 F4 E4 F4 G4 G4 A4 G4 F4 E4 F4 G4 D4 G4 E4 C4"},
    {"ODE TO JOY", "E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 E4 D4 D4 E4 E4 F4 G4 G4 F4 E4 D4 C4 C4 D4 E4 D4 C4 C4"},
};
#define KID_NSONGS (sizeof KID_SONGS / sizeof KID_SONGS[0])

static uint32_t kid_song_key(uint32_t song_i, uint32_t i)       /* note i of a song as a key (0 .. 26), 99 past its end */
{
    static const uint8_t PC[7] = {9, 11, 0, 2, 4, 5, 7};          /* A .. G */
    const char *s = KID_SONGS[song_i % KID_NSONGS].notes;
    while (*s) {
        if (*s >= 'A' && *s <= 'G') {
            if (!i--)
                return (uint32_t)(12 * (s[1] - '0' + 1) + PC[*s - 'A']) - 53u;
            s++;
        }
        s++;
    }
    return 99u;
}

/* GUESS: the notes it asks, the first two far apart; a level more after three right in a row */
static const uint8_t KID_GUESS_KEY[8] = {7, 14, 11, 9, 16, 12, 18, 19};   /* C4 G4 E4 D4 A4 F4 B4 C5 */
enum { KG_IDLE, KG_SING, KG_WAIT };

static void kid_sing(uint32_t key, int on)                       /* the friend sings (or stops) a note itself */
{
    fm1_irq_off();
    if (on)
        trk_note_on(&trk[0], 53u + key, 100);
    else
        trk_note_off(&trk[0], 53u + key);
    fm1_irq_on();
}

static void kid_mode_set(uint32_t m)
{
    if (kid.mode == KM_GUESS && kid.g_state == KG_SING)
        kid_sing(kid.g_note, 0);
    kid.mode = (uint8_t)(m % KM_COUNT);
    kid.chord = kid.mode == KM_CHOIR;
    kb_strum = kid.mode == KM_STRUM && !kw.on;
    kb_accord = kid.mode == KM_ACCORD && !kw.on;
    kid.g_state = KG_IDLE;
    kid.g_ms = fm1_ms + 900u;                                     /* (GUESS: the first note in a moment) */
    kid.g_level = kid.g_streak = kid.g_wrong = kid.g_again = 0;
    kid.f_pos = 0;
    kid.f_done_ms = 0;
    kid_knobs();
}

/* every frame: GUESS asks, listens and answers; FOLLOW moves on. lo: the lowest key pressed just now, -1 none */
static void kid_game(int32_t lo)
{
    uint32_t now = fm1_ms;
    if (kid.mode == KM_GUESS) {
        uint32_t pool = 2u + kid.g_level;
        if (kid.g_state == KG_IDLE && (int32_t)(now - kid.g_ms) >= 0) {
            if (!kid.g_again) {                                   /* a new note, not the one before */
                uint32_t t = KID_GUESS_KEY[rng() % pool];
                while (t == kid.g_target && pool > 1u)
                    t = KID_GUESS_KEY[rng() % pool];
                kid.g_target = (uint8_t)t;
                kid.g_wrong = 0;
            }
            kid.g_again = 0;
            kid.g_note = kid.g_target;
            kid_sing(kid.g_note, 1);
            kid.hop_ms = now;
            kid.g_state = KG_SING;
            kid.g_ms = now;
        } else if (kid.g_state == KG_SING && now - kid.g_ms > 700u) {
            kid_sing(kid.g_note, 0);
            kid.g_state = KG_WAIT;
        }
        if (lo >= 0 && kid.g_state != KG_IDLE) {
            if (kid.g_state == KG_SING)
                kid_sing(kid.g_note, 0);
            if ((uint32_t)lo == kid.g_target) {                   /* right: confetti, and the next one */
                kid.g_yes_ms = now | 1u;
                kid.g_try_ms = 0;
                kid.party_ms = now | 1u;
                if (++kid.g_streak >= 3u && kid.g_level < 6u) {
                    kid.g_level++;
                    kid.g_streak = 0;
                }
                kid.g_ms = now + 1700u;
            } else {                                              /* not yet: no sound of its own, the note again */
                kid.g_try_ms = now | 1u;
                kid.g_yes_ms = 0;
                kid.g_streak = 0;
                kid.g_wrong++;
                kid.g_again = 1;
                kid.g_ms = now + 1100u;
            }
            kid.g_state = KG_IDLE;
        }
    } else if (kid.mode == KM_FOLLOW) {
        if (kid.f_done_ms && now - kid.f_done_ms > 3000u) {      /* the song again from the start */
            kid.f_done_ms = 0;
            kid.f_pos = 0;
            kid.st_n = 0;
        }
        if (lo >= 0 && !kid.f_done_ms && (uint32_t)lo == kid_song_key(kid.f_song, kid.f_pos)) {
            kid.f_pos++;
            if (kid_song_key(kid.f_song, kid.f_pos) == 99u) {     /* the whole song: a party */
                kid.f_done_ms = now | 1u;
                kid.party_ms = now | 1u;
            }
        }
    }
}

static int32_t kid_lit_key(uint32_t now)                         /* the key to light (GUESS's hint, FOLLOW's next), -1 */
{
    if (kid.mode == KM_FOLLOW && !kid.f_done_ms)
        return (int32_t)kid_song_key(kid.f_song, kid.f_pos);
    if (kid.mode == KM_GUESS && kid.g_wrong >= 2u && (now / 250u) & 1u)
        return kid.g_target;
    return -1;
}

static void kid_input(void)
{
    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), held = fm1_in.buttons, id, k;
    uint32_t home = 1u << panel.btn[B_HOME], both = home | (1u << panel.btn[B_SAVE]);
    int32_t s;
    uint32_t any = pressed | notes;
    fm6_poll();
    song.grid = 0;
    perf_kill = 0;
    perf_latch_on = 0;                            /* (held, not latched) */
    if (kid.rearm) {                              /* the looping effects, let go for the last note: on again */
        uint32_t m = kid.rearm & kid.fx;
        kid.rearm = 0;
        for (k = 0; k < KX_COUNT; k++)
            if ((m >> k) & 1u) {
                fm1_irq_off();
                perf_press(KID_FX_PF[k], 1);
                fm1_irq_on();
            }
    }
    for (id = 0; id < 14u; id++) {
        uint32_t b;
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        for (k = 0; k < KX_COUNT; k++)            /* the top row: a tap turns its effect on (another's off), */
            if (b == KID_FX_BTN[k]) {             /* a second tap off: little hands need not hold */
                kid.fxk = (uint8_t)k;
                kid_fx_set(kid.fx == 1u << k ? 0u : 1u << k);
                kid.sleep_ms = k == KX_SLEEPY && kid.fx ? fm1_ms | 1u : 0u;
            }
        switch (b) {
        case B_PLAY:
            if (kid.mode == KM_GUESS && !kw.on) { /* GUESS: the note again */
                if (kid.g_state == KG_SING)
                    kid_sing(kid.g_note, 0);
                kid.g_again = kid.g_state != KG_IDLE || kid.g_again;
                kid.g_state = KG_IDLE;
                kid.g_ms = fm1_ms;
                break;
            }
            if (song.playing || chain_busy()) {
                transport_req = 2;
            } else {
                transport_req = 1;
                kid.play_ms = fm1_ms;
            }
            break;
        case B_ARP:
            if (kw.on)
                break;
            kid.arp ^= 1u;
            kid_knobs();
            kid_hint(KH_SPARKLE);
            break;
        case B_SEQ:
            if (kid.mode == KM_FOLLOW && !kw.on) {   /* FOLLOW: the next song */
                kid.f_song = (uint8_t)((kid.f_song + 1u) % KID_NSONGS);
                kid.f_pos = 0;
                kid.f_done_ms = 0;
                kid.st_n = 0;
                kid_hint(KH_SONG);
                break;
            }
            if (kw.on) {                          /* WRITE: no beat, then each of them */
                kw.beat = (uint8_t)((kw.beat + 1u) % (KB_COUNT + 1u));
                if (kw.beat)
                    kid.beat = (uint8_t)(kw.beat - 1u);
                kid_beat_load();
                kw_changed();
            } else {
                kid.beat = (uint8_t)((kid.beat + 1u) % KB_COUNT);
                kid_beat_load();
            }
            kid_hint(KH_BEAT);
            break;
        case B_REC:                               /* the next sky, with a party; WRITE: held 2 s, a new song */
            if (kw.on) {
                kw.clear_on = 1;
                kw.clear_ms = fm1_ms | 1u;
                break;
            }
            kid.scene = (uint8_t)((kid.scene + 1u) % KS_COUNT);
            kid.full = 1;
            kid.party_ms = fm1_ms | 1u;
            kid.hop_ms = fm1_ms;
            break;
        case B_SAVE:                              /* WRITE: her song, and back (not with HOME: that leaves) */
            if (held & home)
                break;
            if (kw.on)
                kw_leave();
            else
                kw_enter();
            kid.party_ms = fm1_ms | 1u;
            break;
        case B_HOME:
            kid.home_down = 1;
            kid.vol_hold = 1;
            kid.vol_ms = fm1_ms | 1u;
            break;
        case B_OCTDN:
        case B_OCTUP:
            if (kid.vol_open)
                kid_vol_step(b == B_OCTUP ? 1 : -1);
            else if (kw.on) {                     /* WRITE: the column back / on */
                kw.cur = (uint8_t)((kw.cur + (b == B_OCTUP ? 1u : KW_COLS - 1u)) % KW_COLS);
                kw.adv = 0;
            } else
                kid_size(b == B_OCTUP ? 1 : -1);
            break;
        default:
            break;
        }
    }
    if (kid.sleep_ms && (fm1_ms | 1u) - kid.sleep_ms > KID_SLEEP_MS) {   /* SLEEPY wakes up by itself (its tape */
        kid.sleep_ms = 0;                                                  /* stop would leave her keys silent) */
        kid_fx_set(0);
        kid.hop_ms = fm1_ms;
    }
    if ((held & both) == both) {                  /* HOME + SAVE held 3 s: the full Felucca */
        kid.home_down = 0;
        if (!kid.exit_ms)
            kid.exit_ms = fm1_ms | 1u;
        else if ((fm1_ms | 1u) - kid.exit_ms > KID_EXIT_MS) {
            kid_leave();
            return;
        }
    } else {
        kid.exit_ms = 0;
    }
    if (kid.vol_hold && (held & home) && !(held & both & ~home) && !kid.vol_open &&
        (fm1_ms | 1u) - kid.vol_ms >= KID_VOL_HOLD_MS) {   /* HOME held 1 s: the grown-ups' VOLUME opens */
        kid.vol_open = 1;
        kid.home_down = 0;                        /* (no surprise friend when it is let go) */
        kid_hint(KH_VOLUME);
    }
    if (kid.vol_open && (held & home))
        kid.hint_ms = fm1_ms;                     /* (stays up while HOME is held) */
    if (!(held & home)) {
        if (kid.vol_open)
            kid.hint_ms = fm1_ms - 800u;          /* (a moment more, then the name again) */
        kid.vol_open = kid.vol_hold = 0;
    }
    if (kid.home_down && !(held & home)) {        /* HOME let go: a surprise friend (WRITE: the column wiped) */
        kid.home_down = 0;
        if (kw.on) {
            kw.note[kw.cur][0] = kw.note[kw.cur][1] = 0;
            kw_changed();
        } else {
            kid_friend(kid.fr + 1u + rng() % (KID_N - 1u));
            kid.party_ms = fm1_ms | 1u;
        }
    }
    if (kw.on) {
        if (kw.clear_on && !((held >> panel.btn[B_REC]) & 1u))
            kw.clear_on = 0;
        if (kw.clear_on && (fm1_ms | 1u) - kw.clear_ms > 2000u) {  /* REC held 2 s: a new song */
            kw.clear_on = 0;
            kw_clear();
            kid_hint(KH_NEWSONG);
            kid.party_ms = fm1_ms | 1u;
        }
        if (kw.dirty && !song.playing && fm1_ms - kw.edit_ms > KW_SAVE_MS)
            kw_save();
    }
    if (kw.on) {                                  /* WRITE: a key writes (kw_notes), the column is left when all are up */
        if (notes)
            kw_notes(notes);
        if (kw.adv && !fm1_in.notes) {
            kw.adv = 0;
            kw.cur = (uint8_t)((kw.cur + 1u) % KW_COLS);
        }
    }
    if (notes) {                                  /* a key: its letter, and a hop */
        uint32_t loop = kid.fx & (1u << KX_HICCUP | 1u << KX_BACK | 1u << KX_SLEEPY | 1u << KX_FREEZE);
        if (loop) {                               /* HICCUP, BACKWARDS, SLEEPY and FREEZE loop or stop what they */
            for (k = 0; k < KX_COUNT; k++)        /* heard: let go for the new note and press again next frame, or a key */
                if ((loop >> k) & 1u) {           /* tapped in a silence would loop that silence */
                    fm1_irq_off();
                    perf_press(KID_FX_PF[k], 0);
                    fm1_irq_on();
                }
            kid.rearm = (uint8_t)loop;
        }
        if (!kw.on) {                             /* (WRITE: the big staff is hers already) */
            for (k = 26u; notes >> k == 0u; k--)
                ;
            kid.key = (int8_t)k;
            kid.key_ms = fm1_ms;
            kid.hop_ms = fm1_ms;
            if (kid.mode != KM_STRUM && !drum_track(&trk[0]))   /* (STRUM, a drum friend: the keys are not notes) */
                kid_staff_add(notes, fm1_ms);
        }
    } else if (!kw.on && kid.key >= 0 && ((fm1_in.notes >> kid.key) & 1u)) {
        kid.key_ms = fm1_ms;                      /* (held: it stays) */
    } else if (!kw.on && kid.key >= 0 && fm1_in.notes) {   /* let go with others held: the highest of them */
        for (k = 26u; fm1_in.notes >> k == 0u; k--)
            ;
        kid.key = (int8_t)k;
        kid.key_ms = fm1_ms;
    }
    if (!kw.on) {
        uint32_t c = fm1_in.notes, nk = 0;
        int32_t lo = -1;
        for (k = 0; k < 27u; k++) {
            nk += (c >> k) & 1u;
            if (lo < 0 && ((notes >> k) & 1u))
                lo = (int32_t)k;
        }
        if (nk >= 2u) {                           /* keys held together: their chord's letters and name */
            kid.chord_keys = c;
            kid.chord_ms = fm1_ms;
        } else if (notes) {
            kid.chord_ms = 0;
        }
        kid_game(lo);
    }
    if ((s = panel_enc(EN_PRESET)) != 0 && kid.vol_open) {   /* HOME held 1 s: the grown-ups' LOOK-ALIKES */
        any = 1u;
        kid_look_set(s < 0);
    } else if (s != 0) {
        any = 1u;
        if (kw.on) {                              /* WRITE: the friend she writes with */
            kid.fr = (uint8_t)(((int32_t)kid.fr + (s > 0 ? 1 : (int32_t)KID_N - 1)) % (int32_t)KID_N);
            kw_stamp(kid.fr);
        } else {
            kid_friend((uint32_t)((int32_t)kid.fr + (s > 0 ? 1 : (int32_t)KID_N - 1)));
        }
    }
    if ((s = panel_enc(EN_ALGO)) != 0 && !kw.on) {   /* how the keys play: a step a turn */
        any = 1u;
        kid_mode_set((uint32_t)clamp((int32_t)kid.mode + (s > 0 ? 1 : -1), 0, KM_COUNT - 1));
        kid_hint(KH_CHORD);
    }
    if ((s = panel_enc(EN_SELECT)) != 0) {
        any = 1u;
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + s * 5, 60, 180);
        kid_hint(KH_SPEED);
    }
    if ((s = panel_enc(EN_K1)) != 0 && !kw.on) { /* two detents an octave: a small hand turns a lot */
        any = 1u;
        kid.acc = (int8_t)clamp(kid.acc + s, -2, 2);
        if (kid.acc >= 2 || kid.acc <= -2) {
            kid_size(kid.acc > 0 ? 1 : -1);
            kid.acc = 0;
        }
        kid_hint(KH_SIZE);
    }
    if ((s = panel_enc(EN_K2)) != 0) {
        any = 1u;
        uint32_t n = (uint32_t)clamp(kid.night + s, 0, 8);
        if (n != kid.night) {
            kid.night = (uint8_t)n;
            kid.full = 1;
        }
        kid_knobs();
        kid_hint(KH_NIGHT);
    }
    if ((s = panel_enc(EN_K3)) != 0 && !kw.on) {
        any = 1u;
        kid.echo = (uint8_t)clamp(kid.echo + s, 0, 8);
        kid_knobs();
        kid_hint(KH_ECHO);
    }
    if ((s = panel_enc(EN_K4)) != 0 && !kw.on) {
        any = 1u;
        kid.wiggle = (uint8_t)clamp(kid.wiggle + s, 0, 8);
        kid_knobs();
        kid_hint(KH_WIGGLE);
    }
    if (any && kid.hello_ms) {                    /* she is playing: the hello makes way */
        kid.hello_ms = 0;
        kid.full = 1;
    }
}

static void kid_leds(void)
{
    uint8_t nl[FM1_NCOL] = {0}, nd[FM1_NCOL] = {0};
    uint32_t k, c;
    for (k = 0; k < KX_COUNT; k++)
        led_put(nl, panel.btn[KID_FX_BTN[k]], (kid.fx >> k) & 1u);
    led_put(nl, panel.btn[B_ARP], kid.arp && !kw.on);
    led_put(nl, panel.btn[B_SAVE], kw.on);        /* (WRITE: SAVE lit) */
    for (k = 0; k < 27u; k++) {
        led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u) || (int32_t)k == kid_lit_key(fm1_ms));
        led_put(nd, 14u + k, 1);
    }
    for (k = 0; k < NB; k++)
        led_put(nd, panel.btn[k], 1);
    if (song.playing) {
        led_clear(nd, panel.btn[B_PLAY]);
        nl[LED_PLAY_GREEN >> 3] |= (uint8_t)(1u << (LED_PLAY_GREEN & 7u));
    }
    fm1_led_dim_level(0);
    for (c = 0; c < FM1_NCOL; c++) {
        fm1_led_dim[c] = nd[c];
        fm1_led_breath[c] = 0;
        fm1_led[c] = nl[c];
    }
}

/* -------------------------------------------------------------- drawing */
static inline uint16_t kid_mix(uint32_t a, uint32_t b, uint32_t t)     /* a .. b, t 0 .. 256 */
{
    uint32_t r = ((a >> 11) * (256u - t) + (b >> 11) * t) >> 8;
    uint32_t g = (((a >> 5) & 63u) * (256u - t) + ((b >> 5) & 63u) * t) >> 8;
    uint32_t bl = ((a & 31u) * (256u - t) + (b & 31u) * t) >> 8;
    return (uint16_t)(r << 11 | g << 5 | bl);
}

static inline uint32_t kid_hash(uint32_t a, uint32_t b)
{
    uint32_t h = a * 374761393u + b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return h ^ (h >> 16);
}

typedef struct {
    const char *s;
    int16_t x, y, sc;                   /* top left, scale (a font pixel is sc x sc) */
} kid_txt_t;

typedef struct {
    int16_t x, y, w, h, wig;            /* top left, size (px), wobble (px) */
    uint16_t invx, invy;                /* KID_PW * 256 / w, / h */
    uint8_t flip;                       /* mirrored (BACKWARDS) */
    const uint8_t *pix;
    const uint16_t *pal;
} kid_inst_t;

/* one frame's picture: computed by kid_frame_setup, read by kid_px */
static struct {
    uint32_t t, ph;                     /* ms; the wobble's phase */
    uint16_t sky[KID_BAND_Y];
    uint16_t sky_mid;
    kid_inst_t in[6];                   /* front first */
    uint32_t n;
    uint16_t pal[4][16];                /* the echoes' faded palettes */
    uint16_t ice[16];                   /* FREEZE: the friend's palette, icy */
    kid_txt_t zz;                       /* SLEEPY: a Z floating up */
    int8_t zz_on;
    uint8_t party;
    /* the band */
    uint8_t band;
    const char *txt;
    kid_txt_t bt;                       /* the band's text */
    kid_txt_t bt2;                      /* .. its second line (a long name: two lines), sc 0 = none */
    char l1[16], l2[16];
    uint16_t ccol[16];                  /* KB_CHORD: the colour of each of l1's letters */
    int8_t icon;                        /* ALGORITHM's mode as a picture at the band's left (KID_ICON_*), -1 none */
    int16_t ol;                         /* its outline (px) */
    uint16_t ink, bg, bg2;
    int8_t meter;                       /* dots lit (HINT), -1 none */
    uint8_t staff;                      /* the staff shows */
    uint8_t st_ty[KID_ST_N];            /* .. each entry's note value (KT_*) */
    int8_t st_lo[KID_ST_N], st_hi[KID_ST_N];   /* .. its lowest and highest head (steps) */
    kid_txt_t stl;                      /* .. its octave mark (8VA ..), sc 0 = none */
} kf;

static uint32_t kid_beat_ms(void) { return 60000u / (uint32_t)clamp(song.g[G_BPM], 40, 240); }

static int32_t kid_hop(uint32_t t)      /* how high the friend is at ms t */
{
    int32_t h = 0;
    uint32_t d = t - kid.hop_ms, dance = KID_BEATS[kid.beat % KB_COUNT].dance;
    if (d < 320u)
        h = (int32_t)(18u * 4u * d * (320u - d) / (320u * 320u));
    if (song.playing && (dance == KD_HOP || dance == KD_BOTH)) {   /* a hop on every beat */
        uint32_t bm = kid_beat_ms(), half = bm / 2u, p = (t - kid.play_ms) % bm;
        if (p < half && (int32_t)(10u * 4u * p * (half - p) / (half * half)) > h)
            h = (int32_t)(10u * 4u * p * (half - p) / (half * half));
    }
    return h;
}

static int32_t kid_sway(uint32_t t)     /* side to side, px (the swaying dances) */
{
    uint32_t dance = KID_BEATS[kid.beat % KB_COUNT].dance, bm;
    if (!song.playing || dance == KD_HOP)
        return 0;
    bm = kid_beat_ms() * (dance == KD_SLOW ? 4u : 2u);           /* one sway over two beats (four, slow) */
    return KID_SIN[(((t - kid.play_ms) % bm) * 32u / bm) & 31u] * (dance == KD_SLOW ? 6 : 10) / 127;
}

/* the friend's size by octave (-2 .. 2): 3x, 2.5x, 2x, 1.5x and 1x the 48 px picture. At the largest, hopping,
 * it still fits above the ground (kid_inst keeps every friend on the screen) */
static const uint8_t KID_SIZES[5] = {144, 120, 96, 72, 48};
#define KID_MAXS 168                    /* GIANT: at most this */

static void kid_inst(kid_inst_t *in, int32_t cx, int32_t w, int32_t h, int32_t hop, const uint16_t *pal)
{
    in->w = (int16_t)w;
    in->h = (int16_t)h;
    in->invx = (uint16_t)(KID_PW * 256u / (uint32_t)w);
    in->invy = (uint16_t)(KID_PW * 256u / (uint32_t)h);
    in->wig = (int16_t)(kid.wiggle * w / 96);
    in->x = (int16_t)clamp(cx - w / 2, in->wig, 240 - w - in->wig);           /* never off the screen's sides */
    in->y = (int16_t)clamp(KID_GROUND - h - hop, 0, KID_GROUND - h);         /* .. nor its top */
    in->flip = (kid.fx >> KX_BACK) & 1u;
    in->pix = KID_PIX[kid_art(kid.fr)];
    in->pal = pal;
}

static void kid_frame_setup(uint32_t now)
{
    static const uint16_t DAY_T = RGB(110, 196, 255), DAY_B = RGB(206, 238, 255);
    static const uint16_t NIGHT_T = RGB(18, 22, 70), NIGHT_B = RGB(96, 60, 150);
    uint32_t y, k, ne = kid.echo ? 1u + (kid.echo - 1u) / 3u : 0u, n = 0, fx = kid.fx;
    int32_t s = KID_SIZES[clamp(kid.oct + 2, 0, 4)], h, jx = 0, sway = kid_sway(now);
    uint16_t dt = kid.scene == KS_DESERT ? RGB(255, 156, 110) : kid.scene == KS_SNOW ? RGB(168, 204, 240) : DAY_T;
    uint16_t db = kid.scene == KS_DESERT ? RGB(255, 222, 150) : kid.scene == KS_SNOW ? RGB(236, 242, 252) : DAY_B;
    uint16_t top = kid_mix(dt, NIGHT_T, kid.night * 32u), bot = kid_mix(db, NIGHT_B, kid.night * 32u);   /* (the desert:
                                                  * a sunset sky; snow: a pale one) */
    const uint16_t *pal = KID_PAL[kid_art(kid.fr)];
    kid_inst_t tmp[6];
    kf.t = now;
    kf.ph = now / 45u;
    for (y = 0; y < KID_BAND_Y; y++)
        kf.sky[y] = kid_mix(top, bot, y * 256u / KID_BAND_Y);
    kf.sky_mid = kf.sky[KID_BAND_Y / 2u];
    kf.party = kid.party_ms && (now | 1u) - kid.party_ms < 1600u;
    /* the top row's pictures: SQUEAKY smaller, GIANT bigger, SLEEPY squashed (and a Z), FREEZE icy, HICCUP shaking,
     * BACKWARDS mirrored (kid_inst) */
    if ((fx >> KX_SQUEAKY) & 1u)
        s = s * 2 / 3;
    if ((fx >> KX_GIANT) & 1u)
        s = s * 4 / 3 > KID_MAXS ? KID_MAXS : s * 4 / 3;
    h = (fx >> KX_SLEEPY) & 1u ? s * 3 / 4 : s;
    if ((fx >> KX_HICCUP) & 1u)
        jx = (now / 45u) & 1u ? 3 : -3;
    if ((fx >> KX_FREEZE) & 1u) {
        for (k = 0; k < 16u; k++) {                               /* ice: each colour's brightness, in blues */
            uint32_t c = pal[k], l = (((c >> 11) << 3) * 77u + (((c >> 5) & 63u) << 2) * 150u + ((c & 31u) << 3) * 29u) >> 8;
            kf.ice[k] = RGB(l * 3u / 4u + 20u, l * 7u / 8u + 24u, l / 3u + 170u > 255u ? 255u : l / 3u + 170u);
        }
        pal = kf.ice;
    }
    /* back to front into tmp: the echoes (farthest first), the chord's two friends, the friend */
    for (k = ne; k >= 1u; k--) {
        uint32_t i;
        for (i = 0; i < 16u; i++)
            kf.pal[k - 1u][i] = kid_mix(pal[i], kf.sky_mid, 70u + 50u * k);
        kid_inst(&tmp[n++], 120 + sway + jx - (int32_t)k * (6 + kid.echo * 3), s, h, kid_hop(now - 110u * k),
                 kf.pal[k - 1u]);
    }
    if (kid.chord) {                                              /* (half size: whole pixels at 2x and 3x) */
        kid_inst(&tmp[n++], 120 - 80 + sway, s / 2, h / 2, kid_hop(now - 70u), pal);
        kid_inst(&tmp[n++], 120 + 80 + sway, s / 2, h / 2, kid_hop(now - 140u), pal);
    }
    kid_inst(&tmp[n++], 120 + sway + jx, s, h, kid_hop(now), pal);
    for (k = 0; k < n; k++)
        kf.in[k] = tmp[n - 1u - k];
    kf.n = n;
    kf.zz_on = (fx >> KX_SLEEPY) & 1u;
    if (kf.zz_on) {                                               /* a Z rising by the friend's head, again and again */
        kf.zz.s = "Z";
        kf.zz.sc = 4;
        kf.zz.x = (int16_t)clamp(kf.in[0].x + kf.in[0].w - 8, 4, 210);
        kf.zz.y = (int16_t)clamp(kf.in[0].y - 6 - (int32_t)((now / 30u) % 40u), 2, 150);
    }
}

/* the band's content for this frame; returns its signature (a change redraws it) */
/* a drum friend's key: the drum it strikes, as a word, and the drum's colour (Felucca's kit or a model kit's piece) */
static const char *kid_drum_word(uint32_t key, uint16_t *col)
{
    static const char *const STD[DVT_COUNT] = {"KICK", "KICK", "SNARE", "CLAP", "HI-HAT", "OPEN HAT", "TOM", "CONGA",
                                                "RIM", "CLAVE", "COWBELL", "CYMBAL"};
    static const uint8_t LANE_OF[DVT_COUNT] = {0, 0, 1, 2, 3, 4, 5, 5, 6, 6, 7, 7};
    track_t *t = &trk[0];
    int32_t st;
    uint32_t n = kb_map(t, key), ty = drum_gm(t->p, n == KB_SILENT ? 36u : n, &st), lane;
    const char *w;
    if (ty >= 16u) {                                              /* a model kit: its piece in the lane */
        lane = ty & 7u;
        w = drum_lane_name(t, lane);
        w = str_eq(w, "HATCL") ? "HI-HAT" : str_eq(w, "HATOP") ? "OPEN HAT" : str_eq(w, "CYM") ? "CYMBAL" :
            str_eq(w, "BELL") ? "COWBELL" : w;
    } else {
        lane = LANE_OF[ty % DVT_COUNT];
        w = STD[ty % DVT_COUNT];
    }
    *col = KID_RAINBOW[lane % 7u];
    return w;
}

static uint32_t kid_band_word(const char *s, int16_t sc, uint16_t bg, uint32_t sig)   /* a word alone in the band */
{
    uint32_t w = str_len(s) * 6u * (uint32_t)sc - (uint32_t)sc;
    kf.band = KB_NAME;
    kf.txt = kf.bt.s = s;
    kf.bt.sc = sc;
    kf.bt.x = (int16_t)((240 - (int32_t)w) / 2);
    kf.bt.y = (int16_t)(KID_BAND_Y + (56 - 7 * sc) / 2 + 2);
    kf.bt2.sc = 0;
    kf.ol = sc >= 6 ? 3 : 2;
    kf.ink = KID_WHITE;
    kf.bg = bg;
    kf.bg2 = kid_mix(bg, RGB(0, 0, 0), 60u);
    return sig;
}

/* a chord in the band: its letters in their colours (low to high), and its name under them when it has one */
static uint32_t kid_chord_band(const uint8_t *pcs, uint32_t n)
{
    static const struct { uint16_t m; const char *q; } T[] = {
        {0x091, "MAJOR"}, {0x089, "MINOR"}, {0x491, "SEVEN"}, {0x049, "DIM"}, {0x0A1, "SUS"}};
    uint32_t mask = 0, i, j, root = pcs[0], sig = 0x600u, w;
    const char *q = 0;
    char *p = kf.l1;
    for (i = 0; i < n; i++)
        mask |= 1u << pcs[i];
    for (i = 0; i < n && !q; i++) {                               /* (any of its notes can be the root: inversions) */
        uint32_t rel = ((mask >> pcs[i]) | (mask << (12u - pcs[i]))) & 0xFFFu;
        for (j = 0; j < sizeof T / sizeof T[0]; j++)
            if (rel == T[j].m) {
                q = T[j].q;
                root = pcs[i];
                break;
            }
    }
    for (i = 0; i < n && p < kf.l1 + 12; i++) {
        const char *l = KID_NOTE[pcs[i]];
        if (i)
            *p++ = ' ';
        for (; *l; l++, p++) {
            *p = *l;
            kf.ccol[p - kf.l1] = KID_NOTE_COL[pcs[i]];
        }
        sig = sig * 31u + pcs[i];
    }
    *p = 0;
    kf.band = KB_CHORD;
    kf.txt = kf.bt.s = kf.l1;
    kf.bt.sc = (int16_t)(str_len(kf.l1) * 24u - 4u <= 232u ? 4 : 3);
    w = str_len(kf.l1) * 6u * (uint32_t)kf.bt.sc - (uint32_t)kf.bt.sc;
    kf.bt.x = (int16_t)((240 - (int32_t)w) / 2);
    kf.bt.y = (int16_t)(q ? KID_BAND_Y + 4 : KID_BAND_Y + 15);
    kf.bt2.sc = 0;
    if (q) {
        str_cpy(kf.l2, KID_NOTE[root], sizeof kf.l2);
        p = kf.l2 + str_len(kf.l2);
        *p++ = ' ';
        str_cpy(p, q, sizeof kf.l2 - (uint32_t)(p - kf.l2));
        kf.bt2.s = kf.l2;
        kf.bt2.sc = 3;
        kf.bt2.x = (int16_t)((240 - (int32_t)(str_len(kf.l2) * 18u - 3u)) / 2);
        kf.bt2.y = KID_BAND_Y + 34;
        sig = sig * 31u + root * 7u + (uint32_t)(q[1]);
    }
    kf.ol = 2;
    kf.ink = KID_WHITE;
    kf.bg = kid_mix(KID_NOTE_COL[root], KID_WHITE, 170u);
    kf.bg2 = kid_mix(KID_NOTE_COL[root], KID_WHITE, 90u);
    return sig;
}

/* ALGORITHM's modes in the band: 0 = nothing of theirs now (the usual band) */
static uint32_t kid_play_band(uint32_t now)
{
    int keyed = kid.key >= 0 && now - kid.key_ms < 900u;
    uint8_t pcs[6];
    uint32_t n = 0, k;
    if (kid.hint != KH_NONE && now - kid.hint_ms < 1500u && !(keyed && kid.key_ms - kid.hint_ms < 0x80000000u))
        return 0;                                                 /* (a knob's word, unless a key came after it) */
    if (kid.mode == KM_GUESS) {
        if (kid.g_yes_ms && (now | 1u) - kid.g_yes_ms < 1300u)
            return kid_band_word("YES!", 6, RGB(56, 170, 80), 0x701u);
        if (kid.g_try_ms && (now | 1u) - kid.g_try_ms < 1500u)
            return kid_band_word("TRY AGAIN", 4, RGB(236, 120, 40), 0x702u);
        if (!keyed)
            return kid_band_word("?", 7, RGB(150, 110, 230), 0x703u);
        return 0;
    }
    if (kid.mode == KM_STRUM && keyed && !ENGINES[eng_idx(trk[0].eng_req)]->keys) {   /* the chord she strums */
        uint32_t r = kb_strum_root;
        pcs[0] = (uint8_t)r;
        pcs[1] = (uint8_t)((r + (kb_strum_minor ? 3u : 4u)) % 12u);
        pcs[2] = (uint8_t)((r + 7u) % 12u);
        return kid_chord_band(pcs, 3);
    }
    if (kid.mode == KM_ACCORD && keyed && ((0x54Au >> ((53u + (uint32_t)kid.key) % 12u)) & 1u) &&
        !ENGINES[eng_idx(trk[0].eng_req)]->keys) {                /* ACCORDION: a black key's chord */
        uint32_t r = (53u + (uint32_t)kid.key) % 12u - 1u;
        pcs[0] = (uint8_t)r;
        pcs[1] = (uint8_t)((r + (r == 9u || r == 2u ? 3u : 4u)) % 12u);
        pcs[2] = (uint8_t)((r + 7u) % 12u);
        return kid_chord_band(pcs, 3);
    }
    if (drum_track(&trk[0]))                                      /* (a drum friend: drums, not chords) */
        return 0;
    if (kid.mode == KM_CHOIR && keyed && !((0x54Au >> ((53u + (uint32_t)kid.key) % 12u)) & 1u)) {
        static const uint8_t SCALE[7] = {0, 2, 4, 5, 7, 9, 11};  /* three friends: the key's chord in C major */
        uint32_t pc = (53u + (uint32_t)kid.key) % 12u, d;
        for (d = 0; SCALE[d] != pc; d++)
            ;
        for (k = 0; k < 3u; k++)
            pcs[k] = SCALE[(d + 2u * k) % 7u];
        return kid_chord_band(pcs, 3);
    }
    if (keyed && kid.chord_ms && now - kid.chord_ms < 900u) {     /* keys pressed together: their chord */
        uint32_t seen = 0;
        for (k = 0; k < 27u && n < 5u; k++)
            if ((kid.chord_keys >> k) & 1u) {
                uint32_t pc = (53u + k) % 12u;
                if (!((seen >> pc) & 1u)) {
                    seen |= 1u << pc;
                    pcs[n++] = (uint8_t)pc;
                }
            }
        if (n >= 2u)
            return kid_chord_band(pcs, n);
    }
    if (kid.mode == KM_FOLLOW && !keyed) {                        /* the next note: its letter, waiting */
        uint32_t nk = kid_song_key(kid.f_song, kid.f_pos), pc;
        if (kid.f_done_ms)
            return kid_band_word("YAY!", 6, RGB(56, 170, 80), 0x704u);
        pc = (53u + nk) % 12u;
        kid_band_word(KID_NOTE[pc], 7, kid_mix(KID_NOTE_COL[pc], KID_WHITE, 170u), 0);
        kf.ink = KID_NOTE_COL[pc];
        kf.bg2 = kid_mix(KID_NOTE_COL[pc], KID_WHITE, 90u);
        kf.ol = 3;
        kf.bt.y = KID_BAND_Y + 5;
        return 0x800u | nk << 4 | kid.f_pos << 12;
    }
    return 0;
}

static uint32_t kid_band_setup(uint32_t now)
{
    uint32_t sig, len, w;
    int vol = (kid.hint == KH_VOLUME || kid.hint == KH_LOOK) && now - kid.hint_ms < 1500u;     /* (the grown-ups' VOLUME comes before all) */
    int hint = kid.hint != KH_NONE && now - kid.hint_ms < 1500u;
    kf.meter = -1;
    kf.ol = 2;
    if (kw.on && !(hint && (kid.hint == KH_SPEED || kid.hint == KH_NIGHT || kid.hint == KH_VOLUME || kid.hint == KH_LOOK))) {
        /* WRITE: the friend she writes with, and a word */
        kf.band = KB_NAME;
        kf.txt = hint && kid.hint == KH_BEAT ? (kw.beat ? KID_BEATS[(kw.beat - 1u) % KB_COUNT].name : "NO BEAT") :
                 hint && kid.hint == KH_NEWSONG ? "NEW SONG" : song.playing ? "LISTEN" : "WRITE";
        kf.bt.sc = str_len(kf.txt) * 24u - 4u <= 176u ? 4 : 3;
        kf.ink = KID_WHITE;
        kf.bg = RGB(150, 110, 230);
        kf.bg2 = RGB(120, 84, 200);
        sig = 0x500u ^ (uint32_t)(uintptr_t)kf.txt * 7u ^ (uint32_t)kw.band[kw.stamp % 3u] << 24;
    } else if (!vol && !kw.on && (sig = kid_play_band(now)) != 0) {   /* (ALGORITHM's modes, a chord: all set) */
        return sig;
    } else if (!vol && kid.key >= 0 && now - kid.key_ms < 900u && drum_track(&trk[0]) && !kid.fx) {   /* a drum friend: */
        uint16_t c;                                                   /* the drum's name */
        const char *w = kid_drum_word((uint32_t)kid.key, &c);
        return kid_band_word(w, str_len(w) * 24u - 4u <= 232u ? 4 : 3, kid_mix(c, RGB(0, 0, 0), 30u),
                             0x900u ^ (uint32_t)(uintptr_t)w * 7u);
    } else if (!vol && kid.key >= 0 && now - kid.key_ms < 900u) {      /* a note: its letter, big (the effect's word, if one is on) */
        uint32_t pc = (53u + (uint32_t)kid.key) % 12u;    /* key 0 is F (seq.c kb_map: 53 + k) */
        kf.band = KB_NOTE;
        kf.txt = KID_NOTE[pc];
        kf.bt.sc = 7;                                     /* (the band's own: as big as fits) */
        kf.ol = 3;
        kf.ink = KID_NOTE_COL[pc];
        kf.bg = kid_mix(KID_NOTE_COL[pc], KID_WHITE, 170u);
        kf.bg2 = kid_mix(KID_NOTE_COL[pc], KID_WHITE, 90u);
        sig = 0x100u | pc;
        if (kid.fx) {
            kf.txt = KID_FX_NAME[kid.fxk % KX_COUNT];
            kf.bt.sc = 4;
            kf.ol = 2;
            kf.ink = KID_WHITE;
            kf.bg = KID_RAINBOW[kid.fxk % KX_COUNT];
            kf.bg2 = kid_mix(kf.bg, RGB(0, 0, 0), 60u);
            sig = 0x400u | kid.fxk;
        }
    } else if (kid.fx && !vol) {                          /* an effect held: its word */
        kf.band = KB_NAME;
        kf.txt = KID_FX_NAME[kid.fxk % KX_COUNT];
        kf.bt.sc = 4;
        kf.ink = KID_WHITE;
        kf.bg = KID_RAINBOW[kid.fxk % KX_COUNT];
        kf.bg2 = kid_mix(kf.bg, RGB(0, 0, 0), 60u);
        sig = 0x400u | kid.fxk;
    } else if (kid.hint != KH_NONE && now - kid.hint_ms < 1500u) {
        static const char *const WORD[] = {"", "", "DAY", "ECHO", "WIGGLE", "", "", "SPARKLE", "", "VOLUME", "", "", ""};
        int32_t v = 0;
        kf.band = KB_HINT;
        kf.txt = WORD[kid.hint];
        switch (kid.hint) {
        case KH_SIZE: kf.txt = kid.oct < 0 ? "BIG" : kid.oct > 0 ? "SMALL" : "MEDIUM"; v = 4 - kid.oct * 2; break;
        case KH_NIGHT: kf.txt = kid.night >= 5u ? "NIGHT" : kid.night >= 3u ? "SUNSET" : "DAY"; v = kid.night; break;
        case KH_ECHO: v = kid.echo; break;
        case KH_WIGGLE: v = kid.wiggle; break;
        case KH_SPEED:
            v = (song.g[G_BPM] - 60) / 15;
            kf.txt = song.g[G_BPM] < 100 ? "SLOW" : song.g[G_BPM] > 135 ? "FAST" : "WALK";
            break;
        case KH_CHORD: kf.txt = KID_MODE_NAME[kid.mode % KM_COUNT]; v = -1; break;   /* (its picture: kid_band_icon) */
        case KH_SPARKLE: v = kid.arp ? 8 : 0; break;
        case KH_BEAT: kf.txt = KID_BEATS[kid.beat % KB_COUNT].name; v = -1; break;
        case KH_VOLUME: v = (int32_t)kid_vol_level(); break;
        case KH_SONG: kf.txt = KID_SONGS[kid.f_song % KID_NSONGS].name; v = -1; break;
        case KH_LOOK: kf.txt = KID_VOL_BYTE & KID_HIDE_BIT ? "LOOK-ALIKES OFF" : "LOOK-ALIKES ON"; v = -1; break;
        default: break;
        }
        kf.meter = (int8_t)clamp(v, -1, 8);
        kf.bt.sc = kid.hint == KH_LOOK ? 2 : 3;
        kf.ink = KID_WHITE;
        kf.bg = RGB(150, 110, 230);
        kf.bg2 = RGB(120, 84, 200);
        sig = 0x200u | (uint32_t)kid.hint << 4 | (uint32_t)(v + 1) << 12 | (uint32_t)kid.beat << 20 | (uint32_t)kid.f_song << 26;
    } else {
        kf.band = KB_NAME;
        kf.txt = KID_NAME[kid_art(kid.fr)];
        kf.bt.sc = str_len(kf.txt) * 24u - 4u <= 228u ? 4 : 3;
        kf.ink = KID_WHITE;
        kf.bg = kid.night >= 5u ? RGB(40, 110, 70) : RGB(96, 200, 90);
        kf.bg2 = kid.night >= 5u ? RGB(24, 80, 50) : RGB(56, 156, 66);
        sig = 0x300u | (uint32_t)kid.fr << 4 | (kid.night >= 5u) << 12;
    }
    kf.bt2.sc = 0;
    len = str_len(kf.txt);
    w = len * 6u * (uint32_t)kf.bt.sc - (uint32_t)kf.bt.sc;
    kf.bt.s = kf.txt;
    kf.bt.x = (int16_t)((240 - (int32_t)w) / 2);
    if (kw.on && kf.band == KB_NAME)                      /* (WRITE: beside the friend) */
        kf.bt.x = (int16_t)(58 + (178 - (int32_t)w) / 2);
    kf.bt.y = (int16_t)(kf.band == KB_NOTE ? KID_BAND_Y + 5 : KID_BAND_Y + ((kf.band == KB_HINT ? 40 : 56) - 7 * kf.bt.sc) / 2 + 2);
    if (kf.bt.x < 4 && len < sizeof kf.l1) {              /* too wide even at 3: two lines, split at the middle space */
        uint32_t i, cut = 0;
        for (i = 0; i < len; i++)
            if (kf.txt[i] == ' ' && (!cut || (i > cut ? i - len / 2 : len / 2 - i) < (cut > len / 2 ? cut - len / 2 : len / 2 - cut)))
                cut = i;
        if (cut) {
            for (i = 0; i < cut; i++)
                kf.l1[i] = kf.txt[i];
            kf.l1[cut] = 0;
            str_cpy(kf.l2, kf.txt + cut + 1, sizeof kf.l2);
            kf.bt.s = kf.l1;
            kf.bt.sc = kf.bt2.sc = 3;
            kf.bt2.s = kf.l2;
            kf.bt.x = (int16_t)((240 - (int32_t)(str_len(kf.l1) * 18u - 3u)) / 2);
            kf.bt2.x = (int16_t)((240 - (int32_t)(str_len(kf.l2) * 18u - 3u)) / 2);
            kf.bt.y = KID_BAND_Y + 7;
            kf.bt2.y = KID_BAND_Y + 33;
        }
    }
    return sig;
}

static inline int kid_ink_at(const kid_txt_t *t, int32_t x, int32_t y)   /* is (x, y) on the text */
{
    int32_t gx, gy, ci, g;
    x -= t->x;
    y -= t->y;
    if (x < 0 || y < 0)
        return 0;
    gx = x / t->sc;
    gy = y / t->sc;
    ci = gx / 6;
    if (gy >= 7 || gx % 6 == 5 || ci >= (int32_t)str_len(t->s) || (g = kid_glyph_of(t->s[ci])) < 0)
        return 0;
    return (KID_FONT[g][gy] >> (4 - gx % 6)) & 1;
}

/* 1 on the text, 2 on its outline (o px around it), 0 neither */
static int kid_text_at(const kid_txt_t *t, int32_t x, int32_t y, int32_t o)
{
    if (kid_ink_at(t, x, y))
        return 1;
    if (kid_ink_at(t, x - o, y) || kid_ink_at(t, x + o, y) || kid_ink_at(t, x, y - o) || kid_ink_at(t, x, y + o) ||
        kid_ink_at(t, x - o, y - o) || kid_ink_at(t, x + o, y - o) || kid_ink_at(t, x - o, y + o) ||
        kid_ink_at(t, x + o, y + o))
        return 2;
    return 0;
}

static int32_t kw_sprite(uint32_t fr, int32_t x, int32_t y, int32_t sz);

/* ALGORITHM's mode as a picture for children who do not read yet: beside its name when the knob turns, and at the band's
 * left while a mode other than 1 FRIEND is on, the words moved over beside it (none when they would not fit) */
static uint32_t kid_band_icon(uint32_t sig)
{
    int32_t m = -1, w1, w2 = 0;
    kf.icon = -1;
    if (kw.on || kid.hello_ms)
        return sig;
    if (kf.band == KB_HINT && kid.hint == KH_CHORD) {             /* (no dots under it: the word in the middle) */
        m = kid.mode;
        kf.bt.y = (int16_t)(KID_BAND_Y + (56 - 7 * kf.bt.sc) / 2 + 2);
    }
    else if (kf.band != KB_HINT && kid.mode != KM_ONE)
        m = kid.mode;
    if (m < 0)
        return sig;
    w1 = (int32_t)(str_len(kf.bt.s) * 6u) * kf.bt.sc - kf.bt.sc;
    if (kf.bt2.sc)
        w2 = (int32_t)(str_len(kf.bt2.s) * 6u) * kf.bt2.sc - kf.bt2.sc;
    if (w1 > 178 || w2 > 178)
        return sig;
    kf.bt.x = (int16_t)(58 + (178 - w1) / 2);
    if (kf.bt2.sc)
        kf.bt2.x = (int16_t)(58 + (178 - w2) / 2);
    kf.icon = (int8_t)m;
    return sig * 31u + (uint32_t)m + 1u;
}

static uint16_t kid_band_px(int32_t x, int32_t y)
{
    int k;
    if (kf.icon >= 0 && x >= 6 && x < 54 && y >= (int32_t)KID_BAND_Y + 5 && y < (int32_t)KID_BAND_Y + 53) {
        uint32_t ix = (uint32_t)(x - 6), iy = (uint32_t)(y - ((int32_t)KID_BAND_Y + 5)), b;   /* the mode's picture */
        b = KID_ICON_PIX[kf.icon % KID_ICON_N][iy * 24u + (ix >> 1)];
        b = ix & 1u ? b & 15u : b >> 4;
        if (b)
            return KID_ICON_PAL[kf.icon % KID_ICON_N][b];
    }
    if (kw.on && kf.band == KB_NAME && x < 56 && y >= (int32_t)KID_BAND_Y + 5) {    /* WRITE: the stamp friend */
        int32_t p = kw_sprite(kw.band[kw.stamp % 3u], x - 4, y - ((int32_t)KID_BAND_Y + 6), 48);
        if (p >= 0)
            return (uint16_t)p;
    }
    k = kid_text_at(&kf.bt, x, y, kf.ol);
    if (!k && kf.bt2.sc)
        k = kid_text_at(&kf.bt2, x, y, kf.ol);
    if (k == 1 && kf.band == KB_CHORD && kid_text_at(&kf.bt, x, y, kf.ol) == 1)   /* (a chord: each letter its colour) */
        return kf.ccol[((x - kf.bt.x) / (6 * kf.bt.sc)) % 16];
    if (k)
        return k == 1 ? kf.ink : KID_INK;
    if (kf.meter >= 0 && y >= (int32_t)KID_BAND_Y + 38) {         /* the meter: 8 dots */
        int32_t i = (x - 28) / 24, dx = (x - 28) % 24 - 12, dy = y - ((int32_t)KID_BAND_Y + 46), d = dx * dx + dy * dy;
        if (x >= 28 && i < 8 && d <= 49)
            return d >= 30 ? KID_INK : i < kf.meter ? KID_RAINBOW[i % 7] : KID_WHITE;
    }
    if (y < (int32_t)KID_BAND_Y + 3)
        return KID_INK;
    if (y < (int32_t)KID_BAND_Y + 7)
        return kf.bg2;
    return kf.bg;
}

static int32_t kid_scene_px(uint32_t scene, int32_t x, int32_t y)   /* the scene's decoration, -1 none */
{
    uint32_t t = kf.t, h;
    switch (scene) {
    case KS_RAINBOW: {
        int32_t dx = x - 120, dy = y - 236, d = dx * dx + dy * dy, i;
        if (d >= 208 * 208 || d < 136 * 136)
            break;
        for (i = 0; i < 6; i++)
            if (d >= (208 - 12 * (i + 1)) * (208 - 12 * (i + 1)))
                return kid_mix(KID_RAINBOW[i], kf.sky[y], 70u + kid.night * 14u);
        break;
    }
    case KS_STARS: {
        int32_t cx = x / 24, cy = y / 24, sx, sy, dx, dy, r;
        h = kid_hash((uint32_t)cx, (uint32_t)cy);
        if ((h & 3u) == 0u)
            break;
        sx = cx * 24 + 4 + (int32_t)((h >> 4) % 16u);
        sy = cy * 24 + 4 + (int32_t)((h >> 8) % 16u);
        dx = x - sx < 0 ? sx - x : x - sx;
        dy = y - sy < 0 ? sy - y : y - sy;
        r = 1 + (int32_t)((h >> 12) & 1u) + (((h >> 16) + t / 260u) % 4u == 0u);
        if ((dx == 0 && dy <= r) || (dy == 0 && dx <= r))
            return (h >> 14) % 3u == 0u ? RGB(255, 236, 120) : (h >> 14) % 3u == 1u ? RGB(255, 190, 230) : KID_WHITE;
        break;
    }
    case KS_HEARTS: {
        static const uint16_t HEART[8] = {0x0C6, 0x1EF, 0x1FF, 0x1FF, 0x0FE, 0x07C, 0x038, 0x010};
        int32_t yy = y + (int32_t)(t / 45u), cx = x / 30, cy = yy / 30, sc, hx, hy;
        h = kid_hash((uint32_t)cx + 7u, (uint32_t)cy);
        if ((h & 3u) == 0u)
            break;
        sc = 1 + (int32_t)((h >> 9) & 1u);
        hx = (x - cx * 30 - (int32_t)(h % 12u)) / sc;
        hy = (yy - cy * 30 - (int32_t)((h >> 5) % 12u)) / sc;
        if (x - cx * 30 - (int32_t)(h % 12u) >= 0 && yy - cy * 30 - (int32_t)((h >> 5) % 12u) >= 0 && hx < 9 && hy < 8 &&
            ((HEART[hy] >> (8 - hx)) & 1u))
            return (h >> 12) % 3u == 0u ? RGB(236, 64, 150) : (h >> 12) % 3u == 1u ? RGB(255, 120, 170) : RGB(236, 52, 80);
        break;
    }
    case KS_BUBBLES: {
        int32_t yy = y + (int32_t)(t / 35u), cx = x / 32, cy = yy / 32, r, bx, by, d;
        h = kid_hash((uint32_t)cx + 13u, (uint32_t)cy);
        if ((h & 3u) == 0u)
            break;
        r = 3 + (int32_t)(h % 6u);
        bx = cx * 32 + 9 + (int32_t)((h >> 4) % 14u) + KID_SIN[(cy * 5 + (int32_t)(t / 90u)) & 31] * 3 / 127;
        by = cy * 32 + 9 + (int32_t)((h >> 8) % 14u);
        d = (x - bx) * (x - bx) + (yy - by) * (yy - by);
        if (d <= r * r + r && d >= (r - 1) * (r - 1))
            return RGB(255, 255, 255);
        if (d < (r - 1) * (r - 1))
            return (x - bx == -r / 2 && yy - by == -r / 2) ? KID_WHITE : kid_mix(kf.sky[y], RGB(200, 240, 255), 90u);
        break;
    }
    case KS_FLOWERS: {
        int32_t hy = 146 + KID_SIN[(x / 8) & 31] * 8 / 127, cx, fx, fy, dx, dy;
        if (y >= hy)
            return y < hy + 3 ? RGB(56, 156, 66) : kid_mix(RGB(110, 210, 90), RGB(70, 170, 70), (uint32_t)(y - hy) * 6u);
        cx = x / 26;
        h = kid_hash((uint32_t)cx + 31u, 5u);
        fx = cx * 26 + 13;
        fy = 146 + KID_SIN[(fx / 8) & 31] * 8 / 127 - 8 - (int32_t)(h % 10u);
        dx = x - fx;
        dy = y - fy;
        if (dx * dx + dy * dy <= 4)
            return RGB(255, 216, 46);
        if ((dx - 3) * (dx - 3) + dy * dy <= 5 || (dx + 3) * (dx + 3) + dy * dy <= 5 || dx * dx + (dy - 3) * (dy - 3) <= 5 ||
            dx * dx + (dy + 3) * (dy + 3) <= 5)
            return KID_RAINBOW[(h >> 8) % 7u == 2u ? 6u : (h >> 8) % 7u];
        if (dx == 0 && dy > 0 && dy < 14)
            return RGB(36, 130, 64);
        break;
    }
    case KS_CLOUDS: {                                             /* puffy clouds, drifting by */
        int32_t xx = x + (int32_t)(t / 70u), cx = xx / 96, cy = y / 56, bx, by, k;
        if (y >= 150)
            break;
        h = kid_hash((uint32_t)cx + 41u, (uint32_t)cy);
        if ((h & 3u) == 0u)
            break;
        bx = cx * 96 + 30 + (int32_t)(h % 30u);
        by = cy * 56 + 24 + (int32_t)((h >> 6) % 14u);
        for (k = -1; k <= 1; k++) {                               /* three puffs and a flat bottom */
            int32_t r = k ? 11 : 15, dx = xx - (bx + k * 15), dy = y - (by - (k ? 0 : 5));
            if (dx * dx + dy * dy <= r * r && y <= by + 8)
                return y >= by + 5 ? kid_mix(KID_WHITE, kf.sky[y], 70u) : KID_WHITE;
        }
        break;
    }
    case KS_SNOW: {                                               /* snowy hills, little pines, and falling snow */
        int32_t hy = 150 + KID_SIN[((x + 20) / 9) & 31] * 7 / 127, yy, cx, cy, fx, fy, dx, dy, tx;
        cx = x / 44;                                              /* a pine on some of the hills */
        h = kid_hash((uint32_t)cx + 53u, 9u);
        tx = cx * 44 + 10 + (int32_t)(h % 24u);
        if ((h >> 5) % 3u != 0u) {
            int32_t ty = 150 + KID_SIN[((tx + 20) / 9) & 31] * 7 / 127, top = ty - 30, w;
            if (y >= top && y < ty - 4) {
                w = (y - top) * 9 / 26 + ((y - top) % 9) / 2;
                if (x - tx <= w && tx - x <= w)
                    return y < top + 6 || (y - top) % 9 == 0 ? KID_WHITE : RGB(36, 130, 64);
            }
            if (y >= ty - 4 && y < ty + 2 && x - tx <= 1 && tx - x <= 1)
                return RGB(120, 76, 40);
        }
        if (y >= hy)
            return y < hy + 2 ? RGB(186, 210, 240) : kid_mix(KID_WHITE, RGB(200, 222, 248), (uint32_t)(y - hy) * 5u);
        yy = y - (int32_t)(t / 38u) + (1 << 26);                  /* the flakes fall: the field moves down */
        cx = x / 22;
        cy = yy / 22;
        h = kid_hash((uint32_t)cx + 61u, (uint32_t)cy);
        if ((h & 1u) == 0u)
            break;
        fx = cx * 22 + 4 + (int32_t)(h % 14u) + KID_SIN[(cy * 7 + (int32_t)(t / 110u)) & 31] * 3 / 127;
        fy = cy * 22 + 4 + (int32_t)((h >> 5) % 14u);
        dx = x - fx < 0 ? fx - x : x - fx;
        dy = yy - fy < 0 ? fy - yy : yy - fy;
        if ((h >> 10) & 1u ? (dx + dy <= 1) : ((dx == 0 && dy <= 2) || (dy == 0 && dx <= 2) || (dx == dy && dx == 1)))
            return KID_WHITE;
        break;
    }
    case KS_LEAVES: {                                             /* autumn hills, and leaves drifting down */
        static const uint8_t LEAF[7] = {0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08};
        static const uint16_t LC[4] = {RGB(236, 92, 36), RGB(255, 176, 30), RGB(200, 52, 40), RGB(170, 110, 40)};
        int32_t hy = 152 + KID_SIN[((x + 60) / 8) & 31] * 6 / 127, yy, cx, cy, lx, ly;
        if (y >= hy)
            return y < hy + 3 ? RGB(150, 120, 40) : kid_mix(RGB(206, 170, 70), RGB(150, 140, 60), (uint32_t)(y - hy) * 6u);
        yy = y - (int32_t)(t / 30u) + (1 << 26);                  /* (twice size: a leaf is 14 x 18) */
        lx = x + KID_SIN[((yy / 40) * 5 + (int32_t)(t / 140u)) & 31] * 6 / 127 + 40;   /* (swaying as it falls) */
        cx = lx / 40;
        cy = yy / 40;
        h = kid_hash((uint32_t)cx + 71u, (uint32_t)cy);
        if ((h & 3u) == 0u)
            break;
        lx = (lx - cx * 40 - 4 - (int32_t)(h % 20u)) >> 1;
        ly = (yy - cy * 40 - 4 - (int32_t)((h >> 5) % 18u)) >> 1;
        if (lx >= 0 && lx < 7 && ly >= 0 && ly < 9) {
            if (ly >= 7)                                           /* the stem */
                return lx == 3 ? RGB(120, 76, 40) : -1;
            if ((LEAF[ly] >> (6 - lx)) & 1u)
                return lx == 3 && ly >= 2 ? kid_mix(LC[(h >> 9) & 3u], RGB(0, 0, 0), 50u) : LC[(h >> 9) & 3u];
        }
        break;
    }
    case KS_DESERT: {                                             /* mesas far off, dunes, and cacti */
        int32_t dy0 = 156 + KID_SIN[((x + 40) / 10) & 31] * 6 / 127, cx, tx, ty, dx, ry;
        cx = x / 70;                                              /* a cactus on some of the dunes */
        h = kid_hash((uint32_t)cx + 83u, 3u);
        tx = cx * 70 + 14 + (int32_t)(h % 40u);
        ty = 156 + KID_SIN[((tx + 40) / 10) & 31] * 6 / 127;
        if ((h >> 6) % 3u != 0u) {
            dx = x - tx;
            ry = ty - y;
            if ((dx >= -3 && dx <= 3 && ry >= 0 && ry < 34) ||                        /* the trunk, two arms */
                (dx >= -11 && dx <= -8 && ry >= 14 && ry < 26) || (dx >= -11 && dx <= -3 && ry >= 14 && ry < 17) ||
                (dx >= 8 && dx <= 11 && ry >= 18 && ry < 28) || (dx >= 3 && dx <= 11 && ry >= 18 && ry < 21))
                return dx == -1 || dx == 2 || dx == -10 || dx == 10 ? RGB(46, 140, 70) : RGB(70, 180, 90);
            if (dx * dx + (ry - 34) * (ry - 34) <= 9 && ry >= 34)
                return RGB(70, 180, 90);
        }
        if (y >= dy0)
            return y < dy0 + 2 ? RGB(214, 150, 70) : kid_mix(RGB(250, 200, 110), RGB(232, 170, 90), (uint32_t)(y - dy0) * 6u);
        cx = x / 120;                                             /* the mesas: flat tops, steep sides */
        h = kid_hash((uint32_t)cx + 97u, 1u);
        {
            int32_t mx = cx * 120 + 20 + (int32_t)(h % 40u), mw = 22 + (int32_t)((h >> 6) % 20u), mt = 112 + (int32_t)((h >> 11) % 18u);
            int32_t side = (y - mt) / 2;
            if (y >= mt && x >= mx - side && x <= mx + mw + side)
                return kid_mix(RGB(206, 110, 80), kf.sky[y], 60u + kid.night * 12u);
        }
        break;
    }
    default:
        break;
    }
    return -1;
}

static int32_t kid_confetti(int32_t x, int32_t y, uint32_t speed, uint32_t salt)
{
    int32_t yy = y - (int32_t)(kf.t / speed), cx = x / 14, cy = (yy + 1400) / 14, px, py;
    uint32_t h = kid_hash((uint32_t)cx + salt, (uint32_t)cy);
    if ((h & 1u) == 0u)
        return -1;
    px = x - cx * 14 - (int32_t)(h % 10u);
    py = yy + 1400 - cy * 14 - (int32_t)((h >> 4) % 10u);
    if ((h >> 7) & 1u ? (px >= 0 && px < 4 && py >= 0 && py < 2) : (px >= 0 && px < 2 && py >= 0 && py < 4))
        return KID_RAINBOW[(h >> 8) % 7u];
    return -1;
}

static int32_t kid_friends_px(int32_t x, int32_t y)        /* the friends, front first; -1 none */
{
    uint32_t i;
    for (i = 0; i < kf.n; i++) {
        const kid_inst_t *in = &kf.in[i];
        int32_t sy = y - in->y, sx, ry, rx, xo;
        uint32_t b;
        if ((uint32_t)sy >= (uint32_t)in->h)
            continue;
        ry = sy * in->invy >> 8;
        xo = in->wig ? KID_SIN[((uint32_t)ry / 3u + kf.ph) & 31u] * in->wig / 127 : 0;
        sx = x - in->x - xo;
        if ((uint32_t)sx >= (uint32_t)in->w)
            continue;
        rx = sx * in->invx >> 8;
        if (in->flip)
            rx = KID_PW - 1 - rx;
        b = in->pix[ry * (KID_PW / 2) + (rx >> 1)];
        b = rx & 1 ? b & 15u : b >> 4;
        if (b)
            return in->pal[b];
    }
    return -1;
}

/* the hello: HI and the name, a letter at a time, each dropping in, then a wave running through them */
static int kid_char_ink(char ch, int32_t cx, int32_t cy, int32_t sc, int32_t x, int32_t y)
{
    int32_t g = kid_glyph_of(ch), gx, gy;
    if (g < 0 || x < cx || y < cy || x >= cx + 5 * sc || y >= cy + 7 * sc)
        return 0;
    gx = (x - cx) / sc;
    gy = (y - cy) / sc;
    return (KID_FONT[g][gy] >> (4 - gx)) & 1;
}

static int32_t kid_hello_line(const char *s, int32_t sc, int32_t top, uint32_t first, int32_t x, int32_t y)
{
    static const int8_t OX[8] = {-1, 1, 0, 0, -1, 1, -1, 1}, OY[8] = {0, 0, -1, 1, -1, -1, 1, 1};
    int32_t n = (int32_t)str_len(s), x0 = (240 - (n * 6 * sc - sc)) / 2, k, o, pass, ol = sc >= 7 ? 3 : 2;
    int32_t ci = x < x0 ? -1 : (x - x0) / (6 * sc);
    uint32_t t = (kf.t | 1u) - kid.hello_ms;
    for (pass = 0; pass < 2; pass++)                  /* the letters first, then their outlines (under neighbours) */
        for (k = ci - 1; k <= ci + 1; k++) {
            uint32_t i = first + (uint32_t)k, at = i * 160u, d;
            int32_t cx = x0 + k * 6 * sc, cy, w;
            if (k < 0 || k >= n || t < at)
                continue;
            d = t - at;
            w = KID_SIN[((t / 40u) + i * 3u) & 31u];
            cy = top - (d < 300u ? (int32_t)(300u - d) / 2 : 0) - (w > 0 ? w * 7 / 127 : 0);   /* drop in, then wave */
            if (y < cy - ol || y >= cy + 7 * sc + ol)                 /* (most pixels are nowhere near the letter) */
                continue;
            if (!pass && kid_char_ink(s[k], cx, cy, sc, x, y))
                return KID_RAINBOW[i % 7u];
            for (o = 0; pass && o < 8; o++)
                if (kid_char_ink(s[k], cx, cy, sc, x + OX[o] * ol, y + OY[o] * ol))
                    return KID_INK;
        }
    return -1;
}

static uint16_t kid_hello_px(int32_t x, int32_t y)
{
    int32_t c;
    if (KID_HELLO_NAME[0] ? (c = kid_hello_line("HI", 7, 14, 0u, x, y)) >= 0 ||
                            (c = kid_hello_line(KID_HELLO_NAME, KID_HELLO_SC, 76, 2u, x, y)) >= 0
                          : (c = kid_hello_line("HI!", 8, 34, 0u, x, y)) >= 0)
        return (uint16_t)c;
    if ((c = kid_confetti(x, y, 14u, 5u)) >= 0)
        return (uint16_t)c;
    if ((c = kid_friends_px(x, y)) >= 0)
        return (uint16_t)c;
    if (y >= KID_GROUND)
        return y < KID_GROUND + 3 ? RGB(56, 156, 66) : RGB(96, 200, 90);
    if ((c = kid_scene_px(KS_RAINBOW, x, y)) >= 0)
        return (uint16_t)c;
    return kf.sky[y];
}

static uint16_t kid_pic_px(int32_t x, int32_t y)
{
    int32_t c;
    if (kf.party && (c = kid_confetti(x, y, 9u, 77u)) >= 0)
        return (uint16_t)c;
    if (kf.zz_on && (c = kid_text_at(&kf.zz, x, y, 2)) != 0)
        return c == 1 ? KID_WHITE : KID_INK;
    if ((c = kid_friends_px(x, y)) >= 0)
        return (uint16_t)c;
    if (kid.night >= 3u || kid.scene == KS_RAINBOW || kid.scene == KS_FLOWERS) {   /* the sun or the moon */
        int32_t dx = x - 206, dy = y - 32, d = dx * dx + dy * dy;
        if (kid.night < 5u) {
            int32_t ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
            if (d <= 15 * 15)
                return d >= 13 * 13 ? RGB(255, 150, 30) : kid.night >= 3u ? RGB(255, 150, 60) : RGB(255, 220, 50);
            if (d >= 19 * 19 && d <= 25 * 25 && (ax <= 1 || ay <= 1 || (ax - ay <= 1 && ay - ax <= 1)))
                return RGB(255, 200, 40);
        } else if (d <= 15 * 15 && (dx - 7) * (dx - 7) + (dy + 5) * (dy + 5) > 12 * 12) {
            return RGB(255, 246, 200);
        }
    } else if (kid.scene != KS_STARS) {
        int32_t dx = x - 206, dy = y - 32, d = dx * dx + dy * dy, ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
        if (d <= 15 * 15)
            return d >= 13 * 13 ? RGB(255, 150, 30) : RGB(255, 220, 50);
        if (d >= 19 * 19 && d <= 25 * 25 && (ax <= 1 || ay <= 1 || (ax - ay <= 1 && ay - ax <= 1)))
            return RGB(255, 200, 40);
    }
    if ((c = kid_scene_px(kid.scene, x, y)) >= 0)
        return (uint16_t)c;
    if (kid.scene == KS_CONFETTI && (c = kid_confetti(x, y, 30u, 3u)) >= 0)
        return (uint16_t)c;
    return kf.sky[y];
}

/* the staff: whether it shows, and a signature of what it shows (a change repaints its panel) */
static uint32_t kid_staff_setup(uint32_t now)
{
    static const char *const MARK[5] = {"15MB", "8VB", 0, "8VA", "15MA"};
    uint32_t i, j, sig = 7;
    kf.staff = kid.st_n && now - kid.st_ms < KID_ST_MS;
    if (!kf.staff) {
        kid.st_n = 0;
        return 0;
    }
    {                                                   /* the newest note grows while its keys are held */
        kid_st_t *e = &kid.st[kid.st_n - 1u];
        if (fm1_in.notes & e->keys)
            e->ms = (uint16_t)(now - kid.st_t0 > 60000u ? 60000u : now - kid.st_t0);
    }
    for (i = 0; i < kid.st_n; i++) {
        const kid_st_t *e = &kid.st[i];
        uint32_t q = kid_beat_ms(), m = e->ms;
        int32_t lo = 99, hi = -99;
        kf.st_ty[i] = (uint8_t)(m * 8u < q * 3u ? KT_16TH : m * 4u < q * 3u ? KT_8TH : m * 2u < q * 3u ? KT_4TH :
                                m < q * 3u ? KT_HALF : KT_WHOLE);
        for (j = 0; j < e->n; j++) {
            lo = e->step[j] < lo ? e->step[j] : lo;
            hi = e->step[j] > hi ? e->step[j] : hi;
        }
        kf.st_lo[i] = (int8_t)lo;
        kf.st_hi[i] = (int8_t)hi;
        sig = sig * 31u + e->n + (uint32_t)e->sharp * 7u + (uint32_t)kf.st_ty[i] * 4099u;
        for (j = 0; j < e->n; j++)
            sig = sig * 31u + (uint8_t)e->step[j];
    }
    kf.stl.s = MARK[clamp(kid.oct + 2, 0, 4)];
    kf.stl.sc = kf.stl.s ? 1 : 0;
    kf.stl.x = 31;
    kf.stl.y = kid.oct < 0 ? 60 : 8;
    return sig * 5u + (uint32_t)(kid.oct + 3);
}

static int kid_staff_in(int32_t x, int32_t y, int32_t in, int32_t r)    /* in the panel, shrunk by in (corners: radius r) */
{
    int32_t x0 = KID_ST_X0 + in, x1 = KID_ST_X1 - 1 - in, y0 = KID_ST_Y0 + in, y1 = KID_ST_Y1 - 1 - in, cx, cy;
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return 0;
    cx = x < x0 + r ? x0 + r : x > x1 - r ? x1 - r : x;
    cy = y < y0 + r ? y0 + r : y > y1 - r ? y1 - r : y;
    return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r + r;
}

/* a pixel of the panel: the picture through a pale glass, the clef, five lines, the notes in their colours */
static uint16_t kid_staff_px(int32_t x, int32_t y)
{
    static const uint8_t SPC[7] = {0, 2, 4, 5, 7, 9, 11};            /* a letter's pitch class */
    int32_t i = x < 44 ? -1 : (x - 44) / 21, d, h;
    uint16_t pic = kid_pic_px(x, y);
    if (!kid_staff_in(x, y, 0, 6))
        return pic;
    if (!kid_staff_in(x, y, 2, 4))
        return KID_INK;
    if (i >= 0 && i < (int32_t)kid.st_n) {                           /* the notes of this slot */
        const kid_st_t *e = &kid.st[i];
        int32_t cx = 56 + 21 * i, dx = x - cx, lo = kf.st_lo[i], hi = kf.st_hi[i], ls, ty = kf.st_ty[i];
        for (h = 0; h < (int32_t)e->n; h++) {
            int32_t s = e->step[h], dy = y - KID_ST_Y(s), a = dx * dx, pc;
            if (dx >= -4 && dx <= 4 && dy >= -3 && dy <= 3 && 49 * a + 81 * dy * dy <= 992) {
                pc = (SPC[((s % 7) + 7) % 7] + (int32_t)((e->sharp >> h) & 1u)) % 12;
                if (36 * a + 49 * dy * dy > 440)
                    return KID_INK;
                return ty >= KT_HALF && 36 * a + 49 * dy * dy <= 100 ? KID_WHITE : KID_NOTE_COL[pc];   /* half, whole: open */
            }
            if (((e->sharp >> h) & 1u) && kid_char_ink('#', cx - 12, KID_ST_Y(s) - 3, 1, x, y))
                return KID_INK;
        }
        if (ty != KT_WHOLE) {                                         /* the stem, up on the right below the middle */
            int up = lo + hi < 12, sx = up ? cx + 4 : cx - 4;          /* line, down on the left from it; 3.5 spaces */
            int32_t y0 = up ? KID_ST_Y(hi) - 20 : KID_ST_Y(hi), y1 = up ? KID_ST_Y(lo) : KID_ST_Y(lo) + 20, f;
            if (x == sx && y >= y0 && y <= y1)
                return KID_INK;
            for (f = 0; f < (ty == KT_16TH ? 2 : ty == KT_8TH ? 1 : 0); f++) {   /* flags: an eighth one, a sixteenth two */
                int32_t fx = x - sx - 1, fy = up ? y - y0 - 5 * f : y1 - 5 * f - y;
                if (fx >= 0 && fx <= 3 && fy >= 2 * fx && fy <= 2 * fx + 2)
                    return KID_INK;
            }
        }
        ls = (54 - y) % 3 == 0 ? (54 - y) / 3 : 1;                   /* a ledger line: on an even step off the staff */
        if (dx >= -7 && dx <= 7 && !(ls & 1) && ((ls <= 0 && lo <= ls) || (ls >= 12 && hi >= ls)))
            return KID_INK;
    }
    d = y - 15;
    if (x >= 13 && x < 13 + KID_CLEF_W && d >= 0 && d < KID_CLEF_H && ((KID_CLEF[d] >> (15 - (x - 13))) & 1u))
        return KID_INK;
    if (x >= 12 && x < 228 && y >= 24 && y <= 48 && (y - 24) % 6 == 0)
        return KID_INK;
    if (kf.stl.sc && kid_ink_at(&kf.stl, x, y))
        return KID_INK;
    return kid_mix(pic, KID_WHITE, 214u);
}

static inline uint16_t kid_view_px(int32_t x, int32_t y)             /* the picture, and the staff over it */
{
    if (kf.staff && x >= KID_ST_X0 && x < KID_ST_X1 && y >= KID_ST_Y0 && y < KID_ST_Y1)
        return kid_staff_px(x, y);
    return kid_pic_px(x, y);
}

/* ---------------------------------------------------------------- WRITE's screen: a big staff, a page of 8 columns */
#define KW_X0 46                        /* the first column's left edge */
#define KW_CW 23                        /* a column's width */
#define KW_Y(s) (128 - 6 * (s))         /* a step's line or space: 12 px between the lines */
#define KW_HOP_W 24                     /* the hopping friend (PLAY) */

static int32_t kw_at(void)                                      /* the column shown: the playing one, or the cursor */
{
    if (song.playing) {
        uint32_t len = trk[0].p[P_SLEN] > 0 ? (uint32_t)trk[0].p[P_SLEN] : 1u;
        return (int32_t)(trk[0].seq_idx % len);
    }
    return kw.cur;
}

static int kw_in_box(int32_t x, int32_t y, int32_t x0, int32_t y0, int32_t x1, int32_t y1, int32_t r)
{
    int32_t cx, cy;
    if (x < x0 || x > x1 || y < y0 || y > y1)
        return 0;
    cx = x < x0 + r ? x0 + r : x > x1 - r ? x1 - r : x;
    cy = y < y0 + r ? y0 + r : y > y1 - r ? y1 - r : y;
    return (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r + r;
}

static int32_t kw_sprite(uint32_t fr, int32_t x, int32_t y, int32_t sz)   /* friend fr at sz x sz, (x, y) inside; -1 clear */
{
    uint32_t rx = (uint32_t)(x * KID_PW / sz), ry = (uint32_t)(y * KID_PW / sz), b;
    if (x < 0 || y < 0 || x >= sz || y >= sz)
        return -1;
    b = KID_PIX[kid_art(fr)][ry * (KID_PW / 2) + (rx >> 1)];
    b = rx & 1u ? b & 15u : b >> 4;
    return b ? (int32_t)KID_PAL[kid_art(fr)][b] : -1;
}

static uint16_t kw_px(int32_t x, int32_t y)
{
    static const uint8_t SPC[7] = {0, 2, 4, 5, 7, 9, 11};
    int32_t at = kw_at(), page = at / (int32_t)KW_PAGE, c, col, cx, j, s, d;
    if (!kw_in_box(x, y, 3, 4, 236, 179, 8))
        return kf.sky[y];
    if (!kw_in_box(x, y, 5, 6, 234, 177, 6))
        return KID_INK;
    if (song.playing && x >= kw.hx && x < kw.hx + KW_HOP_W && (d = kw_sprite(kw.hop_fr, x - kw.hx, y - kw.hy, KW_HOP_W)) >= 0)
        return (uint16_t)d;                                       /* the friend hopping along */
    c = x >= KW_X0 ? (x - KW_X0) / KW_CW : -1;
    if (c >= 0 && c < (int32_t)KW_PAGE) {
        col = page * (int32_t)KW_PAGE + c;
        cx = KW_X0 + c * KW_CW + KW_CW / 2 - 1;
        for (j = 1; j >= 0; j--) {                                /* its notes: a friend in a coloured ball */
            uint32_t n = kw.note[col][j], sh, key;
            int32_t cy, dx, dy, r2;
            if (!n)
                continue;
            key = (n & 31u) - 1u;
            s = kw_step(key, &sh);
            cy = KW_Y(s);
            dx = x - cx;
            dy = y - cy;
            if (sh && (dx - 8) * (dx - 8) + (dy + 8) * (dy + 8) <= 25)   /* a sharp: a # on a white badge */
                return kid_char_ink('#', cx + 6, cy - 11, 1, x, y) ? KID_INK : KID_WHITE;
            r2 = dx * dx + dy * dy;
            if (r2 <= 110) {
                int32_t p = kw_sprite(kw.band[(n >> 5) % 3u], dx + 9, dy + 9, 18);
                if (r2 >= 82)
                    return KID_INK;
                if (p >= 0)
                    return (uint16_t)p;
                return KID_NOTE_COL[(SPC[((s % 7) + 7) % 7] + sh) % 12u];
            }
            if (s <= 0 && dx >= -13 && dx <= 13 && y <= KW_Y(s) && (KW_Y(0) - y) % 12 == 0 && y >= KW_Y(0))
                return KID_INK;                                    /* ledger lines under the staff */
        }
        if (col == at && x <= KW_X0 + c * KW_CW + KW_CW - 2 && y >= 10 && y <= 173) {   /* the column: highlighted */
            if (y >= KW_Y(10) && y <= KW_Y(2) && (KW_Y(2) - y) % 12 == 0)
                return KID_INK;
            return song.playing ? RGB(200, 240, 170) : RGB(255, 236, 160);
        }
    }
    d = (y - 50) / 2;                                             /* the clef, twice its size */
    if (x >= 10 && x < 10 + 2 * KID_CLEF_W && y >= 50 && d < KID_CLEF_H && ((KID_CLEF[d] >> (15 - (x - 10) / 2)) & 1u))
        return KID_INK;
    if (x >= 10 && x < 231 && y >= KW_Y(10) && y <= KW_Y(2) && (KW_Y(2) - y) % 12 == 0)
        return KID_INK;
    for (c = 0; c < 4; c++) {                                     /* the pages: dots */
        int32_t dx = x - (198 + c * 10), dy = y - 166;
        if (dx * dx + dy * dy <= 12)
            return c == page ? KID_INK : RGB(200, 190, 220);
    }
    return kid_mix(kf.sky[y], KID_WHITE, 224u);
}

/* draw x0 .. x1-1, y0 .. y1-1: strips into the canvas's two halves, one drawn while the other goes out */
static void kid_paint(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    uint32_t w, rows, buf = 0;
    int32_t y;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > 240) x1 = 240;
    if (y1 > 240) y1 = 240;
    if (x1 <= x0 || y1 <= y0)
        return;
    w = (uint32_t)(x1 - x0);
    rows = CV_MAX / 2u / w;
    lcd_sync();
    for (y = y0; y < y1; y += (int32_t)rows) {
        uint16_t *px = cv_px + buf * (CV_MAX / 2u), *p = px;
        int32_t yy, x, n = y1 - y < (int32_t)rows ? y1 - y : (int32_t)rows;
        for (yy = y; yy < y + n; yy++) {
            if (kid.hello_ms) {                   /* the hello in 2 x 2 blocks: a quarter of the work (it ran choppy) */
                if ((yy & 1) && yy > y) {
                    for (x = x0; x < x1; x++, p++)
                        *p = p[-(int32_t)w];
                } else {
                    for (x = x0; x < x1; x++, p++)
                        *p = (x & 1) && x > x0 ? p[-1] : swap16(kid_hello_px(x & ~1, yy & ~1));
                }
                continue;
            }
            for (x = x0; x < x1; x++)
                *p++ = swap16(yy < (int32_t)KID_BAND_Y ? (kw.on ? kw_px(x, yy) : kid_view_px(x, yy)) : kid_band_px(x, yy));
        }
        lcd_blit((uint32_t)x0, (uint32_t)y, w, (uint32_t)n, px);
        buf ^= 1u;
    }
}

/* WRITE's frame: the page when it changed, the playing column as it moves, the hopping friend */
static void kw_draw(uint32_t now, uint32_t band)
{
    int32_t at = kw_at(), page = at / (int32_t)KW_PAGE, c;
    uint32_t sig = 0x9E3779B9u ^ (uint32_t)page * 977u ^ (song.playing ? 1u : (uint32_t)kw.cur << 8) ^ kid.night << 20;
    for (c = page * (int32_t)KW_PAGE; c < (page + 1) * (int32_t)KW_PAGE; c++)
        sig = sig * 31u + kw.note[c][0] * 257u + kw.note[c][1];
    sig = sig * 31u + kw.band[0] + kw.band[1] * 64u + kw.band[2] * 4096u;
    if (song.playing) {                                           /* the friend of the column hops over it */
        uint32_t ph = (now - kw.col_ms) * 256u / (kid_beat_ms() ? kid_beat_ms() : 500u), hop;
        if (at != kw.col) {
            kw.col_ms = now;
            ph = 0;
            if (kw.note[at][0])
                kw.hop_fr = kw.band[(kw.note[at][0] >> 5) % 3u];
            else if (kw.note[at][1])
                kw.hop_fr = kw.band[(kw.note[at][1] >> 5) % 3u];
        }
        ph = ph > 256u ? 256u : ph;
        hop = ph * (256u - ph) * 16u / 16384u;                    /* (0 .. 16 px) */
        {
            int16_t ox = kw.hx, oy = kw.hy;
            kw.hx = (int16_t)(KW_X0 + (at % (int32_t)KW_PAGE) * KW_CW - 1);
            kw.hy = (int16_t)(26 - (int32_t)hop);
            if (sig == kw.sig && !kid.full) {
                if (at != kw.col && kw.col >= 0 && kw.col / (int32_t)KW_PAGE == page) {   /* the column moved on */
                    kid_paint(KW_X0 + (kw.col % (int32_t)KW_PAGE) * KW_CW - 2, 8, KW_X0 + (kw.col % (int32_t)KW_PAGE + 1) * KW_CW + 2, 176);
                    kid_paint(KW_X0 + (at % (int32_t)KW_PAGE) * KW_CW - 2, 8, KW_X0 + (at % (int32_t)KW_PAGE + 1) * KW_CW + 2, 176);
                } else {                                          /* the hop alone */
                    int32_t x0 = ox < kw.hx ? ox : kw.hx, y0 = oy < kw.hy ? oy : kw.hy;
                    int32_t x1 = (ox > kw.hx ? ox : kw.hx) + KW_HOP_W, y1 = (oy > kw.hy ? oy : kw.hy) + KW_HOP_W;
                    kid_paint(x0 > 6 ? x0 : 6, y0 > 7 ? y0 : 7, x1 < 234 ? x1 : 234, y1);
                }
            }
        }
    }
    kw.col = (int16_t)at;
    if (sig != kw.sig || kid.full) {
        kid_paint(0, 0, 240, kid.full ? 240 : KID_BAND_Y);
        kw.sig = sig;
        if (kid.full)
            kid.band_sig = band;
        kid.full = 0;
    }
    if (band != kid.band_sig) {
        kid_paint(0, KID_BAND_Y, 240, 240);
        kid.band_sig = band;
    }
}

static void kid_draw(void)
{
    uint32_t now = fm1_ms, i, band = kid_band_icon(kid_band_setup(now)), sig = 0, ssig, top = 0;
    int32_t x0 = 240, y0 = 240, x1 = 0, y1 = 0;
    int anim = kid.scene == KS_STARS || kid.scene == KS_HEARTS || kid.scene == KS_BUBBLES || kid.scene == KS_CONFETTI;
    kid_frame_setup(now);
    if (kid.hello_ms) {                                           /* the hello: the whole screen, every frame */
        if ((now | 1u) - kid.hello_ms > KID_HELLO_MS) {
            kid.hello_ms = 0;
            kid.full = 1;
        } else {
            if (now - kid.hop_ms > 700u)
                kid.hop_ms = now;                                 /* the friend hops along, small, under the name */
            kid_inst(&kf.in[0], 120, 72, 72, kid_hop(now), KID_PAL[kid_art(kid.fr)]);
            kf.n = 1;
            kid_paint(0, 0, 240, 240);
            return;
        }
        kid_frame_setup(now);
    }
    if (kw.on) {
        kw_draw(now, band);
        return;
    }
    ssig = kid_staff_setup(now);
    for (i = 0; i < kf.n; i++) {
        const kid_inst_t *in = &kf.in[i];
        x0 = in->x - in->wig < x0 ? in->x - in->wig : x0;
        x1 = in->x + in->w + in->wig > x1 ? in->x + in->w + in->wig : x1;
        y0 = in->y < y0 ? in->y : y0;
        y1 = in->y + in->h > y1 ? in->y + in->h : y1;
        sig = sig * 31u + (uint32_t)in->x * 7u + (uint32_t)in->y * 131u + (uint32_t)in->w * 3u + (uint32_t)in->h
              + (uint32_t)in->flip * 977u + (uint32_t)(in->pal == kf.ice) * 1931u;
    }
    if (kf.zz_on) {                                               /* SLEEPY's Z: its area too (rising: every frame) */
        x0 = kf.zz.x - 4 < x0 ? kf.zz.x - 4 : x0;
        x1 = kf.zz.x + 26 > x1 ? kf.zz.x + 26 : x1;
        y0 = kf.zz.y - 4 < y0 ? kf.zz.y - 4 : y0;
        sig = sig * 31u + (uint32_t)kf.zz.y;
    }
    if (kid.zz_was) {                                             /* (where it was, when it goes) */
        x0 = x0 < kid.zx0 ? x0 : kid.zx0;
        y0 = y0 < kid.zy0 ? y0 : kid.zy0;
        x1 = x1 > kid.zx0 + 30 ? x1 : kid.zx0 + 30;
    }
    kid.zz_was = kf.zz_on;
    kid.zx0 = (int16_t)(kf.zz.x - 4);
    kid.zy0 = (int16_t)(kf.zz.y - 4);
    if (kid.wiggle)
        sig += kf.ph;
    if (y1 > (int32_t)KID_BAND_Y)
        y1 = KID_BAND_Y;
    if (kid.full) {
        kid.full = 0;
        kid_paint(0, 0, 240, 240);
        kid.band_sig = band;
        kid.bg_ms = now;
        top = 1;
    } else {
        if (kf.party || (kid.party_ms && (now | 1u) - kid.party_ms < 1700u) || (anim && now - kid.bg_ms >= 90u)) {
            kid_paint(0, 0, 240, KID_BAND_Y);                     /* (the confetti's last frame clears it) */
            kid.bg_ms = now;
            top = 1;
        } else if (sig != kid.pic_sig) {                          /* the friends moved: where they were and are */
            kid_paint(x0 < kid.bx0 ? x0 : kid.bx0, y0 < kid.by0 ? y0 : kid.by0,
                      x1 > kid.bx1 ? x1 : kid.bx1, y1 > kid.by1 ? y1 : kid.by1);
        }
        if (ssig != kid.st_sig && !top)                           /* the staff came, wrote a note, or went */
            kid_paint(KID_ST_X0, KID_ST_Y0, KID_ST_X1, KID_ST_Y1);
        if (band != kid.band_sig) {
            kid_paint(0, KID_BAND_Y, 240, 240);
            kid.band_sig = band;
        }
    }
    kid.st_sig = ssig;
    kid.pic_sig = sig;
    kid.bx0 = (int16_t)x0;
    kid.by0 = (int16_t)y0;
    kid.bx1 = (int16_t)x1;
    kid.by1 = (int16_t)y1;
}

/* the main loop's frame (main.c, the browser's web_frame): 1 if Rainbow mode took it */
static void settings_poll(void);                 /* (project.c) */

static int kid_frame(void)
{
    if (!kid.on)
        return 0;
    if (!kid.ready)
        kid_enter();
    kid_input();
    settings_poll();                              /* the VOLUME saved (only while the transport is stopped) */
    if (!kid.on)                                  /* (left just now) */
        return 0;
    kid_leds();
    kid_draw();
    return 1;
}
#define KID_ON() (kid.on)
#else
#define KID_ON() 0
static int kid_frame(void) { return 0; }
static void kid_master(void) {}
#endif
