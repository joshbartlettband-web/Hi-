#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rainbow mode's friends: 20 pixel-art pictures (run by tools/build.py generate()).

  gen_kid_art.py OUT.h [--png SHEET.png]

Each friend is drawn from shapes (no antialiasing) on a roomy canvas, then outlined (every empty pixel that
touches the picture becomes the outline colour), trimmed, centred across a 48 x 48 grid and stood on its
bottom row. A picture that would not fit stops the build: nothing is ever cut off. At most 15 colours per picture (index 0 = see-through); the
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

N = 48                           # a picture: N x N, shapes at most N - 2 across (the outline round them)
W = 72                           # the drawing canvas: room on every side, so nothing is cut while drawing
OFF = 12                         # design coordinates (about 0 .. 47) sit this far into the canvas
OUT = (52, 26, 58)               # the outline: a dark plum, softer than black
C = dict(
    white=(255, 255, 255), black=(28, 20, 36), cream=(255, 244, 214), grey=(176, 180, 200), dgrey=(96, 98, 120),
    yellow=(255, 216, 46), dyellow=(236, 166, 24), gold=(255, 178, 30), orange=(255, 132, 36), dorange=(214, 88, 24),
    red=(236, 52, 64), dred=(170, 24, 48), pink=(255, 138, 196), dpink=(222, 92, 162), lpink=(255, 200, 226),
    hpink=(236, 64, 150), blue=(56, 128, 244), dblue=(30, 70, 170), lblue=(150, 206, 255), mblue=(96, 160, 236),
    navy=(30, 36, 90), teal=(36, 196, 190), green=(70, 200, 90), dgreen=(36, 130, 64), lime=(176, 232, 70),
    dlime=(112, 180, 40), lgreen=(214, 252, 170), purple=(150, 88, 226), dpurple=(104, 56, 180),
    lpurple=(204, 164, 255), brown=(150, 92, 46), dbrown=(84, 50, 32), tan=(226, 176, 112), dtan=(186, 128, 70),
    silver=(214, 220, 236), dwhite=(204, 212, 238), sky=(150, 206, 255),
)


