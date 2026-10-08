/* SPDX-License-Identifier: GPL-3.0-only */
/* RAINBOW MODE (FELUCCA_KID): the FM-1 as a toy for a small child. It is on from power-up; holding HOME and SAVE
 * together for 3 s leaves it for the full Felucca until the next power-up.
 *   Keys        play the friend's sound; the band shows the note's letter, big, in its colour (C red, D orange,
 *               E yellow, F green, G teal, A purple, B pink, as the coloured bells and tubes of music classes),
 *               and the friend hops
 *   PRESETS     the next / previous friend (40, each a picture, a sound, a home sky and a favourite beat:
 *               tools/gen_kid_art.py, KID_SOUND)
 *   ALGORITHM   right: three friends sing (a key plays a chord in key), left: one
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
 *               of ten beats
 *   ARP         sparkle: a held key plays up and down
 *   REC         the next sky (rainbow, stars, hearts, bubbles, flowers, confetti), with confetti
 *   HOME        a surprise friend; SAVE a confetti party
 *   THE STAFF   a treble clef drops in at the top when she plays, and writes her notes as coloured noteheads, up to 8,
 *               chords stacked, sharps with a #, ledger lines, 8VA / 8VB over the staff for the octave (OCT); it goes
 *               after 6 s of quiet. The big letter stays in the band
 *   HOME held 1 s, then OCT- / OCT+   the grown-ups' VOLUME: how loud MASTER can go (8 steps, kept over power-off)
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
#define KID_VOL_BYTE (favorites.factory[15][26])   /* the VOLUME level, ^ 6 (0 = 6, the default; settings_persist.c) */
#define KID_VOL_DEF 6u
#define KID_VOL_HOLD_MS 1000u           /* HOME held this long opens the VOLUME (then OCT- / OCT+) */
#define KID_LIM_X2 15u                  /* the limiter's ceiling is song.master_q12 * 7.5 (a plain chord over the beat peaks
                                         * at about 0.8 of it, any friend: web/emu/kid_test.mjs) */
/* MASTER at most this at VOLUME 1 .. 8 (song.master_q12, 4096 = full): steps of about 3 dB; 6 is the old half */
static const uint16_t KID_VOL_CAP[8] = {362, 512, 724, 1024, 1448, 2048, 2896, 4096};
#define KID_EXIT_MS 3000u
#define KID_HELLO_MS 4000u              /* the hello at power-up, at most this long (any key or knob ends it) */
#define KID_SLEEP_MS 3000u              /* SLEEPY: winds down, dozes, then wakes up by itself after this */

enum { KS_RAINBOW, KS_STARS, KS_HEARTS, KS_BUBBLES, KS_FLOWERS, KS_CONFETTI, KS_COUNT };
enum { KB_DANCE, KB_MARCH, KB_SPOOKY, KB_ROCK, KB_DISCO, KB_HIPHOP, KB_TRAIN, KB_SAMBA, KB_REGGAE, KB_LULLABY,
       KB_COUNT };

/* each friend's sound, in the order of KID_NAME: an engine and one of its factory presets (by name), its level
 * (P_LEVEL, 1/2 dB steps: the friends measured alike, about 0.14 peak with MASTER up), its home sky and its
 * favourite beat (PLAY) */
