/* SPDX-License-Identifier: GPL-3.0-only */
/* RAINBOW MODE (FELUCCA_KID): the FM-1 as a toy for a small child. It is on from power-up; holding HOME and SAVE
 * together for 3 s leaves it for the full Felucca until the next power-up.
 *   Keys        play the friend's sound; the screen shows the note's letter, big, in its colour (C red, D orange,
 *               E yellow, F green, G teal, A purple, B pink, as the coloured bells and tubes of music classes),
 *               and the friend hops
 *   PRESETS     the next / previous friend (20, each a picture and a sound: tools/gen_kid_art.py, KID_SOUND)
 *   ALGORITHM   right: three friends sing (a key plays a chord in key), left: one
 *   SELECT      the beat slower / faster
 *   KNOB 1      big / small: the octave (low notes, a big friend; high notes, a small one); OCT- / OCT+ too
 *   KNOB 2      day / night: the sky, the sun and the moon, and the sound gets darker (FX filter)
 *   KNOB 3      echo: delay and reverb, and copies of the friend that follow it
 *   KNOB 4      wiggle: vibrato, and the friend wobbles
 *   FX SCL ENV LFO EDIT GLO   the six skies: rainbow, stars, hearts, bubbles, flowers, confetti
 *   PLAY        the beat (track 4, a drum pattern), the friend dances to it; SEQ: the next beat
 *   ARP         sparkle: a held key plays up and down
 *   HOME        a surprise friend; REC and SAVE a confetti party
 * MASTER is capped at about half (KID_VOL_MAX) for small ears. Everything else (USB, MIDI, the editor, the web
 * installer) works as in the full Felucca.
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
#define KID_VOL_MAX 2048u               /* MASTER at most this (song.master_q12, 4096 = full) */
#define KID_EXIT_MS 3000u
#define KID_HELLO_MS 4000u              /* the hello at power-up, at most this long (any key or knob ends it) */
#define KID_BUB_X 54                    /* the bubble a note shows in */
#define KID_BUB_Y 54
#define KID_BUB_R 50

/* each friend's sound, in the order of KID_NAME: an engine and one of its factory presets (by name), the beat PLAY
 * starts with, and its level (P_LEVEL, 1/2 dB steps: the friends measured alike, about 0.14 peak with MASTER up) */
typedef struct { uint8_t eng; const char *preset; uint8_t beat, level; } kid_sound_t;
static const kid_sound_t KID_SOUND[KID_N] = {
    {12, "TINE EP", 0, 108},       /* DUCKY: FM6 */
    {7, "SOFT FLUTE", 0, 116},     /* PINK DUCKY: WHEEL */
    {0, "SAW LEAD", 1, 102},       /* COOL DUCKY: ANALOG */
    {9, "KALIMBA", 0, 123},        /* AXOLOTL: PHYS */
    {8, "SHIMMER", 0, 114},        /* UNICORN: GRAIN */
    {12, "MARIMBA", 1, 118},       /* GIRAFFE */
    {5, "WOW BASS", 2, 106},       /* GOO: VOICE */
    {6, "FAT BASS", 1, 110},       /* GOOBERT: TRIO */
    {3, "PULSE LD", 1, 103},       /* BLUE PUP: LOFI */
    {5, "VOX LEAD", 0, 96},       /* RED MONSTER */
    {7, "FULL ORGAN", 0, 107},     /* BLUE MONSTER */
    {2, "BRASS", 1, 97},          /* APRIL: PHASE (the family dog) */
    {10, "DRUM KIT", 1, 104},      /* SCISSORS: DRUM, every key another drum */
    {5, "CHOIR AAH", 2, 89},      /* GHOST: oooOOooo, and the spooky beat */
    {6, "SYNC LEAD", 1, 103},      /* WEB HERO */
    {12, "BELL", 0, 112},          /* BUTTERFLY */
    {9, "HARP", 0, 114},           /* KITTY */
    {3, "WAVE BASS", 2, 111},      /* FROG */
    {3, "ARP 8BIT", 1, 108},       /* ROBOT */
    {8, "CLOUD PAD", 0, 108},      /* RAINBOW */
};

/* the beats: 16 steps, a lane bit each (eng_drum.c: 0 kick, 1 snare, 2 clap, 3 closed hat, 4 open hat, 5 tom,
 * 6 rim, 7 cowbell) */
