#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Rainbow mode's friends: 40 pixel-art pictures (run by tools/build.py generate()).

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
    silver=(214, 220, 236), dwhite=(204, 212, 238), sky=(150, 206, 255), glass=(212, 236, 255),
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
SHADE = dict(lpurple="purple", yellow="dyellow", pink="dpink", lblue="mblue", white="dwhite", lime="dlime", red="dred",
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


def crown(p):
    p.poly([(11, 12), (13, 6), (16, 9), (19, 4), (22, 9), (25, 6), (27, 12)], "gold")
    p.rect(11, 11, 27, 13, "gold")
    p.ell(19, 9, 1, 1, "red"); p.ell(14, 11, 1, 1, "lblue"); p.ell(24, 11, 1, 1, "lblue")


def redpup():                                                    # a red heeler pup (original design)
    p = Pic()
    p.poly([(7, 2), (18, 14), (5, 20)], "dorange")               # ears
    p.poly([(41, 2), (30, 14), (43, 20)], "dorange")
    p.poly([(9, 8), (14, 14), (8, 17)], "cream"); p.poly([(39, 8), (34, 14), (40, 17)], "cream")
    p.ball(24, 27, 17, 15, "orange", "dorange")                  # head
    p.ell(24, 14, 5, 3, "dorange")
    p.ell(15, 25, 6, 6, "dorange"); p.ell(33, 25, 6, 6, "dorange")
    p.ell(24, 36, 11, 7, "cream")
    p.eye(15, 25, 4); p.eye(33, 25, 4)
    p.ell(24, 32, 3, 2, "black")
    p.smile(24, 37, 4)
    p.ell(26, 41, 2, 2, "hpink")
    return p.done()


def yellowbird():                                                # a big yellow bird (original design)
    p = Pic()
    for i in range(7):                                           # feathery body
        p.ell(10 + i * 5, 34 + (i % 2) * 2, 6, 9, "dyellow" if i % 2 else "yellow")
    p.ball(24, 18, 13, 13, "yellow", "dyellow")                  # head
    for x in (16, 22, 28):                                       # a tuft of feathers on top
        p.ell(x, 5, 2, 4, "yellow")
    p.eye(19, 16, 4); p.eye(29, 16, 4)
    p.poly([(16, 23), (33, 23), (24, 31)], "orange")             # a big beak
    p.rect(17, 23, 32, 24, "dorange")
    return p.done()


def slimy():                                                     # a green slime ghost (original design)
    p = Pic()
    p.ball(24, 22, 17, 16, "lime", "dlime")
    p.poly([(8, 24), (40, 24), (36, 44), (24, 38), (12, 44)], "lime")   # a wispy tail
    p.ell(6, 30, 4, 3, "lime"); p.ell(42, 30, 4, 3, "lime")      # little arms
    p.eye(17, 18, 4); p.eye(31, 18, 4)
    p.pie(24, 26, 9, 0, 180, "black")                            # a goofy grin, tongue out
    p.ell(27, 33, 3, 3, "hpink")
    p.ell(13, 10, 3, 2, "lgreen")
    return p.done()


def bunny():
    p = Pic()
    p.ell(16, 10, 4, 11, "white"); p.ell(32, 10, 4, 11, "white")  # long ears
    p.ell(16, 11, 2, 8, "lpink"); p.ell(32, 11, 2, 8, "lpink")
    p.ball(24, 30, 16, 14, "white", "dwhite")
    p.eye(17, 28, 4); p.eye(31, 28, 4)
    p.poly([(21, 34), (27, 34), (24, 37)], "hpink")
    p.smile(24, 39, 3)
    p.rect(22, 40, 26, 43, "white")                              # two front teeth
    p.line([(24, 40), (24, 43)], "dwhite", 1)
    p.cheek(11, 35); p.cheek(37, 35)
    return p.done()


def panda():
    p = Pic()
    p.ell(9, 10, 6, 6, "black"); p.ell(39, 10, 6, 6, "black")    # ears
    p.ball(24, 27, 18, 16, "white", "dwhite")
    p.ell(15, 25, 6, 7, "black"); p.ell(33, 25, 6, 7, "black")   # eye patches
    p.eye(15, 25, 3); p.eye(33, 25, 3)
    p.ell(24, 33, 4, 3, "black")
    p.smile(24, 37, 3)
    p.cheek(9, 33); p.cheek(39, 33)
    return p.done()


def penguin():
    p = Pic()
    p.ball(24, 26, 16, 19, "navy", "black")                      # body
    p.ell(24, 30, 11, 14, "white")                               # white tummy
    p.ell(24, 17, 12, 9, "white")                                # face
    p.ell(7, 30, 4, 9, "navy"); p.ell(41, 30, 4, 9, "navy")      # flippers
    p.eye(19, 16, 3); p.eye(29, 16, 3)
    p.poly([(20, 21), (28, 21), (24, 26)], "orange")
    p.ell(17, 45, 5, 2, "orange"); p.ell(31, 45, 5, 2, "orange")  # feet
    p.cheek(14, 21); p.cheek(34, 21)
    return p.done()


def owl():
    p = Pic()
    p.poly([(8, 4), (16, 12), (8, 16)], "brown"); p.poly([(40, 4), (32, 12), (40, 16)], "brown")   # ear tufts
    p.ball(24, 26, 17, 19, "brown", "dbrown")
    p.ell(24, 33, 11, 11, "tan")                                 # tummy
    for x, y in ((19, 30), (29, 30), (24, 36), (19, 41), (29, 41)):
        p.poly([(x - 2, y), (x + 2, y), (x, y + 2)], "dtan")     # feathers
    p.ell(16, 18, 7, 7, "cream"); p.ell(32, 18, 7, 7, "cream")   # big eye rings
    p.eye(16, 18, 4); p.eye(32, 18, 4)
    p.poly([(22, 22), (26, 22), (24, 27)], "orange")
    p.ell(6, 30, 3, 9, "dbrown"); p.ell(42, 30, 3, 9, "dbrown")  # wings
    return p.done()


def bee():
    p = Pic()
    p.ell(14, 10, 8, 7, "lblue"); p.ell(32, 10, 8, 7, "lblue")   # wings
    p.ell(14, 10, 5, 4, "white"); p.ell(32, 10, 5, 4, "white")
    p.ball(24, 28, 19, 15, "yellow", "dyellow")                  # round body
    for x in (14, 24, 34):                                       # stripes
        p.rect(x - 2, 15, x + 2, 42, "black")
    p.ball(13, 28, 9, 9, "yellow", "dyellow")                    # face
    p.eye(10, 26, 3); p.eye(17, 26, 3)
    p.smile(13, 32, 2)
    p.cheek(7, 31)
    p.poly([(42, 26), (47, 28), (42, 30)], "black")              # stinger
    return p.done()


def ladybug():
    p = Pic()
    p.ball(24, 29, 19, 15, "red", "dred")                        # shell
    p.rect(23, 14, 25, 44, "black")                              # the line down the back
    for x, y in ((14, 24), (34, 24), (12, 35), (36, 35), (19, 41), (29, 41)):
        p.ell(x, y, 3, 3, "black")
    p.ball(24, 11, 11, 8, "black", "black")                      # head
    p.eye(19, 11, 3); p.eye(29, 11, 3)
    p.line([(18, 6), (14, 3)], "black"); p.line([(30, 6), (34, 3)], "black")   # antennae
    p.ell(14, 3, 2, 2, "black"); p.ell(34, 3, 2, 2, "black")
    return p.done()


def dino():
    p = Pic()
    p.poly([(36, 34), (47, 44), (34, 44)], "dgreen")             # tail
    for x, y in ((14, 6), (22, 3), (30, 6), (36, 12)):           # spikes on top
        p.poly([(x - 3, y + 6), (x, y), (x + 3, y + 6)], "orange")
    p.ball(24, 32, 15, 13, "green", "dgreen")                    # body
    p.ball(18, 18, 14, 11, "green", "dgreen")                    # big head
    p.ell(24, 36, 8, 7, "lime")                                  # tummy
    p.eye(19, 15, 4)
    p.pie(14, 22, 8, 0, 180, "black")                            # a toothy grin
    p.poly([(9, 22), (11, 25), (13, 22)], "white"); p.poly([(15, 22), (17, 25), (19, 22)], "white")
    p.ell(14, 45, 4, 2, "dgreen"); p.ell(30, 45, 4, 2, "dgreen")
    return p.done()


def whale():
    p = Pic()
    p.poly([(38, 26), (47, 16), (47, 34)], "blue")               # tail
    p.ball(22, 30, 20, 13, "blue", "dblue")
    p.ell(20, 36, 15, 6, "lblue")                                # belly
    p.ell(14, 26, 3, 4, "white"); p.ell(14, 27, 2, 3, "black")   # eye
    p.px(13, 25, "white")
    p.smile(10, 33, 4)
    p.cheek(18, 31)
    p.line([(22, 16), (22, 9)], "lblue", 3)                      # a spout of water
    p.ell(18, 7, 3, 3, "lblue"); p.ell(26, 7, 3, 3, "lblue"); p.ell(22, 5, 3, 3, "lblue")
    return p.done()


def octopus():
    p = Pic()
    for i, x in enumerate((6, 13, 20, 28, 35, 42)):              # legs, curling
        p.ell(x, 38, 3, 8, "purple"); p.ell(x + (2 if i < 3 else -2), 45, 3, 2, "purple")
    p.ball(24, 20, 17, 17, "purple", "dpurple")
    p.ell(17, 11, 4, 3, "lpurple")
    p.eye(17, 21, 4); p.eye(31, 21, 4)
    p.smile(24, 29, 4)
    p.cheek(11, 27); p.cheek(37, 27)
    return p.done()


def fish():
    p = Pic()
    p.poly([(34, 24), (47, 12), (47, 38)], "orange")             # tail
    p.ball(21, 25, 19, 14, "orange", "dorange")
    p.rect(14, 12, 18, 38, "white"); p.rect(26, 13, 30, 37, "white")   # clownfish stripes
    p.poly([(16, 11), (26, 3), (30, 12)], "dorange")             # fin
    p.eye(9, 22, 4)
    p.smile(6, 30, 2)
    p.cheek(13, 29)
    return p.done()


def turtle():
    p = Pic()
    p.ball(8, 28, 8, 7, "lime", "dlime")                         # head
    p.eye(7, 26, 3)
    p.smile(6, 32, 2)
    p.ell(14, 41, 4, 4, "lime"); p.ell(34, 41, 4, 4, "lime")     # legs
    p.ball(25, 30, 18, 13, "green", "dgreen")                    # shell
    for x, y in ((18, 26), (30, 26), (24, 34), (14, 34), (36, 34)):
        p.ell(x, y, 4, 3, "dgreen")
    p.rect(8, 38, 43, 40, "dlime")                               # its rim
    return p.done()


def icecream():
    p = Pic()
    p.poly([(12, 26), (36, 26), (24, 45)], "tan")                # cone
    for y in (31, 36, 41):                                       # its waffle lines, inside the cone
        half = (45 - y) * 12 // 19
        p.rect(24 - half + 1, y, 24 + half - 1, y, "dtan")
    p.ball(24, 24, 13, 7, "lpink", "pink")                       # a scoop of strawberry
    p.ball(24, 14, 11, 9, "cream", "tan")                        # and one of vanilla
    for x, y in ((18, 12), (26, 9), (30, 16), (20, 20)):         # sprinkles
        p.rect(x, y, x + 2, y, "hpink")
    p.ell(24, 5, 3, 3, "red")                                    # a cherry
    p.eye(20, 15, 3); p.eye(28, 15, 3)
    p.smile(24, 21, 2)
    return p.done()


def cupcake():
    p = Pic()
    p.poly([(9, 28), (39, 28), (35, 46), (13, 46)], "lblue")     # the cup
    for x in (14, 20, 26, 32):
        p.rect(x, 29, x + 1, 45, "mblue")
    p.ball(24, 24, 17, 8, "pink", "dpink")                       # frosting
    p.ball(24, 15, 12, 8, "pink", "dpink")
    p.ell(24, 6, 4, 4, "red")                                    # a cherry
    for x, y in ((14, 22), (30, 20), (22, 13), (34, 26)):
        p.rect(x, y, x + 2, y, "yellow")
    p.eye(19, 36, 3); p.eye(29, 36, 3)
    p.smile(24, 42, 2)
    return p.done()


def rocket():
    p = Pic()
    p.poly([(14, 34), (6, 44), (14, 44)], "red"); p.poly([(34, 34), (42, 44), (34, 44)], "red")   # fins
    p.poly([(24, 2), (33, 12), (15, 12)], "red")                 # nose
    p.ball(24, 27, 10, 17, "white", "dwhite")
    p.rect(14, 12, 34, 40, "white"); p.rect(32, 12, 34, 40, "dwhite")
    p.ell(24, 22, 6, 6, "dblue"); p.ell(24, 22, 4, 4, "lblue")   # a round window
    p.px(22, 20, "white")
    p.poly([(18, 41), (30, 41), (24, 45)], "orange")             # flame
    p.poly([(21, 41), (27, 41), (24, 44)], "yellow")
    return p.done()


def strawberry():
    p = Pic()
    p.poly([(4, 14), (44, 14), (24, 44)], "red")
    p.ball(24, 20, 20, 12, "red", "dred")
    for x, y in ((12, 20), (20, 26), (30, 25), (36, 18), (24, 34), (16, 30), (31, 33)):   # seeds
        p.rect(x, y, x + 1, y + 1, "yellow")
    p.poly([(12, 9), (24, 4), (36, 9), (30, 13), (24, 10), (18, 13)], "green")   # leaves
    p.rect(23, 2, 25, 7, "dgreen")
    p.eye(18, 22, 3); p.eye(30, 22, 3)
    p.smile(24, 29, 3)
    return p.done()


def star():
    import math
    p = Pic()
    pts = []
    for i in range(10):
        r = 23 if i % 2 == 0 else 10
        a = math.radians(-90 + i * 36)
        pts.append((24 + r * math.cos(a), 25 + r * math.sin(a)))
    p.poly([(x + 1, y + 1) for x, y in pts], "dyellow")
    p.poly(pts, "yellow")
    p.eye(19, 24, 3); p.eye(29, 24, 3)
    p.smile(24, 31, 3)
    p.cheek(15, 30); p.cheek(33, 30)
    return p.done()


# friends suggested by r/MVaveFM1's u/veecheech (CREDITS.md): a T-Rex, a skeleton, a cowboy and a cowgirl, a vacuum
def trex():                                                      # a T-Rex, side on (original design)
    p = Pic()
    p.poly([(34, 26), (45, 37), (44, 41), (32, 38)], "dorange")  # tail
    p.rect(19, 34, 25, 46, "dorange"); p.rect(30, 34, 36, 46, "dorange")   # legs
    p.ell(21, 46, 5, 2, "dorange"); p.ell(34, 46, 5, 2, "dorange")         # feet
    p.ball(28, 30, 13, 11, "orange", "dorange")                  # body
    p.ell(25, 34, 7, 6, "cream")                                 # tummy
    for x in (24, 30, 36):                                       # stripes on the back
        p.poly([(x - 2, 20), (x + 2, 20), (x, 25)], "dorange")
    p.ball(16, 15, 14, 11, "orange", "dorange")                  # big head
    p.pie(10, 20, 8, 0, 180, "dred")                             # a wide-open roar
    p.rect(2, 20, 17, 20, "dred")
    for x in (4, 8, 12, 16):                                     # teeth, top and bottom
        p.poly([(x - 1, 20), (x + 1, 20), (x, 23)], "white")
    for x in (6, 10, 14):
        p.poly([(x - 1, 27), (x + 1, 27), (x, 24)], "white")
    p.ell(10, 25, 3, 1, "hpink")                                  # tongue
    p.eye(19, 11, 4)
    p.rect(14, 5, 24, 6, "dorange")                              # a cross brow (it is pretend-scary)
    p.ell(6, 10, 1, 1, "dorange")                                # nostril
    p.ell(15, 30, 3, 2, "dorange"); p.px(12, 31, "white")         # a tiny arm, with a claw
    return p.done()


def skeleton():                                                  # a dapper skeleton (original design)
    p = Pic()
    p.rect(13, 31, 35, 45, "black")                              # a black suit with pinstripes
    for x in (17, 22, 27, 32):
        p.line([(x, 33), (x, 45)], "dgrey", 1)
    p.line([(13, 33), (5, 42)], "black", 4); p.line([(35, 33), (43, 42)], "black", 4)   # arms
    p.ell(5, 43, 2, 2, "white"); p.ell(43, 43, 2, 2, "white")    # bony hands
    p.poly([(24, 33), (14, 29), (17, 33), (14, 37)], "purple")   # a bat bow tie
    p.poly([(24, 33), (34, 29), (31, 33), (34, 37)], "purple")
    p.ell(24, 33, 2, 2, "dpurple")
    p.ball(24, 15, 15, 14, "white", "dwhite")                    # the skull
    p.ell(17, 13, 5, 6, "black"); p.ell(31, 13, 5, 6, "black")   # big round eye holes
    p.ell(16, 11, 1, 2, "white"); p.ell(30, 11, 1, 2, "white")   # .. with a twinkle
    p.poly([(24, 18), (22, 21), (26, 21)], "black")              # nose
    p.line([(13, 24), (17, 26), (31, 26), (35, 24)], "black", 1) # a stitched grin
    for x in (16, 20, 24, 28, 32):
        p.line([(x, 24), (x, 28)], "black", 1)
    return p.done()


def cowkid(hat, hatd, hair, scarf, shirt, spots=False, braid=False):
    def f():                                                     # a cowboy or cowgirl (original designs)
        p = Pic()
        p.rect(8, 38, 40, 47, shirt)                             # shoulders
        if spots:
            for x, y in ((12, 42), (20, 45), (33, 41), (38, 45)):
                p.ell(x, y, 2, 2, "black")
        else:
            for x in (12, 18, 30, 36):                           # a check shirt
                p.line([(x, 38), (x, 47)], "dorange", 1)
            p.line([(8, 43), (40, 43)], "dorange", 1)
        p.poly([(15, 35), (33, 35), (24, 45)], scarf)            # a neckerchief
        if braid:
            for y in range(24, 44, 4):                           # a long braid over the shoulder
                p.ell(39, y, 3, 3, hair)
            p.ell(39, 45, 2, 2, "yellow")
        p.ball(24, 25, 13, 12, "tan", "dtan")                    # face
        p.rect(11, 15, 37, 19, hair)                             # hair under the hat
        p.eye(19, 24, 3); p.eye(29, 24, 3)
        p.smile(24, 31, 4)
        p.cheek(14, 29); p.cheek(34, 29)
        if braid:
            for x, y in ((16, 28), (18, 29), (30, 29), (32, 28)):   # freckles
                p.px(x, y, "dtan")
        p.ell(24, 14, 22, 3, hat)                                # the brim, wide
        p.ell(24, 15, 22, 2, hatd)
        p.poly([(13, 13), (15, 2), (24, 5), (33, 2), (35, 13)], hat)   # the crown, dented on top
        p.rect(14, 10, 34, 12, hatd)                             # the band
        return p.done()
    return f


def vacuum():                                                    # an upright vacuum cleaner (original design)
    p = Pic()
    p.rect(22, 3, 26, 12, "dgrey"); p.rect(17, 2, 31, 4, "dgrey")   # the handle
    p.ball(24, 22, 13, 13, "purple", "dpurple")                  # the bag
    p.ell(24, 24, 9, 8, "lpurple")                               # .. its face
    p.eye(20, 22, 3); p.eye(28, 22, 3)
    p.smile(24, 28, 3)
    p.cheek(17, 27); p.cheek(31, 27)
    p.rect(8, 36, 40, 42, "silver"); p.rect(8, 41, 40, 42, "dgrey")  # the floor head
    p.rect(10, 35, 38, 36, "red")                                # a red stripe
    p.ell(12, 44, 2, 2, "black"); p.ell(36, 44, 2, 2, "black")   # wheels
    for x, y, r in ((4, 42, 2), (2, 38, 1), (44, 43, 1)):        # dust puffs it is about to eat
        p.ell(x, y, r, r, "grey")
    return p.done()


def spacehero():                                                 # a space hero in a bubble helmet (original design)
    p = Pic()
    p.rect(5, 36, 43, 46, "white")                               # the suit's shoulders
    p.rect(5, 44, 43, 46, "dwhite")
    p.rect(16, 37, 32, 46, "green")                              # the chest panel, its buttons
    p.rect(16, 37, 32, 38, "dgreen")
    for x, c in ((20, "red"), (24, "blue"), (28, "yellow")):
        p.ell(x, 42, 1, 1, c)
    p.rect(5, 36, 9, 46, "purple"); p.rect(39, 36, 43, 46, "purple")   # purple stripes down the arms
    p.ell(24, 21, 21, 20, "lblue")                               # the glass dome
    p.ell(24, 21, 19, 18, "glass")
    p.ball(24, 21, 14, 14, "lpurple", "purple")                  # the hood round the face
    p.ell(24, 23, 10, 11, "tan")                                 # the face
    p.eye(19, 21, 3); p.eye(29, 21, 3)
    p.rect(15, 15, 22, 16, "dbrown"); p.rect(26, 15, 33, 16, "dbrown")  # brave brows
    p.smile(24, 28, 3)
    p.line([(9, 14), (12, 9), (16, 6)], "white", 2)              # the glass's shine
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
    ("PRINCESS DUCKY", ducky("lpurple", "purple", crown)),
    ("RED PUP", redpup),
    ("YELLOW BIRD", yellowbird),
    ("SLIMY", slimy),
    ("BUNNY", bunny),
    ("PANDA", panda),
    ("PENGUIN", penguin),
    ("OWL", owl),
    ("BEE", bee),
    ("LADYBUG", ladybug),
    ("DINO", dino),
    ("WHALE", whale),
    ("OCTOPUS", octopus),
    ("FISH", fish),
    ("TURTLE", turtle),
    ("ICE CREAM", icecream),
    ("CUPCAKE", cupcake),
    ("ROCKET", rocket),
    ("STRAWBERRY", strawberry),
    ("STAR", star),
    ("T-REX", trex),
    ("SKELETON", skeleton),
    ("COWBOY", cowkid("brown", "dbrown", "dbrown", "red", "yellow")),
    ("COWGIRL", cowkid("red", "dred", "orange", "yellow", "white", spots=True, braid=True)),
    ("VACUUM", vacuum),
    ("SPACE HERO", spacehero),
]