typedef struct { uint8_t eng; const char *preset; uint8_t level, sky, beat; } kid_sound_t;
static const kid_sound_t KID_SOUND[KID_N] = {
    {12, "TINE EP", 108, KS_BUBBLES, KB_DANCE},       /* DUCKY: FM6 */
    {7, "SOFT FLUTE", 116, KS_HEARTS, KB_DISCO},      /* PINK DUCKY: WHEEL */
    {0, "SAW LEAD", 102, KS_CONFETTI, KB_HIPHOP},     /* COOL DUCKY: ANALOG */
    {9, "KALIMBA", 123, KS_BUBBLES, KB_REGGAE},       /* AXOLOTL: PHYS */
    {8, "SHIMMER", 114, KS_RAINBOW, KB_DISCO},        /* UNICORN: GRAIN */
    {12, "MARIMBA", 118, KS_FLOWERS, KB_SAMBA},       /* GIRAFFE */
    {5, "WOW BASS", 106, KS_HEARTS, KB_DANCE},        /* GOO: VOICE */
    {6, "FAT BASS", 110, KS_CONFETTI, KB_HIPHOP},     /* GOOBERT: TRIO */
    {3, "PULSE LD", 103, KS_FLOWERS, KB_ROCK},        /* BLUE PUP: LOFI */
    {5, "VOX LEAD", 96, KS_CONFETTI, KB_MARCH},       /* RED MONSTER */
    {7, "FULL ORGAN", 107, KS_RAINBOW, KB_DISCO},     /* BLUE MONSTER */
    {2, "BRASS", 97, KS_FLOWERS, KB_TRAIN},           /* APRIL: PHASE (the family dog) */
    {10, "DRUM KIT", 104, KS_CONFETTI, KB_ROCK},      /* SCISSORS: DRUM, every key another drum */
    {5, "CHOIR AAH", 89, KS_STARS, KB_SPOOKY},        /* GHOST: oooOOooo, and the spooky beat */
    {6, "SYNC LEAD", 103, KS_STARS, KB_ROCK},         /* WEB HERO */
    {12, "BELL", 112, KS_FLOWERS, KB_LULLABY},        /* BUTTERFLY */
    {9, "HARP", 114, KS_HEARTS, KB_SAMBA},            /* KITTY */
    {3, "WAVE BASS", 111, KS_BUBBLES, KB_REGGAE},     /* FROG */
    {3, "ARP 8BIT", 108, KS_STARS, KB_HIPHOP},        /* ROBOT */
    {8, "CLOUD PAD", 108, KS_RAINBOW, KB_LULLABY},    /* RAINBOW */
    {6, "CHIP CHOIR", 97, KS_HEARTS, KB_DISCO},      /* PRINCESS DUCKY: TRIO */
    {3, "STEP LEAD", 103, KS_FLOWERS, KB_ROCK},       /* RED PUP: LOFI */
    {0, "SINE KEY", 105, KS_FLOWERS, KB_MARCH},       /* YELLOW BIRD: ANALOG */
    {8, "GLITCH", 107, KS_STARS, KB_SPOOKY},          /* SLIMY: GRAIN, and the spooky beat */
    {9, "PLUCK", 121, KS_FLOWERS, KB_DANCE},          /* BUNNY: PHYS */
    {12, "PAD", 113, KS_RAINBOW, KB_HIPHOP},          /* PANDA: FM6 */
    {3, "WAVE LEAD", 109, KS_STARS, KB_MARCH},        /* PENGUIN: LOFI */
    {5, "WHISPER", 97, KS_STARS, KB_LULLABY},        /* OWL: VOICE, hoo */
    {6, "ARP LEAD", 101, KS_FLOWERS, KB_SAMBA},       /* BEE: TRIO, bzz */
    {9, "BELL TREE", 113, KS_FLOWERS, KB_DANCE},      /* LADYBUG: PHYS */
    {0, "ACID", 105, KS_CONFETTI, KB_ROCK},           /* DINO: ANALOG */
    {8, "FROZEN", 125, KS_BUBBLES, KB_LULLABY},       /* WHALE: GRAIN */
    {9, "MARIMBA", 120, KS_BUBBLES, KB_REGGAE},       /* OCTOPUS: PHYS */
    {12, "PLUCK", 119, KS_BUBBLES, KB_REGGAE},        /* FISH: FM6 */
    {9, "HAND DRUM", 111, KS_BUBBLES, KB_LULLABY},    /* TURTLE: PHYS */
    {6, "RING BELL", 111, KS_CONFETTI, KB_DISCO},     /* ICE CREAM: TRIO */
    {7, "JAZZ PERC", 106, KS_HEARTS, KB_DANCE},       /* CUPCAKE: WHEEL */
    {11, "ARCADE", 115, KS_STARS, KB_HIPHOP},         /* ROCKET: NOISE */
    {0, "PLUCK", 106, KS_HEARTS, KB_SAMBA},           /* STRAWBERRY: ANALOG */
    {2, "BELL", 108, KS_STARS, KB_LULLABY},           /* STAR: PHASE */
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
    {"MARCH", 100, KD_HOP, 92, 100,
     {"x.......x.......", "....x.......x.x.", 0, 0, 0, "..............xx", "x...x...x...x...", 0},
     {36, 0, 0, 0, 43, 0, 0, 0, 36, 0, 0, 0, 43, 0, 0, 0}},
    {"SPOOKY", 116, KD_BOTH, 89, 98,                 /* a ghost-hunting funk (in the spirit, not the tune) */
     {"x..x...x..x.....", "....x.......x...", "....x.......x..x", "x.xxx.xxx.xxx.x.", "......x.......x.", 0, 0,
      "x.....x...x....."},
     {33, 0, 0, 45, 0, 33, 0, 0, 36, 0, 38, 0, 40, 0, 38, 36}},
    {"ROCK", 120, KD_HOP, 92, 100,
     {"x.....x.x.......", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", "..............x.", 0, 0, 0},
     {36, 0, 36, 0, 36, 0, 36, 0, 41, 0, 41, 0, 43, 0, 43, 0}},
    {"DISCO", 118, KD_BOTH, 88, 98,
     {"x...x...x...x...", "....x.......x...", "....x.......x...", "x.x.x.x.x.x.x.x.", "..x...x...x...x.", 0, 0, 0},
     {36, 0, 48, 0, 36, 0, 48, 0, 41, 0, 53, 0, 43, 0, 55, 0}},
    {"HIP HOP", 90, KD_SWAY, 92, 104,
     {"x......x..x.....", "....x.......x...", 0, "x.x.x.x.x.x.x.x.", 0, 0, "...........x....", 0},
     {33, 0, 0, 0, 0, 0, 0, 33, 0, 0, 31, 0, 0, 0, 0, 0}},
    {"TRAIN", 132, KD_HOP, 88, 100,                   /* choo choo */
     {"x.......x.......", "..x...x...x...x.", 0, "xxxxxxxxxxxxxxxx", 0, 0, 0, 0},
     {36, 0, 0, 0, 43, 0, 0, 0, 41, 0, 0, 0, 43, 0, 0, 0}},
    {"SAMBA", 100, KD_BOTH, 90, 100,
     {"x..xx..xx..xx..x", 0, 0, "xxxxxxxxxxxxxxxx", 0, 0, "x.x..x.x.x..x.x.", "..x...x...x...x."},
     {36, 0, 0, 43, 36, 0, 0, 43, 36, 0, 0, 43, 36, 0, 0, 43}},
    {"REGGAE", 80, KD_SWAY, 89, 104,
     {"........x.......", 0, 0, "x.x.x.x.x.x.x.x.", 0, 0, "........x.......", 0},
     {0, 0, 33, 0, 36, 0, 0, 0, 40, 0, 0, 0, 38, 0, 36, 0}},
    {"LULLABY", 70, KD_SLOW, 92, 106,
     {"x.......x.......", 0, 0, 0, 0, 0, "....x.......x...", 0},
     {36, 1, 1, 1, 1, 1, 1, 1, 31, 1, 1, 1, 1, 1, 1, 1}},
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

/* 5 x 7 capitals, digits 1, 3, 5 and 8, '#' and '!': a row a byte, bit 4 the left column */
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
};
static int32_t kid_glyph_of(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    return c == '1' ? 26 : c == '3' ? 27 : c == '#' ? 28 : c == '!' ? 29 : c == '5' ? 30 : c == '8' ? 31 : -1;
}

static const int8_t KID_SIN[32] = {0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49, 25,
                                   0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117, -106, -90, -71, -49, -25};

enum { KH_NONE, KH_SIZE, KH_NIGHT, KH_ECHO, KH_WIGGLE, KH_SPEED, KH_CHORD, KH_SPARKLE, KH_BEAT, KH_VOLUME };
enum { KB_NAME, KB_NOTE, KB_HINT };

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
} kid_st_t;

