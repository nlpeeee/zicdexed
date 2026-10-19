#!/usr/bin/env python3
"""Render ZicDexed UI mockups at the real 320x170 screen size.

Layout: the 4 encoders sit physically ABOVE the screen, so the top band holds
one "lane" per encoder. The keys sit BELOW the screen, so the bottom band
labels the top key row. The middle is the page's picture.

Usage: render_mockups.py <output dir>
Fonts: Inter (SIL OFL). Firmware would pre-render the same faces to bitmaps.
"""
import math
import os
import sys

from PIL import Image, ImageDraw, ImageFont

W, H = 320, 170
SS = 4
INTER = "/usr/share/fonts/opentype/inter/"

BG = (6, 6, 8)
PANEL = (22, 23, 27)
LINE = (40, 42, 48)
GREY = (120, 122, 130)
DIM = (62, 64, 70)
WHITE = (238, 238, 240)
INK = (6, 6, 8)
# one hot colour per tone generator; the whole screen takes the current TG's colour
TG = [(190, 255, 40), (255, 110, 30), (40, 210, 255), (255, 60, 150),
      (255, 220, 40), (150, 110, 255), (60, 240, 170), (255, 70, 70)]

_f = {}


def F(face, size):
    key = (face, size)
    if key not in _f:
        _f[key] = ImageFont.truetype(INTER + face + ".otf", size)
    return _f[key]


LBL = ("Inter-SemiBold", 9)
SMALL = ("Inter-Medium", 10)
BODY = ("Inter-SemiBold", 12)
VAL = ("InterDisplay-Bold", 21)
BIG = ("InterDisplay-Black", 30)
HUGE = ("InterDisplay-Black", 44)


def dk(c, k):
    return tuple(int(v * k) for v in c)


class Canvas:
    def __init__(self, accent):
        self.img = Image.new("RGB", (W, H), BG)
        self.a = accent

    def shape(self, fn):
        layer = Image.new("RGBA", (W * SS, H * SS), (0, 0, 0, 0))
        fn(ImageDraw.Draw(layer), SS)
        layer = layer.resize((W, H), Image.LANCZOS)
        base = self.img.convert("RGBA")
        base.alpha_composite(layer)
        self.img = base.convert("RGB")

    def rect(self, x, y, w, h, fill=None, r=0, outline=None, ow=1):
        def f(d, s):
            box = [x * s, y * s, (x + w) * s - 1, (y + h) * s - 1]
            d.rounded_rectangle(box, r * s, fill=fill, outline=outline, width=ow * s if outline else 0)
        self.shape(f)

    def text(self, x, y, t, font, color=WHITE, align="left", width=0, track=0):
        fnt = F(*font)
        d = ImageDraw.Draw(self.img)
        if track:  # letter spacing for small caps labels
            tw = sum(d.textlength(c, font=fnt) + track for c in t) - track
        else:
            tw = d.textlength(t, font=fnt)
        if align == "center":
            x += (width - tw) / 2
        elif align == "right":
            x += width - tw
        if track:
            for c in t:
                d.text((x, y), c, font=fnt, fill=color)
                x += d.textlength(c, font=fnt) + track
        else:
            d.text((x, y), t, font=fnt, fill=color)
        return tw

    def tw(self, t, font):
        return ImageDraw.Draw(self.img).textlength(t, font=F(*font))