#define KL(k, s, c, h, o, t, r, b) ((k) | (s) << 1 | (c) << 2 | (h) << 3 | (o) << 4 | (t) << 5 | (r) << 6 | (b) << 7)
static const uint8_t KID_BEAT[3][16] = {
    {KL(1,0,0,1,0,0,0,0), 0, KL(0,0,0,1,0,0,0,0), 0, KL(0,1,0,1,0,0,0,0), 0, KL(0,0,0,1,0,0,0,0), 0,   /* DANCE */
     KL(1,0,0,1,0,0,0,0), 0, KL(1,0,0,1,0,0,0,0), 0, KL(0,1,0,1,0,0,0,0), 0, KL(0,0,0,0,1,0,0,0), 0},
    {KL(1,0,0,0,0,0,0,0), 0, KL(0,0,0,1,0,0,0,0), 0, KL(1,0,1,0,0,0,0,0), 0, KL(0,0,0,1,0,0,0,0), 0,   /* MARCH */
     KL(1,0,0,0,0,0,0,0), 0, KL(0,0,0,1,0,0,0,0), 0, KL(1,0,1,0,0,0,0,0), 0, KL(0,0,0,1,0,1,0,0), KL(0,0,0,0,0,1,0,0)},
    {KL(1,0,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0), KL(1,0,0,1,0,0,0,0),               /* SPOOKY */
     KL(0,1,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0), KL(1,0,0,1,0,0,0,1), KL(0,0,0,1,0,0,0,0),
     KL(0,0,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0), KL(1,0,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0),
     KL(0,1,0,1,0,0,0,0), KL(0,0,0,1,0,0,0,0), KL(0,0,0,0,1,0,0,1), KL(0,0,1,0,0,0,1,0)},
};
static const char *const KID_BEAT_NAME[3] = {"DANCE", "MARCH", "SPOOKY"};

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

/* 5 x 7 capitals, digits 1 and 3, '#' and '!': a row a byte, bit 4 the left column */
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
};
static int32_t kid_glyph_of(char c)
{
    if (c >= 'A' && c <= 'Z')
        return c - 'A';
    return c == '1' ? 26 : c == '3' ? 27 : c == '#' ? 28 : c == '!' ? 29 : -1;
}

static const int8_t KID_SIN[32] = {0, 25, 49, 71, 90, 106, 117, 125, 127, 125, 117, 106, 90, 71, 49, 25,
                                   0, -25, -49, -71, -90, -106, -117, -125, -127, -125, -117, -106, -90, -71, -49, -25};

enum { KS_RAINBOW, KS_STARS, KS_HEARTS, KS_BUBBLES, KS_FLOWERS, KS_CONFETTI, KS_COUNT };
static const uint8_t KID_SCENE_BTN[KS_COUNT] = {B_FX, B_SCL, B_ENV, B_LFO, B_EDIT, B_GLO};
enum { KH_NONE, KH_SIZE, KH_NIGHT, KH_ECHO, KH_WIGGLE, KH_SPEED, KH_CHORD, KH_SPARKLE, KH_BEAT };
enum { KB_NAME, KB_NOTE, KB_HINT };