static struct {
    uint8_t on, ready, fr, scene, beat, chord, arp, night, echo, wiggle;
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
    uint8_t full;                       /* draw everything next frame */
    uint32_t hint_ms, key_ms, hop_ms, party_ms, exit_ms, play_ms, bg_ms;
    uint32_t hello_ms;                  /* the power-up hello since (fm1_ms | 1), 0 none */
    /* (the times stored as fm1_ms | 1 are compared from now | 1: never "in the future") */
    uint32_t band_sig, pic_sig;         /* what the band and the picture last showed */
    int16_t bx0, by0, bx1, by1;         /* the area the friends covered last frame */
} kid = {1};

/* ---------------------------------------------------------------- sound */
static void kid_knobs(void)                       /* the knobs' state into the friend's track (track 1) */
{
    track_t *t = &trk[0];
    t->p[P_DLY] = (int16_t)clamp(kid.dly0 + kid.echo * 14, 0, 127);
    t->p[P_REV] = (int16_t)clamp(kid.rev0 + kid.echo * 6, 0, 127);
    t->p[P_LD_PIT] = (int16_t)(kid.wiggle * 5);
    t->p[P_LRATE] = 74;
    t->p[P_CHRD] = kid.chord ? CH_DIA3 : CH_OFF;
    t->p[P_AMODE] = kid.arp ? 3 : 0;              /* UPDN */
    t->p[P_ARATE] = 2;                            /* 1/16 */
    perf_k[0] = (int8_t)-(kid.night * 8);         /* the FX filter: night closes it to ~700 Hz */
    song.octave = kid.oct;
}