# ------------------------------------------------------------------ chrome
def lanes(cv, items, touched=None):
    """Top band: one lane per encoder, directly under its knob.
    item = (label, value, frac, bipolar)"""
    for i, it in enumerate(items):
        label, value, frac = it[:3]
        bip = len(it) > 3 and it[3]
        x = i * 80
        hot = i == touched
        if hot:
            cv.rect(x + 1, 0, 78, 44, cv.a, r=4)
        fg, lab = (INK, INK) if hot else (WHITE, GREY)
        cv.text(x + 7, 4, label.upper(), LBL, lab, track=1)
        cv.text(x + 6, 13, value, VAL, fg)
        # value track
        bx, by, bw = x + 7, 38, 66
        cv.rect(bx, by, bw, 3, dk(INK, 1) if hot else LINE, r=1)
        if bip:
            mid = bx + bw / 2
            pos = bx + bw * frac
            a, b = sorted([mid, pos])
            cv.rect(a, by, max(2, b - a), 3, INK if hot else cv.a, r=1)
            cv.rect(mid - 0.5, by - 2, 1, 7, INK if hot else GREY)
        else:
            cv.rect(bx, by, max(2, bw * frac), 3, INK if hot else cv.a, r=1)
        if i and not hot and i - 1 != touched:
            cv.rect(x, 6, 1, 32, LINE)


PAGES = ["VOICE", "FM", "OPS", "MOD"]


def footer(cv, page=None, sub=0, subs=1, labels=None, tg=0, muted=(), playing=()):
    """Bottom band, nearest the keys: labels for the top key row (q w e r)."""
    y = 154
    cv.rect(0, y - 2, W, 1, LINE)
    labs = labels or PAGES
    for i, lab in enumerate(labs):
        x = i * 80
        on = page == i
        if on:
            cv.rect(x + 2, y + 1, 76, 14, cv.a, r=3)
        cv.text(x, y + 2, lab, LBL, INK if on else (GREY if lab else DIM), "center", 80, track=1)
        if on and subs > 1:
            for k in range(subs):
                cv.rect(x + 70 - (subs - 1 - k) * 5, y + 6, 3, 3, INK if k == sub else dk(INK, 1) if False else (80, 90, 40) if k != sub else INK, r=1)


def title(cv, tg, name, right=None, y=48):
    """TG badge + voice name at the top of the picture area."""
    cv.rect(4, y + 1, 26, 15, cv.a, r=3)
    cv.text(4, y + 2, f"TG{tg + 1}", ("Inter-Bold", 10), INK, "center", 26)
    cv.text(36, y - 1, name, ("InterDisplay-Bold", 15), WHITE)
    if right:
        cv.text(150, y + 2, right, SMALL, GREY, "right", 165)


def tg_dots(cv, cur, muted=(), playing=(), x0=232, y=52):
    for i in range(8):
        x = x0 + i * 10
        c = TG[i]
        if i in muted:
            cv.rect(x, y, 7, 7, None, r=2, outline=DIM)
        elif i == cur:
            cv.rect(x - 1, y - 1, 9, 9, c, r=2)
        else:
            cv.rect(x, y, 7, 7, dk(c, 0.9 if i in playing else 0.3), r=2)


# ------------------------------------------------------------------ pages
def scope(cv, x0, y0, w, h, seed=0):
    def f(d, s):
        pts = []
        for i in range(w + 1):
            t = i / w
            v = (math.sin(t * 2 * math.pi * 3) * 0.55 + math.sin(t * 2 * math.pi * 3.02 * 2 + 1) * 0.35
                 + math.sin(t * 2 * math.pi * 9.1) * 0.12)
            v = max(-1, min(1, v * 1.25))  # a bit of clipping: it's a reese
            pts.append(((x0 + i) * s, (y0 + h / 2 - v * h / 2 * 0.9) * s))
        glow = dk(cv.a, 0.25)
        d.line(pts, fill=glow, width=5 * s)
        d.line(pts, fill=cv.a, width=int(1.5 * s))
    cv.shape(f)


def page_voice(cv):
    lanes(cv, [("Cutoff", "72", 0.72), ("Reso", "45", 0.45), ("Detune", "+12", 0.6, True), ("Volume", "100", 100 / 127)])
    tg_dots(cv, 0, muted=(4,), playing=(0, 1, 2))
    cv.text(4, 44, "REESE BS", HUGE, WHITE)
    cv.text(6, 98, "nasty  ·  01 / 32", SMALL, GREY)
    cv.text(6, 112, "MONO   PORTA 35   PB 12", SMALL, dk(cv.a, 0.85))
    scope(cv, 150, 100, 164, 44)
    footer(cv, page=0, sub=0, subs=3)