static struct {
    uint8_t on, ready, fr, scene, beat, chord, arp, night, echo, wiggle;
    int8_t oct;                         /* -2 .. 2: song.octave; the friend's size follows */
    int8_t acc;                         /* KNOB 1: detents toward the next octave */
    uint8_t hint, home_down, bub_was;
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

static void kid_beat_load(void)                   /* the beat into track 4 (DRUM) */
{
    track_t *t = &trk[3];
    uint32_t i, l;
    if (t->eng_req != ENGI_DRUM)
        set_engine_of(t, ENGI_DRUM);
    track_defaults_steps(t);
    t->p[P_SLEN] = 16;
    t->p[P_LEVEL] = 92;                           /* (the kit alone peaks twice a friend) */
    for (i = 0; i < 16u; i++)
        for (l = 0; l < NLANE; l++)
            if ((KID_BEAT[kid.beat % 3u][i] >> l) & 1u)
                grid_hit(t, i, l, 1);
}

static void kid_friend(uint32_t f)
{
    kid.fr = (uint8_t)(f % KID_N);
    kid_sound();
    if (kid.beat != KID_SOUND[kid.fr].beat) {
        kid.beat = KID_SOUND[kid.fr].beat;
        kid_beat_load();
    }
    kid.hop_ms = fm1_ms;
    kid.key = -1;
    kid.hint = KH_NONE;
    kid.full = 1;
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

static void kid_input(void)
{
    uint32_t pressed = fm1_input_edges(0), notes = fm1_input_note_edges(), held = fm1_in.buttons, id, k;
    uint32_t home = 1u << panel.btn[B_HOME], both = home | (1u << panel.btn[B_SAVE]);
    int32_t s;
    uint32_t any = pressed | notes;
    fm6_poll();
    song.grid = 0;
    perf_kill = 0;
    for (id = 0; id < 14u; id++) {
        uint32_t b;
        if (!((pressed >> id) & 1u))
            continue;
        b = panel_btn_of(id);
        for (k = 0; k < KS_COUNT; k++)
            if (b == KID_SCENE_BTN[k] && kid.scene != k) {
                kid.scene = (uint8_t)k;
                kid.full = 1;
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
            kid.beat = (uint8_t)((kid.beat + 1u) % 3u);
            kid_beat_load();
            kid_hint(KH_BEAT);
            break;
        case B_REC:
        case B_SAVE:
            kid.party_ms = fm1_ms | 1u;
            kid.hop_ms = fm1_ms;
            break;
        case B_HOME:
            kid.home_down = 1;
            break;
        case B_OCTDN:
        case B_OCTUP:
            kid_size(b == B_OCTUP ? 1 : -1);
            break;
        default:
            break;
        }
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
    if (kid.home_down && !(held & home)) {        /* HOME let go: a surprise friend */
        kid.home_down = 0;
        kid_friend(kid.fr + 1u + rng() % (KID_N - 1u));
        kid.party_ms = fm1_ms | 1u;
    }
    if (notes) {                                  /* a key: its letter, and a hop */
        for (k = 26u; notes >> k == 0u; k--)
            ;
        kid.key = (int8_t)k;
        kid.key_ms = fm1_ms;
        kid.hop_ms = fm1_ms;
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
    led_put(nl, panel.btn[KID_SCENE_BTN[kid.scene % KS_COUNT]], 1);
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
    int16_t x, y, s, wig;               /* top left, size (px), wobble (px) */
    uint16_t inv;                       /* 48 * 256 / s */
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
    uint8_t party;
    /* the band */
    uint8_t band;
    const char *txt;
    kid_txt_t bt;                       /* the band's text */
    int16_t ol;                         /* its outline (px) */
    int8_t bub;                         /* the bubble's note (0 .. 11), -1 none */
    kid_txt_t bubt;
    uint16_t ink, bg, bg2;
    int8_t meter;                       /* dots lit (HINT), -1 none */
} kf;

static int32_t kid_hop(uint32_t t)      /* how high the friend is at ms t */
{
    int32_t h = 0;
    uint32_t d = t - kid.hop_ms;
    if (d < 320u)
        h = (int32_t)(18u * 4u * d * (320u - d) / (320u * 320u));
    if (song.playing) {
        uint32_t bm = 60000u / (uint32_t)clamp(song.g[G_BPM], 40, 240), half = bm / 2u, p = (t - kid.play_ms) % bm;
        if (p < half && (int32_t)(10u * 4u * p * (half - p) / (half * half)) > h)
            h = (int32_t)(10u * 4u * p * (half - p) / (half * half));
    }
    return h;
}

static const uint8_t KID_SIZES[5] = {168, 140, 116, 92, 72};   /* octave -2 .. 2 */

static void kid_inst(kid_inst_t *in, int32_t cx, int32_t s, int32_t hop, const uint16_t *pal)
{
    in->s = (int16_t)s;
    in->inv = (uint16_t)(48u * 256u / (uint32_t)s);
    in->x = (int16_t)(cx - s / 2);
    in->y = (int16_t)(KID_GROUND - s - hop);
    in->wig = (int16_t)(kid.wiggle * s / 96);
    in->pix = KID_PIX[kid.fr % KID_N];
    in->pal = pal;
}

static void kid_frame_setup(uint32_t now)
{
    static const uint16_t DAY_T = RGB(110, 196, 255), DAY_B = RGB(206, 238, 255);
    static const uint16_t NIGHT_T = RGB(18, 22, 70), NIGHT_B = RGB(96, 60, 150);
    uint32_t y, k, ne = kid.echo ? 1u + (kid.echo - 1u) / 3u : 0u, n = 0;
    int32_t s = KID_SIZES[clamp(kid.oct + 2, 0, 4)];
    uint16_t top = kid_mix(DAY_T, NIGHT_T, kid.night * 32u), bot = kid_mix(DAY_B, NIGHT_B, kid.night * 32u);
    kid_inst_t tmp[6];
    kf.t = now;
    kf.ph = now / 45u;
    for (y = 0; y < KID_BAND_Y; y++)
        kf.sky[y] = kid_mix(top, bot, y * 256u / KID_BAND_Y);
    kf.sky_mid = kf.sky[KID_BAND_Y / 2u];
    kf.party = kid.party_ms && (now | 1u) - kid.party_ms < 1600u;
    /* back to front into tmp: the echoes (farthest first), the chord's two friends, the friend */
    for (k = ne; k >= 1u; k--) {
        uint32_t i;
        for (i = 0; i < 16u; i++)
            kf.pal[k - 1u][i] = kid_mix(KID_PAL[kid.fr % KID_N][i], kf.sky_mid, 70u + 50u * k);
        kid_inst(&tmp[n++], 120 - (int32_t)k * (6 + kid.echo * 3), s, kid_hop(now - 110u * k), kf.pal[k - 1u]);
    }
    if (kid.chord) {
        int32_t bs = s * 9 / 16;
        kid_inst(&tmp[n++], 120 - 80, bs, kid_hop(now - 70u), KID_PAL[kid.fr % KID_N]);
        kid_inst(&tmp[n++], 120 + 80, bs, kid_hop(now - 140u), KID_PAL[kid.fr % KID_N]);
    }
    kid_inst(&tmp[n++], 120, s, kid_hop(now), KID_PAL[kid.fr % KID_N]);
    for (k = 0; k < n; k++)
        kf.in[k] = tmp[n - 1u - k];
    kf.n = n;
}

/* the band's content for this frame; returns its signature (a change redraws it) */
static uint32_t kid_band_setup(uint32_t now)
{
    uint32_t sig, len, w;
    kf.meter = -1;
    kf.ol = 2;
    kf.bub = -1;
    if (kid.key >= 0 && now - kid.key_ms < 900u) {
        uint32_t pc = (53u + (uint32_t)kid.key) % 12u;    /* key 0 is F (seq.c kb_map: 53 + k) */
        kf.band = KB_NOTE;
        kf.txt = KID_NOTE[pc];
        kf.bt.sc = 6;
        kf.ol = 3;
        kf.bub = (int8_t)pc;                              /* and the friend sings it, giant, in the bubble */
        kf.bubt.s = kf.txt;
        kf.bubt.sc = kf.txt[1] ? 6 : 10;
        kf.bubt.x = (int16_t)(KID_BUB_X - (int32_t)(str_len(kf.txt) * 6u * (uint32_t)kf.bubt.sc - (uint32_t)kf.bubt.sc) / 2);
        kf.bubt.y = (int16_t)(KID_BUB_Y - 7 * kf.bubt.sc / 2);
        kf.ink = KID_NOTE_COL[pc];
        kf.bg = kid_mix(KID_NOTE_COL[pc], KID_WHITE, 170u);
        kf.bg2 = kid_mix(KID_NOTE_COL[pc], KID_WHITE, 90u);
        sig = 0x100u | pc;
    } else if (kid.hint != KH_NONE && now - kid.hint_ms < 1500u) {
        static const char *const WORD[] = {"", "", "DAY", "ECHO", "WIGGLE", "", "", "SPARKLE", ""};
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
        case KH_BEAT: kf.txt = KID_BEAT_NAME[kid.beat % 3u]; v = -1; break;
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
    len = str_len(kf.txt);
    w = len * 6u * (uint32_t)kf.bt.sc - (uint32_t)kf.bt.sc;
    kf.bt.s = kf.txt;
    kf.bt.x = (int16_t)((240 - (int32_t)w) / 2);
    kf.bt.y = (int16_t)(KID_BAND_Y + ((kf.band == KB_HINT ? 40 : 56) - 7 * kf.bt.sc) / 2 + 2);
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
        if ((uint32_t)sy >= (uint32_t)in->s)
            continue;
        ry = sy * in->inv >> 8;
        xo = in->wig ? KID_SIN[((uint32_t)ry / 3u + kf.ph) & 31u] * in->wig / 127 : 0;
        sx = x - in->x - xo;
        if ((uint32_t)sx >= (uint32_t)in->s)
            continue;
        rx = sx * in->inv >> 8;
        b = in->pix[ry * 24 + (rx >> 1)];
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
    if (kf.bub >= 0) {                                        /* the bubble: the note, giant */
        int32_t dx = x - KID_BUB_X, dy = y - KID_BUB_Y, d = dx * dx + dy * dy, k;
        if (d <= KID_BUB_R * KID_BUB_R) {
            if (d >= (KID_BUB_R - 2) * (KID_BUB_R - 2))
                return KID_INK;
            if (d >= (KID_BUB_R - 7) * (KID_BUB_R - 7))
                return KID_NOTE_COL[kf.bub];
            if ((k = kid_text_at(&kf.bubt, x, y, 3)) != 0)
                return k == 1 ? KID_NOTE_COL[kf.bub] : KID_INK;
            return KID_WHITE;
        }
        if (dx > 20 && dx < 46 && dy > 20 && dy < 46 && dx + dy < 70 && (dx - dy < 6 && dy - dx < 6))
            return dx + dy > 64 || dx - dy > 3 || dy - dx > 3 ? KID_INK : KID_NOTE_COL[kf.bub];   /* its tail */
    }
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
        for (yy = y; yy < y + n; yy++)
            for (x = x0; x < x1; x++)
                *p++ = swap16(kid.hello_ms ? kid_hello_px(x, yy) :
                              yy < (int32_t)KID_BAND_Y ? kid_pic_px(x, yy) : kid_band_px(x, yy));
        lcd_blit((uint32_t)x0, (uint32_t)y, w, (uint32_t)n, px);
        buf ^= 1u;
    }
}

static void kid_draw(void)
{
    uint32_t now = fm1_ms, i, band = kid_band_setup(now), sig = 0;
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
            kid_inst(&kf.in[0], 120, 56, kid_hop(now), KID_PAL[kid.fr % KID_N]);
            kf.n = 1;
            kid_paint(0, 0, 240, 240);
            return;
        }
        kid_frame_setup(now);
    }
    for (i = 0; i < kf.n; i++) {
        const kid_inst_t *in = &kf.in[i];
        x0 = in->x - in->wig < x0 ? in->x - in->wig : x0;
        x1 = in->x + in->s + in->wig > x1 ? in->x + in->s + in->wig : x1;
        y0 = in->y < y0 ? in->y : y0;
        y1 = in->y + in->s > y1 ? in->y + in->s : y1;
        sig = sig * 31u + (uint32_t)in->x * 7u + (uint32_t)in->y * 131u + (uint32_t)in->s;
    }
    if (kid.wiggle)
        sig += kf.ph;
    sig = sig * 31u + (uint32_t)(kf.bub + 1);
    if (kf.bub >= 0 || kid.bub_was) {                             /* the bubble came, changed or went */
        x0 = x0 < 0 ? x0 : 0;
        y0 = y0 < 0 ? y0 : 0;
        x1 = x1 > KID_BUB_X + 50 ? x1 : KID_BUB_X + 50;
        y1 = y1 > KID_BUB_Y + 50 ? y1 : KID_BUB_Y + 50;
    }
    kid.bub_was = kf.bub >= 0;
    if (y1 > (int32_t)KID_BAND_Y)
        y1 = KID_BAND_Y;
    if (kid.full) {
        kid.full = 0;
        kid_paint(0, 0, 240, 240);
        kid.band_sig = band;
        kid.bg_ms = now;
    } else {
        if (kf.party || (kid.party_ms && (now | 1u) - kid.party_ms < 1700u) || (anim && now - kid.bg_ms >= 90u)) {
            kid_paint(0, 0, 240, KID_BAND_Y);                     /* (the confetti's last frame clears it) */
            kid.bg_ms = now;
        } else if (sig != kid.pic_sig) {                          /* the friends moved: where they were and are */
            kid_paint(x0 < kid.bx0 ? x0 : kid.bx0, y0 < kid.by0 ? y0 : kid.by0,
                      x1 > kid.bx1 ? x1 : kid.bx1, y1 > kid.by1 ? y1 : kid.by1);
        }
        if (band != kid.band_sig) {
            kid_paint(0, KID_BAND_Y, 240, 240);
            kid.band_sig = band;
        }
    }
    kid.pic_sig = sig;
    kid.bx0 = (int16_t)x0;
    kid.by0 = (int16_t)y0;
    kid.bx1 = (int16_t)x1;
    kid.by1 = (int16_t)y1;
}

/* the main loop's frame (main.c, the browser's web_frame): 1 if Rainbow mode took it */
static int kid_frame(void)
{
    if (!kid.on)
        return 0;
    if (!kid.ready)
        kid_enter();
    kid_input();
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
#endif