static void kid_sound(void)
{
    const kid_sound_t *s = &KID_SOUND[kid.fr % KID_N];
    const engine_t *e = ENGINES[s->eng % NENGINES];
    track_t *t = &trk[0];
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
    kid.rev0 = (uint8_t)t->p[P_REV];
    kid.dly0 = (uint8_t)t->p[P_DLY];
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
    uint32_t l = (uint32_t)KID_VOL_BYTE ^ 6u;
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
static void kid_leave(void)
{
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
    KID_VOL_BYTE = (uint8_t)(l ^ 6u);
    settings_save();
    kid_hint(KH_VOLUME);
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
        kid.st_t0 = now;
    }
    for (k = 0; k < 27u; k++) {
        uint32_t n = 53u + k, pc = n % 12u, h;
        int32_t st = ((int32_t)(n / 12u) - 5) * 7 + IDX[pc];
        if (!((keys >> k) & 1u))
            continue;
        for (h = 0; h < (kid.chord ? 3u : 1u); h++) {
            uint32_t d = e->n, sh = h ? 0u : SHARP[pc], i;
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
            if (song.playing || chain_busy()) {
                transport_req = 2;
            } else {
                transport_req = 1;
                kid.play_ms = fm1_ms;
            }
            break;
        case B_ARP:
            kid.arp ^= 1u;
            kid_knobs();
            kid_hint(KH_SPARKLE);
            break;
        case B_SEQ:
            kid.beat = (uint8_t)((kid.beat + 1u) % KB_COUNT);
            kid_beat_load();
            kid_hint(KH_BEAT);
            break;
        case B_REC:                               /* the next sky, with a party */
            kid.scene = (uint8_t)((kid.scene + 1u) % KS_COUNT);
            kid.full = 1;
            /* fall through */
        case B_SAVE:
            kid.party_ms = fm1_ms | 1u;
            kid.hop_ms = fm1_ms;
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
            else
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
    if (kid.home_down && !(held & home)) {        /* HOME let go: a surprise friend */
        kid.home_down = 0;
        kid_friend(kid.fr + 1u + rng() % (KID_N - 1u));
        kid.party_ms = fm1_ms | 1u;
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
        for (k = 26u; notes >> k == 0u; k--)
            ;
        kid.key = (int8_t)k;
        kid.key_ms = fm1_ms;
        kid.hop_ms = fm1_ms;
        kid_staff_add(notes, fm1_ms);
    } else if (kid.key >= 0 && ((fm1_in.notes >> kid.key) & 1u)) {
        kid.key_ms = fm1_ms;                      /* (held: it stays) */
    } else if (kid.key >= 0 && fm1_in.notes) {    /* let go with others held: the highest of them */
        for (k = 26u; fm1_in.notes >> k == 0u; k--)
            ;
        kid.key = (int8_t)k;
        kid.key_ms = fm1_ms;
    }
    if ((s = panel_enc(EN_PRESET)) != 0) {
        any = 1u;
        kid_friend((uint32_t)((int32_t)kid.fr + (s > 0 ? 1 : (int32_t)KID_N - 1)));
    }
    if ((s = panel_enc(EN_ALGO)) != 0) {
        any = 1u;
        kid.chord = s > 0;
        kid_knobs();
        kid_hint(KH_CHORD);
    }
    if ((s = panel_enc(EN_SELECT)) != 0) {
        any = 1u;
        song.g[G_BPM] = (int16_t)clamp(song.g[G_BPM] + s * 5, 60, 180);
        kid_hint(KH_SPEED);
    }
    if ((s = panel_enc(EN_K1)) != 0) {            /* two detents an octave: a small hand turns a lot */
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
    if ((s = panel_enc(EN_K3)) != 0) {
        any = 1u;
        kid.echo = (uint8_t)clamp(kid.echo + s, 0, 8);
        kid_knobs();
        kid_hint(KH_ECHO);
    }
    if ((s = panel_enc(EN_K4)) != 0) {
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
    led_put(nl, panel.btn[B_ARP], kid.arp);
    for (k = 0; k < 27u; k++) {
        led_put(nl, 14u + k, (int)((fm1_in.notes >> k) & 1u));
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
    int16_t ol;                         /* its outline (px) */
    uint16_t ink, bg, bg2;
    int8_t meter;                       /* dots lit (HINT), -1 none */
    uint8_t staff;                      /* the staff shows */
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
    in->pix = KID_PIX[kid.fr % KID_N];
    in->pal = pal;
}

static void kid_frame_setup(uint32_t now)
{
    static const uint16_t DAY_T = RGB(110, 196, 255), DAY_B = RGB(206, 238, 255);
    static const uint16_t NIGHT_T = RGB(18, 22, 70), NIGHT_B = RGB(96, 60, 150);
    uint32_t y, k, ne = kid.echo ? 1u + (kid.echo - 1u) / 3u : 0u, n = 0, fx = kid.fx;
    int32_t s = KID_SIZES[clamp(kid.oct + 2, 0, 4)], h, jx = 0, sway = kid_sway(now);
    uint16_t top = kid_mix(DAY_T, NIGHT_T, kid.night * 32u), bot = kid_mix(DAY_B, NIGHT_B, kid.night * 32u);
    const uint16_t *pal = KID_PAL[kid.fr % KID_N];
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
static uint32_t kid_band_setup(uint32_t now)
{
    uint32_t sig, len, w;
    int vol = kid.hint == KH_VOLUME && now - kid.hint_ms < 1500u;     /* (the grown-ups' VOLUME comes before all) */
    kf.meter = -1;
    kf.ol = 2;
    if (!vol && kid.key >= 0 && now - kid.key_ms < 900u) {      /* a note: its letter, big (the effect's word, if one is on) */
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
        static const char *const WORD[] = {"", "", "DAY", "ECHO", "WIGGLE", "", "", "SPARKLE", "", "VOLUME"};
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
        case KH_CHORD: kf.txt = kid.chord ? "3 FRIENDS" : "1 FRIEND"; v = kid.chord ? 8 : 0; break;
        case KH_SPARKLE: v = kid.arp ? 8 : 0; break;
        case KH_BEAT: kf.txt = KID_BEATS[kid.beat % KB_COUNT].name; v = -1; break;
        case KH_VOLUME: v = (int32_t)kid_vol_level(); break;
        default: break;
        }
        kf.meter = (int8_t)clamp(v, -1, 8);
        kf.bt.sc = 3;
        kf.ink = KID_WHITE;
        kf.bg = RGB(150, 110, 230);
        kf.bg2 = RGB(120, 84, 200);
        sig = 0x200u | (uint32_t)kid.hint << 4 | (uint32_t)(v + 1) << 12 | (uint32_t)kid.beat << 20;
    } else {
        kf.band = KB_NAME;
        kf.txt = KID_NAME[kid.fr % KID_N];
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

static uint16_t kid_band_px(int32_t x, int32_t y)
{
    int k = kid_text_at(&kf.bt, x, y, kf.ol);
    if (!k && kf.bt2.sc)
        k = kid_text_at(&kf.bt2, x, y, kf.ol);
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
    for (i = 0; i < kid.st_n; i++) {
        sig = sig * 31u + kid.st[i].n + (uint32_t)kid.st[i].sharp * 7u;
        for (j = 0; j < kid.st[i].n; j++)
            sig = sig * 31u + (uint8_t)kid.st[i].step[j];
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
        int32_t cx = 56 + 21 * i, dx = x - cx, lo = 99, hi = -99, ls;
        for (h = 0; h < (int32_t)e->n; h++) {
            int32_t s = e->step[h], dy = y - KID_ST_Y(s), a = dx * dx, pc;
            lo = s < lo ? s : lo;
            hi = s > hi ? s : hi;
            if (dx >= -4 && dx <= 4 && dy >= -3 && dy <= 3 && 49 * a + 81 * dy * dy <= 992) {
                pc = (SPC[((s % 7) + 7) % 7] + (int32_t)((e->sharp >> h) & 1u)) % 12;
                return 36 * a + 49 * dy * dy <= 440 ? KID_NOTE_COL[pc] : KID_INK;
            }
            if (((e->sharp >> h) & 1u) && kid_char_ink('#', cx - 12, KID_ST_Y(s) - 3, 1, x, y))
                return KID_INK;
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
                *p++ = swap16(yy < (int32_t)KID_BAND_Y ? kid_view_px(x, yy) : kid_band_px(x, yy));
        }
        lcd_blit((uint32_t)x0, (uint32_t)y, w, (uint32_t)n, px);
        buf ^= 1u;
    }
}

static void kid_draw(void)
{
    uint32_t now = fm1_ms, i, band = kid_band_setup(now), sig = 0, ssig, top = 0;
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
            kid_inst(&kf.in[0], 120, 72, 72, kid_hop(now), KID_PAL[kid.fr % KID_N]);
            kf.n = 1;
            kid_paint(0, 0, 240, 240);
            return;
        }
        kid_frame_setup(now);
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
