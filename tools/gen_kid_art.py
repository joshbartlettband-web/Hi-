#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rainbow mode's friends: 20 pixel-art pictures (run by tools/build.py generate()).

  gen_kid_art.py OUT.h [--png SHEET.png]

Each friend is drawn from shapes on a 48 x 48 grid (no antialiasing), then outlined: every empty pixel that
touches the picture becomes the outline colour. At most 15 colours per picture (index 0 = see-through); the
header stores 4 bits per pixel and a 16-entry RGB565 palette per picture, in the order of FRIENDS below
(firmware/src/kid.c matches its sounds to the same order). --png writes a contact sheet for checking by eye.
KID_NAME in the environment (a GitHub secret in the workflow) is the name the power-up hello greets: capitals,
spaces and at most 10 letters are kept, "!" is added. Without it the hello is a plain HI!, so the name is never
in the source.
"""
import os
import re
import sys
from PIL import Image, ImageDraw

N = 48
OUT = (52, 26, 58)               # the outline: a dark plum, softer than black
C = dict(
    white=(255, 255, 255), black=(28, 20, 36), cream=(255, 244, 214), grey=(176, 180, 200), dgrey=(96, 98, 120),
    yellow=(255, 216, 46), gold=(255, 178, 30), orange=(255, 132, 36), red=(236, 52, 64), dred=(170, 24, 48),
    pink=(255, 138, 196), lpink=(255, 200, 226), hpink=(236, 64, 150), peach=(255, 190, 150),
    blue=(56, 128, 244), lblue=(150, 206, 255), dblue=(30, 56, 150), navy=(30, 36, 90), teal=(36, 196, 190),
    green=(70, 200, 90), lime=(176, 232, 70), dgreen=(36, 130, 64), purple=(150, 88, 226), lpurple=(204, 164, 255),
    brown=(150, 92, 46), pupblue=(110, 156, 226), tan=(226, 176, 112), dbrown=(84, 50, 32), lgreen=(190, 250, 170),
)


class Pic:
    def __init__(self):
        self.im = Image.new("RGBA", (N, N), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def ell(self, cx, cy, rx, ry, c):
        self.d.ellipse([cx - rx, cy - ry, cx + rx, cy + ry], fill=C[c] + (255,))

    def rect(self, x0, y0, x1, y1, c):
        self.d.rectangle([x0, y0, x1, y1], fill=C[c] + (255,))

    def poly(self, pts, c):
        self.d.polygon(pts, fill=C[c] + (255,))

    def line(self, pts, c, w=1):
        self.d.line(pts, fill=C[c] + (255,), width=w)

    def px(self, x, y, c):
        if 0 <= x < N and 0 <= y < N:
            self.im.putpixel((x, y), C[c] + (255,))

    def eye(self, x, y, big=False):           # a shiny eye: white ring, dark pupil, a white spark
        if big:
            self.ell(x, y, 4, 5, "white")
            self.ell(x, y + 1, 2, 3, "black")
            self.px(x - 1, y - 1, "white")
        else:
            self.ell(x, y, 2, 2, "black")
            self.px(x - 1, y - 1, "white")

    def smile(self, x, y, w=3, c="black"):
        for i in range(-w, w + 1):
            self.px(x + i, y + (1 if abs(i) < w else 0), c)

    def cheek(self, x, y):
        self.ell(x, y, 2, 1, "pink")

    def outline(self):
        src = self.im.copy()
        for y in range(N):
            for x in range(N):
                if src.getpixel((x, y))[3]:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < N and 0 <= yy < N and src.getpixel((xx, yy))[3]:
                        self.im.putpixel((x, y), OUT + (255,))
                        break
        return self


# ---------------------------------------------------------------- the friends
def ducky(body, wing, beak, extra=None):
    def f():
        p = Pic()
        p.ell(22, 33, 15, 10, body)                    # body
        p.poly([(34, 26), (44, 20), (40, 32)], body)  # tail up at the back... (drawn right: the duck faces left)
        p.ell(18, 17, 9, 9, body)                      # head
        p.ell(8, 20, 5, 2, beak)                       # beak
        p.ell(25, 31, 8, 5, wing)                      # wing
        p.ell(24, 30, 7, 3, body)
        p.eye(16, 15)
        p.cheek(20, 21)
        if extra:
            extra(p)
        return p.outline()
    return f


def bow(p):
    p.ell(14, 8, 3, 2, "hpink"); p.ell(21, 8, 3, 2, "hpink"); p.rect(17, 7, 18, 9, "red")


def shades(p):
    p.rect(10, 13, 23, 14, "black"); p.ell(13, 16, 3, 2, "black"); p.ell(20, 16, 3, 2, "black")
    p.px(12, 15, "lblue"); p.px(19, 15, "lblue")
    p.rect(15, 4, 21, 9, "teal"); p.rect(12, 9, 24, 10, "teal")   # a little cap


def axolotl():
    p = Pic()
    p.poly([(36, 30), (47, 22), (46, 38)], "lpink")     # tail fin
    p.ell(28, 32, 13, 8, "pink")                         # body
    p.ell(20, 41, 3, 3, "pink"); p.ell(33, 41, 3, 3, "pink")   # legs
    for gx, gy in ((4, 10), (2, 18), (5, 26), (34, 10), (36, 18), (33, 26)):   # frilly gills
        p.ell(gx, gy, 4, 2, "hpink")
    p.ell(19, 19, 13, 11, "pink")                        # head
    p.eye(13, 17); p.eye(25, 17)
    p.smile(19, 23, 4)
    p.cheek(10, 22); p.cheek(28, 22)
    return p.outline()


def unicorn():
    p = Pic()
    p.poly([(12, 44), (14, 28), (30, 28), (34, 44)], "white")   # neck
    p.ell(22, 24, 11, 10, "white")                       # head
    p.ell(12, 31, 8, 6, "white")                         # snout
    p.ell(8, 33, 4, 3, "lpink")
    p.px(7, 32, "dgrey"); p.px(10, 32, "dgrey")
    p.poly([(20, 14), (26, 0), (28, 15)], "gold")        # horn
    p.line([(21, 11), (26, 9)], "yellow"); p.line([(23, 6), (26, 5)], "yellow")
    p.poly([(26, 14), (30, 8), (33, 18)], "white")       # ear
    for i, c in enumerate(("red", "orange", "yellow", "green", "blue", "purple")):   # rainbow mane
        p.ell(33 + i // 2, 18 + i * 4, 5, 3, c)
    p.eye(19, 22)
    p.cheek(17, 28)
    return p.outline()


def giraffe():
    p = Pic()
    p.rect(24, 14, 32, 46, "yellow")                     # neck
    for x, y in ((26, 20), (30, 27), (26, 34), (30, 41)):
        p.ell(x, y, 2, 2, "brown")
    p.ell(20, 12, 10, 7, "yellow")                       # head
    p.ell(11, 15, 5, 4, "tan")                           # muzzle
    p.px(9, 14, "dbrown"); p.px(12, 14, "dbrown")
    p.rect(19, 1, 20, 5, "brown"); p.rect(24, 1, 25, 5, "brown")   # ossicones
    p.ell(19, 1, 2, 1, "dbrown"); p.ell(25, 1, 2, 1, "dbrown")
    p.ell(30, 8, 4, 2, "yellow")                         # ear
    p.eye(19, 10)
    p.smile(13, 18, 2)
    p.rect(33, 18, 35, 40, "brown")                      # mane
    return p.outline()


def goo(main, dark, light):
    def f():
        p = Pic()
        p.ell(24, 30, 19, 15, main)                      # the blob
        p.ell(24, 18, 13, 12, main)
        for x, h in ((9, 5), (17, 8), (30, 6), (38, 4)):   # drips
            p.rect(x - 2, 40, x + 2, 40 + h, main); p.ell(x, 40 + h, 2, 2, main)
        p.ell(16, 13, 4, 3, light)                       # shine
        p.ell(13, 15, 1, 1, "white")
        p.eye(18, 23, True); p.eye(30, 23, True)
        p.ell(24, 33, 5, 3, dark); p.rect(20, 30, 28, 31, main)   # big grin
        p.cheek(12, 30); p.cheek(36, 30)
        return p.outline()
    return f


def bluepup():                                            # a blue heeler pup (original design)
    p = Pic()
    p.poly([(7, 2), (18, 14), (5, 20)], "navy")           # ears
    p.poly([(41, 2), (30, 14), (43, 20)], "navy")
    p.poly([(9, 8), (14, 14), (8, 17)], "tan"); p.poly([(39, 8), (34, 14), (40, 17)], "tan")
    p.ell(24, 26, 17, 15, "pupblue")                      # head
    p.ell(24, 14, 5, 3, "navy")                           # a dark patch on top
    p.ell(24, 35, 11, 7, "cream")                         # muzzle
    p.ell(15, 24, 5, 5, "navy"); p.ell(33, 24, 5, 5, "navy")   # eye patches
    p.eye(15, 24, True); p.eye(33, 24, True)
    p.ell(24, 31, 3, 2, "black")                          # nose
    p.smile(24, 36, 4)
    p.ell(26, 40, 2, 2, "hpink")                          # tongue
    return p.outline()


def monster(main, dark, nose, cookie=False):
    def f():
        p = Pic()
        for x, y in ((8, 10), (14, 6), (22, 4), (30, 5), (37, 9), (42, 16), (6, 18)):   # fur tufts
            p.ell(x, y, 4, 4, main)
        p.ell(24, 26, 19, 19, main)
        p.ell(16, 13, 6, 6, "white"); p.ell(31, 13, 6, 6, "white")   # googly eyes on top
        p.ell(17, 14, 2, 3, "black"); p.ell(30, 14, 2, 3, "black")
        if nose:
            p.ell(24, 23, 5, 4, nose)
            p.px(22, 21, "white")
        p.d.pieslice([13, 24, 35, 42], 0, 180, fill=C["black"] + (255,))   # a big happy mouth
        p.ell(24, 38, 6, 3, "red")
        if cookie:
            p.ell(38, 38, 7, 7, "tan")
            for x, y in ((36, 36), (40, 39), (37, 41), (41, 35)):
                p.ell(x, y, 1, 1, "dbrown")
            p.poly([(44, 32), (46, 40), (40, 30)], (main))   # a bite out of it
        return p.outline()
    return f


def doggy():                                              # April: brown and black Aussie / shepherd mix
    p = Pic()
    p.poly([(8, 2), (18, 12), (6, 20)], "dbrown")         # tall ears
    p.poly([(40, 2), (30, 12), (42, 20)], "dbrown")
    p.poly([(10, 7), (15, 12), (9, 16)], "tan"); p.poly([(38, 7), (33, 12), (39, 16)], "tan")
    p.ell(24, 25, 16, 15, "brown")                        # head
    p.ell(24, 17, 10, 8, "black")                         # dark saddle on top
    p.ell(24, 34, 10, 8, "tan")                           # muzzle
    p.ell(24, 30, 4, 3, "black")                          # nose
    p.px(23, 29, "grey")
    p.eye(16, 23, True); p.eye(32, 23, True)
    p.ell(16, 19, 2, 1, "tan"); p.ell(32, 19, 2, 1, "tan")   # eyebrow dots
    p.smile(24, 36, 4)
    p.ell(26, 40, 2, 3, "hpink")
    return p.outline()


def scissors():
    p = Pic()
    p.poly([(22, 26), (44, 4), (46, 6), (26, 28)], "grey")    # blades
    p.poly([(22, 22), (44, 44), (42, 46), (20, 26)], "grey")
    p.line([(26, 25), (44, 6)], "white"); p.line([(25, 23), (43, 43)], "white")
    p.ell(23, 24, 2, 2, "dgrey")
    for cx, cy in ((11, 13), (11, 35)):                       # handles
        p.ell(cx, cy, 9, 8, "hpink")
        p.ell(cx, cy, 5, 4, (None) or "white")
    p.poly([(16, 17), (22, 22), (20, 24), (14, 20)], "hpink")
    p.poly([(16, 31), (22, 26), (20, 24), (14, 28)], "hpink")
    im = p.im
    for cx, cy in ((11, 13), (11, 35)):                       # see-through holes
        for y in range(cy - 4, cy + 5):
            for x in range(cx - 5, cx + 6):
                if ((x - cx) / 5.2) ** 2 + ((y - cy) / 4.2) ** 2 <= 1:
                    im.putpixel((x, y), (0, 0, 0, 0))
    p.ell(30, 36, 1, 1, "white")
    return p.outline()


def ghost():
    p = Pic()
    p.ell(24, 20, 16, 16, "white")
    p.rect(8, 20, 40, 38, "white")
    for i, x in enumerate(range(8, 41, 8)):                   # wavy hem
        p.ell(x + 4, 39, 4, 4 if i % 2 else 3, "white")
    p.ell(10, 28, 4, 3, "white"); p.ell(4, 25, 3, 2, "white")   # waving arm
    p.ell(38, 29, 4, 3, "white")
    p.ell(17, 18, 3, 5, "black"); p.ell(31, 18, 3, 5, "black")
    p.px(16, 15, "white"); p.px(30, 15, "white")
    p.ell(24, 30, 4, 4, "black"); p.ell(24, 31, 2, 2, "red")    # an "Ooo!" mouth
    p.cheek(11, 24); p.cheek(37, 24)
    p.ell(30, 8, 3, 2, "lblue")                                 # a bit of glow
    return p.outline()


def webhero():                                                  # a masked web hero (original design)
    p = Pic()
    p.ell(24, 24, 18, 21, "red")
    for a in range(0, 360, 45):                                 # the web
        import math
        x = 24 + 22 * math.cos(math.radians(a)); y = 24 + 24 * math.sin(math.radians(a))
        p.line([(24, 24), (x, y)], "dred")
    for r in (7, 13, 19):
        p.d.ellipse([24 - r, 24 - r * 1.15, 24 + r, 24 + r * 1.15], outline=C["dred"] + (255,))
    p.poly([(8, 16), (20, 20), (20, 28), (10, 27)], "black")      # big eye shapes
    p.poly([(40, 16), (28, 20), (28, 28), (38, 27)], "black")
    p.poly([(10, 18), (19, 21), (19, 26), (11, 25)], "white")
    p.poly([(38, 18), (29, 21), (29, 26), (37, 25)], "white")
    im = p.im                                                   # clip to the head
    for y in range(N):
        for x in range(N):
            if ((x - 24) / 18.5) ** 2 + ((y - 24) / 21.5) ** 2 > 1:
                im.putpixel((x, y), (0, 0, 0, 0))
    return p.outline()


def butterfly():
    p = Pic()
    p.ell(13, 15, 11, 11, "purple"); p.ell(35, 15, 11, 11, "purple")
    p.ell(14, 34, 9, 8, "hpink"); p.ell(34, 34, 9, 8, "hpink")
    p.ell(12, 14, 5, 5, "lpurple"); p.ell(36, 14, 5, 5, "lpurple")
    p.ell(14, 35, 3, 3, "yellow"); p.ell(34, 35, 3, 3, "yellow")
    p.ell(24, 26, 3, 15, "dbrown")
    p.ell(24, 11, 4, 4, "dbrown")
    p.line([(22, 8), (17, 1)], "dbrown"); p.line([(26, 8), (31, 1)], "dbrown")
    p.ell(17, 1, 1, 1, "hpink"); p.ell(31, 1, 1, 1, "hpink")
    p.px(23, 10, "white"); p.px(26, 10, "white")
    return p.outline()


def kitty():
    p = Pic()
    p.poly([(6, 4), (18, 12), (6, 22)], "orange"); p.poly([(42, 4), (30, 12), (42, 22)], "orange")
    p.poly([(9, 10), (14, 13), (9, 18)], "lpink"); p.poly([(39, 10), (34, 13), (39, 18)], "lpink")
    p.ell(24, 26, 18, 16, "orange")
    for y in (14, 18):                                          # stripes
        p.rect(20, y, 28, y + 1, "gold")
    p.ell(24, 34, 9, 6, "cream")
    p.eye(16, 25, True); p.eye(32, 25, True)
    p.poly([(22, 30), (26, 30), (24, 32)], "hpink")
    p.smile(21, 34, 2); p.smile(27, 34, 2)
    for y in (31, 34):                                          # whiskers
        p.line([(2, y - 2), (12, y)], "dbrown"); p.line([(36, y), (46, y - 2)], "dbrown")
    return p.outline()


def frog():
    p = Pic()
    p.ell(24, 31, 20, 13, "green")
    p.ell(13, 15, 8, 8, "green"); p.ell(35, 15, 8, 8, "green")
    p.eye(13, 14, True); p.eye(35, 14, True)
    p.ell(24, 36, 12, 6, "lime")
    p.line([(10, 28), (24, 33), (38, 28)], "dgreen", 2)
    p.cheek(8, 30); p.cheek(40, 30)
    p.ell(9, 44, 5, 2, "green"); p.ell(39, 44, 5, 2, "green")
    p.ell(20, 22, 1, 1, "dgreen"); p.ell(28, 22, 1, 1, "dgreen")
    return p.outline()


def robot():
    p = Pic()
    p.rect(23, 2, 25, 8, "dgrey"); p.ell(24, 2, 3, 2, "red")
    p.rect(8, 9, 40, 34, "grey")
    p.rect(4, 16, 7, 26, "dgrey"); p.rect(41, 16, 44, 26, "dgrey")
    p.rect(11, 12, 37, 31, "lblue")
    p.ell(17, 19, 4, 4, "white"); p.ell(31, 19, 4, 4, "white")
    p.ell(17, 19, 2, 2, "blue"); p.ell(31, 19, 2, 2, "blue")
    for i, c in enumerate(("red", "yellow", "green", "blue", "purple")):   # a light-up smile
        p.rect(14 + i * 4, 26, 16 + i * 4, 28, c)
    p.rect(14, 36, 34, 46, "grey")
    p.ell(24, 41, 3, 3, "hpink")
    p.rect(16, 34, 32, 35, "dgrey")
    return p.outline()


def rainbow():
    p = Pic()
    for i, c in enumerate(("red", "orange", "yellow", "green", "blue", "purple")):
        r = 23 - i * 3
        p.d.pieslice([24 - r, 30 - r, 24 + r, 30 + r], 180, 360, fill=C[c] + (255,))
    p.ell(24, 30, 4, 4, (0, 0, 0, 0) and "white")
    im = p.im
    for y in range(N):
        for x in range(N):
            if (x - 24) ** 2 + (y - 30) ** 2 <= 36 or y > 30:
                im.putpixel((x, y), (0, 0, 0, 0))
    for cx in (6, 42):                                          # clouds at the ends
        p.ell(cx, 34, 6, 5, "white"); p.ell(cx - 4, 37, 4, 3, "white"); p.ell(cx + 4, 37, 4, 3, "white")
    p.ell(4, 34, 1, 1, "black"); p.ell(8, 34, 1, 1, "black"); p.px(6, 37, "hpink")
    p.ell(40, 34, 1, 1, "black"); p.ell(44, 34, 1, 1, "black"); p.px(42, 37, "hpink")
    return p.outline()


# order = the firmware's (kid.c KID_SOUND)
FRIENDS = [
    ("DUCKY", ducky("yellow", "gold", "orange")),
    ("PINK DUCKY", ducky("pink", "hpink", "orange", bow)),
    ("COOL DUCKY", ducky("lblue", "blue", "orange", shades)),
    ("AXOLOTL", axolotl),
    ("UNICORN", unicorn),
    ("GIRAFFE", giraffe),
    ("GOO", goo("pink", "hpink", "lpink")),
    ("GOOBERT", goo("lime", "dgreen", "lgreen")),
    ("BLUE PUP", bluepup),
    ("RED MONSTER", monster("red", "dred", "orange")),
    ("BLUE MONSTER", monster("blue", "dblue", None, True)),
    ("APRIL", doggy),
    ("SCISSORS", scissors),
    ("GHOST", ghost),
    ("WEB HERO", webhero),
    ("BUTTERFLY", butterfly),
    ("KITTY", kitty),
    ("FROG", frog),
    ("ROBOT", robot),
    ("RAINBOW", rainbow),
]


def rgb565(c):
    r, g, b = c[:3]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def main(out, png=None):
    pics = [(name, f().im) for name, f in FRIENDS]
    hello = re.sub(r"[^A-Z ]", "", os.environ.get("KID_NAME", "").upper()).strip()[:10]
    hello = hello + "!" if hello else ""
    lines = ["/* generated by tools/gen_kid_art.py: Rainbow mode's friends */",
             f"#define KID_N {len(pics)}", f"#define KID_PW {N}",
             "static const char *const KID_NAME[KID_N] = {" + ", ".join(f'"{n}"' for n, _ in pics) + "};",
             f'#define KID_HELLO_NAME "{hello}"', f"#define KID_HELLO_SC {5 if len(hello) <= 8 else 4}",
             "static const uint16_t KID_PAL[KID_N][16] = {"]
    data = []
    for name, im in pics:
        cols = []
        idx = []
        for y in range(N):
            for x in range(N):
                p = im.getpixel((x, y))
                if not p[3]:
                    idx.append(0)
                    continue
                c = p[:3]
                if c not in cols:
                    cols.append(c)
                idx.append(cols.index(c) + 1)
        if len(cols) > 15:
            raise SystemExit(f"{name}: {len(cols)} colours, 15 at most")
        pal = [0] + [rgb565(c) for c in cols] + [0] * (15 - len(cols))
        lines.append("    {" + ", ".join(f"0x{v:04X}" for v in pal) + "},")
        data.append(bytes((idx[i] << 4) | idx[i + 1] for i in range(0, len(idx), 2)))
    lines.append("};")
    lines.append(f"static const uint8_t KID_PIX[KID_N][{N * N // 2}] = {{")
    for d in data:
        lines.append("    {" + ",".join(str(b) for b in d) + "},")
    lines.append("};")
    open(out, "w").write("\n".join(lines) + "\n")
    print(f"kid art: {len(pics)} friends, {sum(len(d) for d in data)} B of pixels")
    if png:
        cols = 5
        sheet = Image.new("RGB", (cols * (N * 4 + 8), ((len(pics) + cols - 1) // cols) * (N * 4 + 8)), (120, 200, 255))
        for i, (_, im) in enumerate(pics):
            big = im.resize((N * 4, N * 4), Image.NEAREST)
            sheet.paste(big, ((i % cols) * (N * 4 + 8) + 4, (i // cols) * (N * 4 + 8) + 4), big)
        sheet.save(png)


if __name__ == "__main__":
    a = sys.argv[1:]
    png = a[a.index("--png") + 1] if "--png" in a else None
    main(a[0], png)