# the treble clef of the staff (kid.c): 16 x 46, the G clef (U+1D11E) of the FreeSerif font (GNU FreeFont, GPL-3.0 or
# later with the font exception), scaled down and thresholded, as plain rows here so the build needs no font
CLEF = [
    "........###.....",
    "........####....",
    ".......#####....",
    ".......#####....",
    "......###..##...",
    "......##...##...",
    "......##...##...",
    "......##...##...",
    "......#....##...",
    "......#...###...",
    "......#...###...",
    "......#..###....",
    "......######....",
    ".......#####....",
    "......#####.....",
    ".....#####......",
    "....######......",
    "...######.......",
    "...######.......",
    "..####..#.......",
    ".#####..#.......",
    ".####...##......",
    "####....#####...",
    "###...########..",
    "###...#########.",
    "##...#####.#####",
    "##...##..#...###",
    "##...##..##...##",
    "##...##...#...##",
    "##...##...#...##",
    ".##...#...#...##",
    "..#....#..#...##",
    "..##......##.##.",
    "...##......###..",
    ".....########...",
    "........#..#....",
    "...........##...",
    "...........##...",
    "............#...",
    ".....###....#...",
    "....#####...#...",
    "...######...#...",
    "...######...#...",
    "....####...##...",
    "....###...##....",
    "......#####.....",
]


def rgb565(c):
    r, g, b = c[:3]
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)


def main(out, png=None):
    for name, _ in FRIENDS:                      # the band shows the name: one line at scale 4 or 3, or two at 3
        if not re.fullmatch(r"[A-Z-]+( [A-Z-]+)*", name):
            raise SystemExit(f"{name!r}: capitals, hyphens and single spaces only (the band's font)")
        if len(name) * 18 - 3 > 232 and (" " not in name or len(name) > 15 or
                                          max(len(w) for w in name.split(" ", 1)) * 18 - 3 > 232):
            raise SystemExit(f"{name}: too long for the band, even on two lines")
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
    lines.append(f"#define KID_CLEF_W {len(CLEF[0])}")
    lines.append(f"#define KID_CLEF_H {len(CLEF)}")
    lines.append(f"static const uint16_t KID_CLEF[{len(CLEF)}] = {{    /* bit 15 the left column */")
    lines.append("    " + ", ".join(f"0x{sum(1 << (15 - x) for x, c in enumerate(r) if c == '#'):04X}" for r in CLEF))
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