ALG5 = [(1, 2), (3, 4), (5, 6)]


def algorithm(cv, x0, y0, levels, sel=None, muted=()):
    bw, bh, gx, gy = 34, 24, 18, 14
    pos = {}
    for i, (car, mod) in enumerate(ALG5):
        x = x0 + i * (bw + gx)
        pos[mod] = (x, y0)
        pos[car] = (x, y0 + bh + gy)
    yb = y0 + 2 * bh + gy + 8

    def wires(d, s):
        for car, mod in ALG5:
            mx, my = pos[mod]
            cx, cy = pos[car]
            d.line([((mx + bw / 2) * s, (my + bh) * s), ((cx + bw / 2) * s, cy * s)], fill=GREY, width=s)
            d.line([((cx + bw / 2) * s, (cy + bh) * s), ((cx + bw / 2) * s, yb * s)], fill=GREY, width=s)
        d.line([((x0 + bw / 2) * s, yb * s), ((x0 + 2 * (bw + gx) + bw / 2) * s, yb * s)], fill=GREY, width=s)
        fx, fy = pos[6]
        loop = [((fx + bw) * s, (fy + bh / 2) * s), ((fx + bw + 7) * s, (fy + bh / 2) * s),
                ((fx + bw + 7) * s, (fy - 5) * s), ((fx + bw / 2) * s, (fy - 5) * s), ((fx + bw / 2) * s, fy * s)]
        d.line(loop, fill=dk(cv.a, 0.3), width=6 * s)
        d.line(loop, fill=cv.a, width=2 * s)
    cv.shape(wires)
    for op, (x, y) in pos.items():
        lvl = levels[op - 1] / 99
        carrier = op % 2 == 1
        if op in muted:
            cv.rect(x, y, bw, bh, None, r=4, outline=DIM)
            cv.text(x, y + 4, str(op), BODY, DIM, "center", bw)
            continue
        cv.rect(x, y, bw, bh, PANEL, r=4, outline=cv.a if op == sel else (dk(cv.a, 0.6) if carrier else LINE),
                ow=2 if op == sel else 1)
        fh = (bh - 4) * lvl
        cv.rect(x + 2, y + bh - 2 - fh, bw - 4, fh, dk(cv.a, 0.55 if carrier else 0.3), r=2)
        cv.text(x, y + 4, str(op), BODY, WHITE, "center", bw)


LEVELS = [99, 82, 99, 88, 97, 76]


def page_fm(cv):
    lanes(cv, [("Algo", "5", 4 / 31), ("Feedback", "7", 1.0), ("Bright", "+18", 0.68, True), ("Snarl", "30", 0.3)], touched=1)
    algorithm(cv, 10, 56, LEVELS)
    cv.text(180, 52, "ALGORITHM 5", LBL, GREY, track=1)
    cv.text(180, 62, "3 stacks", BODY, WHITE)
    cv.text(180, 84, "FEEDBACK", LBL, GREY, track=1)
    for i in range(7):
        on = i < 7
        cv.rect(180 + i * 18, 96, 14, 10, cv.a if on else LINE, r=2)
    cv.text(180, 116, "max: noise + grit", SMALL, dk(cv.a, 0.85))
    footer(cv, page=1, sub=0, subs=2)


OPR = ["1.00", "1.00", "0.50", "1.01", "2.00", "3.50"]