class Pic:
    """draw in design coordinates (0 .. 47, a little past is fine); done() outlines, trims and places it"""

    def __init__(self):
        self.im = Image.new("RGBA", (W, W), (0, 0, 0, 0))
        self.d = ImageDraw.Draw(self.im)

    def _c(self, c):
        return C[c] + (255,) if c else (0, 0, 0, 0)

    def ell(self, cx, cy, rx, ry, c):
        self.d.ellipse([cx - rx + OFF, cy - ry + OFF, cx + rx + OFF, cy + ry + OFF], fill=self._c(c))

    def ball(self, cx, cy, rx, ry, c, dark):      # a shaded round shape: the dark side down and to the right
        self.ell(cx, cy, rx, ry, dark)
        self.ell(cx - 1, cy - 1, rx - 1, ry - 1, c)

    def rect(self, x0, y0, x1, y1, c):
        self.d.rectangle([x0 + OFF, y0 + OFF, x1 + OFF, y1 + OFF], fill=self._c(c))

    def poly(self, pts, c):
        self.d.polygon([(x + OFF, y + OFF) for x, y in pts], fill=self._c(c))

    def line(self, pts, c, w=2):
        self.d.line([(x + OFF, y + OFF) for x, y in pts], fill=self._c(c), width=w)

    def pie(self, cx, cy, r, a0, a1, c):
        self.d.pieslice([cx - r + OFF, cy - r + OFF, cx + r + OFF, cy + r + OFF], a0, a1, fill=self._c(c))

    def px(self, x, y, c):
        self.im.putpixel((x + OFF, y + OFF), self._c(c))

    def eye(self, x, y, r=4):                     # a big shiny eye: white, a dark pupil low, a white spark
        self.ell(x, y, r, r + 1, "white")
        self.ell(x, y + 1, r - 1, r, "black")
        self.rect(x - 1, y - 1, x, y, "white")

    def dot_eye(self, x, y):                      # a small solid eye (ducks, a robot's lamp), with a spark
        self.ell(x, y, 2, 3, "black")
        self.px(x - 1, y - 1, "white")

    def smile(self, x, y, w=3, c="black"):
        self.line([(x - w, y), (x - w + 1, y + 1), (x + w - 1, y + 1), (x + w, y)], c, 1)
        self.line([(x - w + 1, y + 1), (x + w - 1, y + 1)], c, 1)

    def cheek(self, x, y):
        self.ell(x, y, 2, 1, "pink")

    def done(self):
        """outline, trim to the outline, then centre it across and stand it on the bottom row"""
        src = self.im.copy()
        for y in range(W):
            for x in range(W):
                if src.getpixel((x, y))[3]:
                    continue
                for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                    xx, yy = x + dx, y + dy
                    if 0 <= xx < W and 0 <= yy < W and src.getpixel((xx, yy))[3]:
                        self.im.putpixel((x, y), OUT + (255,))
                        break
        x0, y0, x1, y1 = self.im.getbbox()
        w, h = x1 - x0, y1 - y0
        if w > N or h > N:
            raise SystemExit(f"a picture is {w} x {h}: {N} x {N} at most (it would be cut)")
        if min(x0, y0) == 0 or x1 == W or y1 == W:
            raise SystemExit("a picture reaches the edge of the drawing canvas (it would be cut)")
        out = Image.new("RGBA", (N, N), (0, 0, 0, 0))
        out.paste(self.im.crop((x0, y0, x1, y1)), ((N - w) // 2, N - h))
        self.im = out
        return self


# ---------------------------------------------------------------- the friends
SHADE = dict(yellow="dyellow", pink="dpink", lblue="mblue", white="dwhite", lime="dlime", red="dred",
             blue="dblue", orange="dorange", green="dgreen", purple="dpurple", tan="dtan", brown="dbrown")


def ducky(body, wing, extra=None):
    def f():
        p = Pic()
        p.poly([(38, 30), (45, 20), (43, 36)], SHADE[body])          # tail, up at the back
        p.ball(26, 37, 17, 10, body, SHADE[body])                   # body
        p.ball(19, 19, 10, 10, body, SHADE[body])                   # head
        p.ell(7, 22, 6, 3, "orange")                                # beak
        p.rect(3, 23, 10, 24, "dorange")
        p.ell(28, 34, 9, 5, wing)                                   # wing
        p.ell(27, 32, 8, 3, body)
        p.dot_eye(17, 17)
        p.cheek(22, 23)
        if extra:
            extra(p)
        return p.done()
    return f


def bow(p):
    p.poly([(12, 7), (18, 10), (12, 13)], "hpink"); p.poly([(24, 7), (18, 10), (24, 13)], "hpink")
    p.ell(18, 10, 2, 2, "red")


def shades(p):
    p.rect(11, 14, 25, 15, "black")
    p.ell(15, 17, 4, 3, "black"); p.ell(23, 17, 3, 3, "black")
    p.px(13, 16, "lblue"); p.px(21, 16, "lblue")
    p.ell(19, 9, 8, 4, "teal"); p.rect(11, 9, 28, 11, "teal")    # a cap, its peak forward
    p.rect(5, 10, 12, 11, "teal")


def axolotl():
    p = Pic()
    p.poly([(34, 32), (45, 25), (45, 43)], "lpink")              # tail fin
    p.ball(29, 35, 12, 7, "pink", "dpink")                       # body
    p.ell(22, 43, 3, 3, "dpink"); p.ell(35, 43, 3, 3, "dpink")   # legs
    for (ax, ay), (tx, ty) in (((10, 14), (4, 8)), ((8, 19), (2, 17)), ((10, 24), (4, 27)),
                               ((30, 14), (36, 8)), ((32, 19), (38, 17)), ((30, 24), (36, 27))):   # frilly gills
        p.line([(ax, ay), (tx, ty)], "hpink", 3)
        p.ell(tx, ty, 2, 2, "hpink")
    p.ball(20, 19, 13, 11, "pink", "dpink")                      # head
    p.eye(14, 17, 3); p.eye(26, 17, 3)
    p.smile(20, 24, 4)
    p.cheek(10, 23); p.cheek(30, 23)
    return p.done()


def unicorn():
    p = Pic()
    p.poly([(14, 46), (16, 30), (32, 30), (36, 46)], "dwhite")   # neck
    p.poly([(16, 46), (17, 31), (30, 31), (33, 46)], "white")
    p.ball(23, 24, 11, 10, "white", "dwhite")                    # head
    p.ball(12, 31, 8, 6, "white", "dwhite")                      # snout
    p.ell(8, 33, 4, 3, "lpink")
    p.rect(6, 32, 7, 33, "dpink"); p.rect(10, 32, 11, 33, "dpink")
    p.poly([(20, 15), (26, 1), (29, 16)], "gold")                # horn, with two stripes
    p.line([(21, 12), (27, 10)], "yellow"); p.line([(23, 7), (27, 6)], "yellow")
    p.poly([(27, 16), (31, 9), (34, 19)], "white")               # ear
    for i, c in enumerate(("red", "orange", "yellow", "green", "blue", "purple")):   # rainbow mane
        p.ell(34 + i // 2, 18 + i * 4, 5, 3, c)
    p.eye(20, 22, 3)
    p.cheek(17, 28)
    return p.done()


def giraffe():
    p = Pic()
    p.rect(25, 16, 34, 44, "dyellow")                            # neck
    p.rect(25, 16, 32, 44, "yellow")
    for x, y in ((28, 22), (31, 29), (27, 35), (31, 41)):        # big spots
        p.ell(x, y, 3, 2, "brown")
    p.rect(34, 18, 37, 42, "brown")                              # mane
    p.ball(21, 14, 11, 8, "yellow", "dyellow")                   # head
    p.ell(11, 17, 6, 5, "tan")                                   # muzzle
    p.rect(8, 15, 9, 16, "dbrown"); p.rect(12, 15, 13, 16, "dbrown")
    p.rect(18, 3, 20, 8, "brown"); p.rect(25, 3, 27, 8, "brown")  # ossicones, thick
    p.ell(19, 3, 2, 2, "dbrown"); p.ell(26, 3, 2, 2, "dbrown")
    p.ell(32, 9, 4, 2, "yellow")                                 # ear
    p.eye(21, 12, 3)
    p.smile(13, 20, 2)
    return p.done()


def goo(main, dark, light):
    def f():
        p = Pic()
        for x, h in ((9, 5), (17, 7), (31, 6), (39, 4)):        # drips
            p.rect(x - 2, 38, x + 2, 38 + h, main); p.ell(x, 38 + h, 2, 2, main)
        p.ball(24, 30, 19, 13, main, dark)                       # the blob
        p.ell(24, 20, 13, 11, main)
        p.ell(16, 14, 4, 3, light)                               # shine
        p.ell(13, 16, 1, 1, "white")
        p.eye(18, 24, 4); p.eye(30, 24, 4)
        p.pie(24, 30, 6, 0, 180, "black")                       # a big grin
        p.ell(24, 34, 3, 1, "hpink")
        p.cheek(11, 30); p.cheek(37, 30)
        return p.done()
    return f


def bluepup():                                                   # a blue heeler pup (original design)
    p = Pic()
    p.poly([(7, 2), (18, 14), (5, 20)], "navy")                  # ears
    p.poly([(41, 2), (30, 14), (43, 20)], "navy")
    p.poly([(9, 8), (14, 14), (8, 17)], "tan"); p.poly([(39, 8), (34, 14), (40, 17)], "tan")
    p.ball(24, 27, 17, 15, "mblue", "blue")                      # head
    p.ell(24, 14, 5, 3, "navy")                                  # a dark patch on top
    p.ell(15, 25, 6, 6, "navy"); p.ell(33, 25, 6, 6, "navy")     # eye patches
    p.ell(24, 36, 11, 7, "cream")                                # muzzle
    p.eye(15, 25, 4); p.eye(33, 25, 4)
    p.ell(24, 32, 3, 2, "black")                                 # nose
    p.smile(24, 37, 4)
    p.ell(26, 41, 2, 2, "hpink")                                 # tongue
    return p.done()


def monster(main, dark, nose, cookie=False):
    def f():
        p = Pic()
        for x, y in ((9, 10), (15, 6), (23, 4), (31, 5), (38, 9), (42, 16), (6, 18)):   # fur tufts
            p.ell(x, y, 4, 4, main)
        p.ball(24, 26, 19, 19, main, dark)
        p.ell(16, 14, 6, 6, "white"); p.ell(31, 14, 6, 6, "white")   # googly eyes on top
        p.ell(17, 15, 3, 3, "black"); p.ell(30, 15, 3, 3, "black")
        p.rect(15, 13, 16, 14, "white"); p.rect(28, 13, 29, 14, "white")
        if nose:
            p.ell(24, 24, 5, 4, nose)
            p.px(22, 22, "white")
        p.pie(24, 29, 11, 0, 180, "black")                       # a big happy mouth
        p.ell(24, 37, 6, 2, "red")
        if cookie:
            p.ball(38, 37, 7, 7, "tan", "dtan")
            for x, y in ((35, 35), (39, 38), (36, 40), (41, 34)):
                p.ell(x, y, 1, 1, "dbrown")
        return p.done()
    return f


def doggy():                                                     # April: brown and black Aussie / shepherd mix
    p = Pic()
    p.poly([(8, 2), (18, 12), (6, 20)], "dbrown")                # tall ears
    p.poly([(40, 2), (30, 12), (42, 20)], "dbrown")
    p.poly([(10, 7), (15, 12), (9, 16)], "tan"); p.poly([(38, 7), (33, 12), (39, 16)], "tan")
    p.ball(24, 26, 16, 15, "brown", "dbrown")                    # head
    p.ell(24, 17, 10, 8, "black")                                # dark saddle on top
    p.ell(24, 35, 10, 8, "tan")                                  # muzzle
    p.eye(16, 24, 4); p.eye(32, 24, 4)
    p.ell(16, 18, 2, 1, "tan"); p.ell(32, 18, 2, 1, "tan")       # eyebrow dots
    p.ell(24, 31, 4, 3, "black")                                 # nose
    p.px(23, 30, "grey")
    p.smile(24, 37, 4)
    p.ell(26, 41, 2, 3, "hpink")
    return p.done()


def scissors():
    p = Pic()
    p.poly([(21, 27), (43, 4), (45, 7), (25, 29)], "dwhite")      # blades, thick
    p.poly([(21, 21), (43, 44), (41, 46), (19, 25)], "dwhite")
    p.line([(25, 26), (43, 6)], "silver"); p.line([(24, 22), (42, 42)], "silver")
    p.ell(22, 24, 2, 2, "dgrey")
    for cx, cy in ((11, 12), (11, 36)):                           # handles, chunky rings
        p.ell(cx, cy, 9, 9, "hpink")
        p.ell(cx, cy, 4, 4, None)
    p.poly([(16, 16), (23, 22), (21, 25), (14, 20)], "hpink")
    p.poly([(16, 32), (23, 26), (21, 23), (14, 28)], "hpink")
    return p.done()


def ghost():
    p = Pic()
    p.ball(24, 20, 16, 16, "white", "dwhite")
    p.rect(8, 20, 40, 38, "white"); p.rect(38, 20, 40, 38, "dwhite")
    for i, x in enumerate(range(8, 41, 8)):                      # wavy hem
        p.ell(x + 4, 39, 4, 4 if i % 2 else 3, "white")
    p.ell(6, 26, 3, 3, "white")                                  # waving arms
    p.ell(42, 28, 3, 3, "white")
    p.ell(17, 18, 3, 5, "black"); p.ell(31, 18, 3, 5, "black")
    p.rect(16, 15, 17, 16, "white"); p.rect(30, 15, 31, 16, "white")
    p.ell(24, 30, 4, 4, "black"); p.ell(24, 31, 2, 2, "red")    # an "Ooo!" mouth
    p.cheek(11, 25); p.cheek(37, 25)
    return p.done()


def webhero():                                                   # a masked web hero (original design)
    import math
    p = Pic()
    p.ball(24, 24, 18, 21, "red", "dred")
    for a in range(0, 360, 45):                                  # the web: a few strong lines
        x = 24 + 22 * math.cos(math.radians(a)); y = 24 + 24 * math.sin(math.radians(a))
        p.line([(24, 24), (x, y)], "dred", 1)
    for r in (8, 15):
        p.d.ellipse([24 - r + OFF, 24 - r * 1.15 + OFF, 24 + r + OFF, 24 + r * 1.15 + OFF], outline=C["dred"] + (255,))
    p.poly([(7, 15), (21, 20), (21, 29), (9, 27)], "black")      # big eye shapes
    p.poly([(41, 15), (27, 20), (27, 29), (39, 27)], "black")
    p.poly([(10, 18), (19, 22), (19, 27), (11, 25)], "white")
    p.poly([(38, 18), (29, 22), (29, 27), (37, 25)], "white")
    im = p.im                                                    # keep only the head
    for y in range(W):
        for x in range(W):
            if ((x - 24 - OFF) / 18.5) ** 2 + ((y - 24 - OFF) / 21.5) ** 2 > 1:
                im.putpixel((x, y), (0, 0, 0, 0))
    return p.done()


def butterfly():
    p = Pic()
    p.ball(13, 15, 11, 11, "purple", "dpurple"); p.ball(36, 15, 11, 11, "purple", "dpurple")
    p.ball(14, 34, 9, 8, "hpink", "dpink"); p.ball(35, 34, 9, 8, "hpink", "dpink")
    p.ell(12, 14, 5, 5, "lpurple"); p.ell(36, 14, 5, 5, "lpurple")
    p.ell(14, 35, 3, 3, "yellow"); p.ell(35, 35, 3, 3, "yellow")
    p.ell(24, 26, 3, 15, "dbrown")
    p.ell(24, 11, 4, 4, "dbrown")
    p.line([(22, 8), (17, 1)], "dbrown"); p.line([(26, 8), (31, 1)], "dbrown")
    p.ell(17, 1, 2, 2, "hpink"); p.ell(31, 1, 2, 2, "hpink")
    p.px(23, 10, "white"); p.px(26, 10, "white")
    return p.done()


def kitty():
    p = Pic()
    p.poly([(6, 3), (18, 12), (6, 22)], "orange"); p.poly([(42, 3), (30, 12), (42, 22)], "orange")
    p.poly([(9, 9), (14, 13), (9, 18)], "lpink"); p.poly([(39, 9), (34, 13), (39, 18)], "lpink")
    p.ball(24, 27, 18, 16, "orange", "dorange")
    for y in (14, 18):                                           # stripes
        p.rect(19, y, 29, y + 1, "dorange")
    p.ell(24, 35, 9, 6, "cream")
    p.eye(16, 26, 4); p.eye(32, 26, 4)
    p.poly([(21, 31), (27, 31), (24, 34)], "hpink")
    p.smile(24, 36, 3)
    for y in (32, 36):                                           # whiskers, 2 px
        p.line([(2, y - 2), (10, y)], "dbrown"); p.line([(38, y), (46, y - 2)], "dbrown")
    return p.done()


def frog():
    p = Pic()
    p.ball(24, 32, 20, 13, "green", "dgreen")
    p.ball(13, 16, 8, 8, "green", "dgreen"); p.ball(35, 16, 8, 8, "green", "dgreen")
    p.eye(12, 15, 4); p.eye(34, 15, 4)
    p.ell(24, 37, 12, 6, "lime")
    p.line([(10, 29), (24, 34), (38, 29)], "dgreen", 2)
    p.cheek(8, 31); p.cheek(40, 31)
    p.ell(9, 45, 5, 2, "dgreen"); p.ell(39, 45, 5, 2, "dgreen")
    return p.done()


def robot():
    p = Pic()
    p.rect(23, 3, 25, 8, "dgrey"); p.ell(24, 3, 3, 2, "red")
    p.rect(8, 9, 40, 34, "dwhite"); p.rect(8, 9, 38, 32, "silver")
    p.rect(3, 16, 7, 26, "dgrey"); p.rect(41, 16, 45, 26, "dgrey")
    p.rect(11, 12, 37, 30, "lblue")
    p.ell(17, 19, 4, 4, "white"); p.ell(31, 19, 4, 4, "white")
    p.ell(17, 20, 2, 2, "blue"); p.ell(31, 20, 2, 2, "blue")
    for i, c in enumerate(("red", "yellow", "green", "blue", "purple")):   # a light-up smile
        p.rect(14 + i * 4, 25, 16 + i * 4, 27, c)
    p.rect(14, 36, 34, 45, "silver")
    p.ell(24, 40, 3, 3, "hpink")
    p.rect(16, 34, 32, 35, "dgrey")
    return p.done()


def rainbow():
    p = Pic()
    for i, c in enumerate(("red", "orange", "yellow", "green", "blue", "purple")):
        p.pie(24, 32, 21 - i * 3, 180, 360, c)
    p.pie(24, 32, 5, 180, 360, None)                             # the arch's hole
    p.rect(-2, 33, 50, 40, None)
    for cx in (9, 39):                                           # clouds at the ends, with faces
        p.ball(cx, 34, 6, 5, "white", "dwhite"); p.ell(cx - 4, 37, 3, 3, "white"); p.ell(cx + 4, 37, 3, 3, "white")
        p.ell(cx - 2, 34, 1, 1, "black"); p.ell(cx + 2, 34, 1, 1, "black"); p.px(cx, 37, "hpink")
    return p.done()


# order = the firmware's (kid.c KID_SOUND)
FRIENDS = [
    ("DUCKY", ducky("yellow", "dyellow")),
    ("PINK DUCKY", ducky("pink", "dpink", bow)),
    ("COOL DUCKY", ducky("lblue", "mblue", shades)),
    ("AXOLOTL", axolotl),
    ("UNICORN", unicorn),
    ("GIRAFFE", giraffe),
    ("GOO", goo("pink", "dpink", "lpink")),
    ("GOOBERT", goo("lime", "dlime", "lgreen")),
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
    pics = []
    for name, f in FRIENDS:
        try:
            pics.append((name, f().im))
        except SystemExit as e:
            raise SystemExit(f"{name}: {e}")
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
            x, y = (i % cols) * (N * 4 + 8) + 4, (i // cols) * (N * 4 + 8) + 4
            ImageDraw.Draw(sheet).rectangle([x, y, x + N * 4 - 1, y + N * 4 - 1], outline=(90, 160, 220))
            sheet.paste(big, (x, y), big)
        sheet.save(png)


if __name__ == "__main__":
    a = sys.argv[1:]
    png = a[a.index("--png") + 1] if "--png" in a else None
    main(a[0], png)