def page_ops(cv, sel=5):
    lanes(cv, [("Op", "6", 1.0), ("Level", "76", 76 / 99), ("Ratio", "3.50", 0.3), ("Detune", "+3", 0.71, True)])
    cw = 52
    for i in range(6):
        x = 4 + i * cw
        s = i == sel
        muted = i == 3 and False
        cv.rect(x, 50, cw - 6, 98, dk(cv.a, 0.16) if s else PANEL, r=5, outline=cv.a if s else None, ow=1)
        cv.text(x + 6, 53, f"OP{i + 1}", LBL, cv.a if s else GREY, track=1)
        cv.text(x + 6, 64, OPR[i], BODY, WHITE)
        # level bar
        bh = 54 * LEVELS[i] / 99
        cv.rect(x + 6, 138 - 54, 6, 54, LINE, r=2)
        cv.rect(x + 6, 138 - bh, 6, bh, cv.a if s else dk(cv.a, 0.5), r=2)
        cv.text(x + 16, 128, str(LEVELS[i]), SMALL, WHITE if s else GREY)
        if i % 2 == 0:
            cv.text(x + 16, 84, "OUT", LBL, dk(cv.a, 0.8), track=1)
    footer(cv, page=2, sub=0, subs=2)


def page_env(cv):
    lanes(cv, [("Rate 1", "92", 0.92), ("Rate 2", "60", 0.6), ("Rate 3", "40", 0.4), ("Rate 4", "55", 0.55)])
    cv.text(6, 47, "OP6", BODY, cv.a)
    cv.text(36, 48, "envelope  ·  Shift: levels", SMALL, GREY)
    tg_dots(cv, 0, muted=(4,), playing=(0,))
    rates, levels = [92, 60, 40, 55], [99, 80, 70, 0]
    x0, y0, w, h = 10, 76, 300, 70
    seg = [(100 - r) / 100 * 80 + 8 for r in rates]
    pts = [(x0, y0 + h)]
    x = x0
    for i in range(3):
        x += seg[i]
        pts.append((x, y0 + h - levels[i] / 99 * h))
    x += 110
    pts.append((x, pts[-1][1]))
    x += seg[3]
    pts.append((x, y0 + h))

    def f(d, s):
        for gx in range(0, 7):
            d.line([((x0 + gx * 50) * s, y0 * s), ((x0 + gx * 50) * s, (y0 + h) * s)], fill=(18, 19, 22), width=s)
        poly = [(px * s, py * s) for px, py in pts]
        d.polygon(poly, fill=dk(cv.a, 0.18))
        d.line(poly, fill=dk(cv.a, 0.3), width=6 * s)
        d.line(poly, fill=cv.a, width=2 * s)
        for k, (px, py) in enumerate(pts[1:4]):
            r = 4
            d.ellipse([(px - r) * s, (py - r) * s, (px + r) * s, (py + r) * s], fill=BG, outline=WHITE, width=2 * s)
    cv.shape(f)
    for k, (px, py) in enumerate(pts[1:4]):
        cv.text(px + 6, py + 2, f"L{k + 1}", LBL, GREY, track=1)
    footer(cv, page=2, sub=1, subs=2)


def page_mod(cv):
    cv.a = TG[1]
    lanes(cv, [("Wave", "TRI", 0.2), ("Speed", "62", 0.62), ("Pitch", "0", 0.0), ("Amp", "70", 0.7)])
    title(cv, 1, "WOBBLE BS", right="LFO  ·  sync 1/8")
    x0, y0, w, h = 8, 72, 304, 72

    def f(d, s):
        d.line([(x0 * s, (y0 + h / 2) * s), ((x0 + w) * s, (y0 + h / 2) * s)], fill=LINE, width=s)
        pts = []
        for i in range(w + 1):
            t = i / w * 4 * 2 * math.pi
            v = (2 / math.pi) * math.asin(math.sin(t))
            pts.append(((x0 + i) * s, (y0 + h / 2 - v * h * 0.42) * s))
        d.line(pts, fill=dk(cv.a, 0.3), width=6 * s)
        d.line(pts, fill=cv.a, width=2 * s)
        px = x0 + w * 0.37
        d.line([(px * s, y0 * s), (px * s, (y0 + h) * s)], fill=WHITE, width=s)
    cv.shape(f)
    footer(cv, page=3, sub=0, subs=3)


MIX = [(100, 0.5), (90, 0.4), (70, 0.6), (80, 0.5), (0, 0.5), (60, 0.3), (75, 0.7), (50, 0.5)]
NAMES = ["REESE", "WOBBL", "KICK", "HAT", "", "SNARE", "HOOVR", "SCRM"]


def page_mix(cv):
    lanes(cv, [("TG", "1", 0.0), ("Volume", "100", 100 / 127), ("Pan", "C", 0.5, True), ("Reverb", "20", 0.2)])
    sw = 40
    for i, (vol, pan) in enumerate(MIX):
        x = i * sw
        c = TG[i]
        cur = i == 0
        muted = i == 4
        if cur:
            cv.rect(x + 2, 48, sw - 4, 100, dk(c, 0.15), r=4, outline=c)
        cv.text(x, 51, str(i + 1), BODY, c if not muted else DIM, "center", sw)
        top, hh = 70, 56
        cv.rect(x + 16, top, 8, hh, PANEL, r=2)
        if not muted:
            bh = hh * vol / 127
            cv.rect(x + 16, top + hh - bh, 8, bh, c, r=2)
            # live level tick
            if i in (0, 1, 2):
                cv.rect(x + 13, top + hh - bh * 0.8, 14, 2, WHITE)
        else:
            cv.text(x, top + 20, "M", BODY, DIM, "center", sw)
        cv.text(x, 132, NAMES[i], LBL, GREY if not muted else DIM, "center", sw)
    cv.text(150, 135, "", SMALL)
    footer(cv, labels=["MUTE", "SOLO", "", ""])


BROWSE = ["REESE BS", "WOBBLE BS", "NEURO STB", "GRIT SUB", "LASER ZAP", "NOISE HAT", "FM KICK"]


def page_browse(cv, sel=2):
    lanes(cv, [("Voice", "03", 2 / 31), ("Bank", "nasty", 0.0), ("To", "TG1", 0.0), ("Volume", "100", 100 / 127)])
    for i, name in enumerate(BROWSE):
        y = 47 + i * 15
        if i == sel:
            cv.rect(2, y, 316, 15, cv.a, r=3)
        cv.text(8, y + 1, f"{i + 1:02d}", SMALL, INK if i == sel else DIM)
        cv.text(30, y, name, BODY, INK if i == sel else (WHITE if abs(i - sel) == 1 else GREY))
        if i == sel:
            cv.text(200, y + 1, "playing preview", SMALL, INK, "right", 110)
    footer(cv, labels=["VOICES", "PERFS", "", "LOAD (11)"], page=0)


def page_keys(cv):
    lanes(cv, [("Octave", "2", 0.4), ("Scale", "MINOR", 0.3), ("Root", "C", 0.0), ("Velocity", "110", 110 / 127)])
    notes = ["C", "D", "Eb", "F", "G", "Ab", "Bb", "C"]
    kw, kh = 75, 49
    for i, n in enumerate(notes):
        r, c = divmod(i, 4)
        x, y = 4 + c * (kw + 4), 48 + r * (kh + 4)
        pressed = i in (0, 4)
        root = n == "C"
        cv.rect(x, y, kw, kh, cv.a if pressed else PANEL, r=6, outline=dk(cv.a, 0.6) if root and not pressed else None)
        cv.text(x, y + 6, n, BIG, INK if pressed else WHITE, "center", kw)
        cv.text(x + 6, y + 3, str(i + 1), LBL, INK if pressed else DIM)
    footer(cv, labels=["", "", "", "EXIT (12)"])


SHIFT = [
    ["INIT", "MUTATE", "OP MUTE", "FX"],
    ["TG MUTE", "TG SOLO", "SAVE VCE", "SAVE PRF"],
    ["SHIFT", "KEYS", "REVERT", "MENU"],
]
NORMAL = [
    ["VOICE", "FM", "OPS", "MOD"],
    ["< TG", "TG >", "BROWSE", "MIX"],
    ["SHIFT", "PLAY", "COMPARE", "EXIT"],
]


def page_shift(cv):
    """Shift held: the screen becomes a map of the 12 keys."""
    cv.text(6, 2, "SHIFT", ("InterDisplay-Black", 18), cv.a)
    cv.text(70, 7, "encoders: fine steps", SMALL, GREY)
    kw, kh = 76, 38
    for r in range(3):
        for c in range(4):
            x, y = 4 + c * (kw + 2), 30 + r * (kh + 6)
            lab = SHIFT[r][c]
            held = (r, c) == (2, 0)
            cv.rect(x, y, kw, kh, cv.a if held else PANEL, r=5)
            cv.text(x + 6, y + 3, str(r * 4 + c + 1), LBL, INK if held else DIM)
            cv.text(x, y + 13, lab, BODY, INK if held else WHITE, "center", kw)
            cv.text(x, y + 26, NORMAL[r][c].lower(), ("Inter-Medium", 9), INK if held else GREY, "center", kw)


VIEWS = [
    ("01_voice", "VOICE  q", lambda cv: page_voice(cv)),
    ("02_fm", "FM  w  (turning Feedback)", page_fm),
    ("03_ops", "OPS  e", page_ops),
    ("04_envelope", "OPS  e again: envelope", page_env),
    ("05_mod", "MOD  r  (TG2)", page_mod),
    ("06_mix", "MIX  f", page_mix),
    ("07_browse", "BROWSE  d", page_browse),
    ("08_keys", "KEYS  Shift+x", page_keys),
    ("09_shift", "Shift held: key map", page_shift),
]


def device_frame(img, scale=2):
    """Put a screen into a rough ZicPixel outline: 4 knobs above, 12 keys below."""
    sw, sh = W * scale, H * scale
    pad = 30
    fw = sw + pad * 2
    knob_h, key_h = 70, 3 * 92
    fh = knob_h + sh + 24 + key_h + pad
    fr = Image.new("RGB", (fw, fh), (225, 226, 228))
    d = ImageDraw.Draw(fr)
    for i in range(4):
        cx = pad + sw * (i + 0.5) / 4
        d.ellipse([cx - 26, 14, cx + 26, 66], fill=(25, 25, 27))
    d.rounded_rectangle([pad - 8, knob_h - 8, pad + sw + 8, knob_h + sh + 8], 8, fill=(30, 60, 140))
    fr.paste(img.resize((sw, sh), Image.LANCZOS), (pad, knob_h))
    ky = knob_h + sh + 24
    kw = sw / 4
    for r in range(3):
        for c in range(4):
            x, y = pad + c * kw + 6, ky + r * 92
            d.rounded_rectangle([x, y, x + kw - 12, y + 84], 10, fill=(250, 250, 250), outline=(200, 200, 205))
            d.text((x + 10, y + 8), str(r * 4 + c + 1), fill=(150, 150, 155), font=F("Inter-SemiBold", 14))
            d.text((x + 10, y + 56), NORMAL[r][c], fill=(60, 60, 65), font=F("Inter-SemiBold", 14))
    return fr


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    shots = []
    for name, cap, fn in VIEWS:
        cv = Canvas(TG[0])
        fn(cv)
        cv.img.save(os.path.join(out, name + ".png"))
        cv.img.resize((W * 3, H * 3), Image.NEAREST).save(os.path.join(out, name + "@3x.png"))
        shots.append((cap, cv.img))
    sc, pad, caph, cols = 2, 20, 30, 3
    rows = math.ceil(len(shots) / cols)
    sheet = Image.new("RGB", (pad + cols * (W * sc + pad), pad + rows * (H * sc + caph + pad)), (16, 17, 20))
    d = ImageDraw.Draw(sheet)
    for i, (cap, im) in enumerate(shots):
        r, c = divmod(i, cols)
        x, y = pad + c * (W * sc + pad), pad + r * (H * sc + caph + pad)
        d.text((x, y + 2), cap, font=F("Inter-SemiBold", 18), fill=WHITE)
        sheet.paste(im.resize((W * sc, H * sc), Image.NEAREST), (x, y + caph))
    sheet.save(os.path.join(out, "contact_sheet.png"))
    device_frame(shots[0][1]).save(os.path.join(out, "device_voice.png"))


if __name__ == "__main__":
    main()
