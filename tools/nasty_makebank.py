import sys, copy, json

def op(coarse=1, fine=0, det=7, lvl=99, R=(99,99,99,99), L=(99,99,99,0), vel=0, ams=0, rs=0, fixed=0):
    return dict(coarse=coarse, fine=fine, det=det, lvl=lvl, R=R, L=L, vel=vel, ams=ams, rs=rs, fixed=fixed)
OFF = dict(lvl=0)

def voice(name, alg, ops, fb=0, peg=((99,99,99,99),(50,50,50,50)), lfo=None, trans=24, oks=1):
    o = {i: op(**ops.get(i, OFF)) for i in range(1,7)}
    return dict(name=name, alg=alg, ops=o, fb=fb, peg=peg, lfo=lfo or {}, trans=trans, oks=oks)

def pack(v):
    b = []
    for i in range(6,0,-1):
        o = v['ops'][i]
        b += list(o['R']) + list(o['L'])
        b += [0, 0, 0, 0]                     # BP, LD, RD, curves
        b += [(o['det'] << 3) | o['rs']]
        b += [(o['vel'] << 2) | o['ams']]
        b += [o['lvl']]
        b += [(o['coarse'] << 1) | o['fixed']]
        b += [o['fine']]
    pr, pl = v['peg']
    b += list(pr) + list(pl)
    b += [v['alg'] - 1, (v['oks'] << 3) | v['fb']]
    l = v['lfo']
    b += [l.get('speed', 35), l.get('delay', 0), l.get('pmd', 0), l.get('amd', 0),
          (l.get('pms', 0) << 4) | (l.get('wave', 0) << 1) | l.get('sync', 0), v['trans']]
    b += list(v['name'].ljust(10)[:10].encode('ascii'))
    assert len(b) == 128, len(b)
    assert all(0 <= x <= 127 for x in b), (v['name'], b)
    return b

BASS = dict(R=(99,55,40,60), L=(99,88,85,0))
V = []
# 1 Reese: three detuned FM saws, an octave down
V.append(voice('REESE BS', 5, {
  1: dict(det=4, **BASS), 2: dict(lvl=80, R=(99,50,30,60), L=(99,90,88,0)),
  3: dict(det=7, lvl=96, **BASS), 4: dict(lvl=80, R=(99,50,30,60), L=(99,90,88,0)),
  5: dict(det=11, lvl=96, **BASS), 6: dict(lvl=82, R=(99,50,30,60), L=(99,90,88,0))},
  fb=6, trans=24))
# 2 Wobble: LFO on the modulators sweeps brightness
V.append(voice('WOBBLE BS', 5, {
  1: dict(det=6, **BASS), 2: dict(lvl=88, ams=3),
  3: dict(det=8, lvl=95, **BASS), 4: dict(lvl=86, ams=3),
  5: dict(coarse=0, lvl=90, **BASS), 6: dict(lvl=70)},
  fb=5, trans=24, lfo=dict(speed=28, amd=85, wave=0)))
# 3 Neuro stab: inharmonic stack
V.append(voice('NEURO STB', 1, {
  1: dict(R=(99,45,30,55), L=(99,80,70,0)), 2: dict(coarse=1, fine=41, lvl=86, R=(99,40,30,50), L=(99,70,60,0)),
  3: dict(coarse=0, R=(99,45,30,55), L=(99,80,70,0)), 4: dict(lvl=88), 5: dict(coarse=3, lvl=72), 6: dict(coarse=7, lvl=66)},
  fb=7, trans=24))
# 4 Grit sub: clean sub with a noisy feedback modulator for crunch
V.append(voice('GRIT SUB', 16, {
  1: dict(coarse=0, **BASS), 2: dict(coarse=0, lvl=74),
  3: dict(coarse=1, lvl=60), 4: dict(coarse=2, lvl=60),
  5: dict(coarse=1, lvl=62), 6: dict(coarse=1, lvl=90)},
  fb=7, trans=24))
# 5 Laser zap: pitch envelope dive
V.append(voice('LASER ZAP', 1, {
  1: dict(R=(99,50,50,60), L=(99,0,0,0)), 2: dict(lvl=82, R=(99,45,50,60), L=(99,0,0,0)),
  3: dict(coarse=2, lvl=70, R=(99,55,50,60), L=(99,0,0,0)), 4: dict(coarse=3, lvl=75)},
  peg=((50,99,99,99),(10,10,10,99)), trans=24))
# 6 Noise hat: feedback noise into a high carrier
V.append(voice('NOISE HAT', 5, {
  5: dict(coarse=11, fine=37, R=(99,56,99,56), L=(99,0,0,0), rs=3), 6: dict(coarse=15, lvl=99, R=(99,99,99,99), L=(99,99,99,0))},
  fb=7, trans=24))
# 7 FM kick
V.append(voice('FM KICK', 1, {
  1: dict(coarse=0, R=(99,48,40,60), L=(99,0,0,0)), 2: dict(coarse=1, lvl=78, R=(99,85,99,99), L=(99,0,0,0))},
  peg=((72,99,99,99),(50,50,50,82)), trans=24))
# 8 Metal snare
V.append(voice('METAL SNR', 5, {
  1: dict(coarse=1, R=(99,62,99,62), L=(99,0,0,0)), 2: dict(coarse=1, fine=41, lvl=80, R=(99,70,99,70), L=(99,0,0,0)),
  3: dict(coarse=2, lvl=85, R=(99,66,99,66), L=(99,0,0,0)), 4: dict(coarse=3, fine=70, lvl=82),
  5: dict(coarse=9, fine=13, lvl=95, R=(99,60,99,60), L=(99,0,0,0)), 6: dict(coarse=15, lvl=99)},
  fb=7, trans=24, peg=((80,99,99,99),(50,50,50,62))))
# 9 Detuned lead with delayed vibrato
V.append(voice('DETUNE LD', 5, {
  1: dict(det=3, R=(99,70,40,55), L=(99,95,90,0)), 2: dict(lvl=76),
  3: dict(det=11, lvl=96, R=(99,70,40,55), L=(99,95,90,0)), 4: dict(lvl=76),
  5: dict(coarse=2, det=7, lvl=80, R=(99,70,40,55), L=(99,95,90,0)), 6: dict(lvl=72)},
  fb=6, lfo=dict(speed=34, delay=45, pmd=10, pms=3, wave=4)))
# 10 Acid squelch: snappy modulator envelope, velocity sensitive
V.append(voice('ACID SQLCH', 16, {
  1: dict(R=(99,50,40,60), L=(99,90,85,0)), 2: dict(lvl=92, vel=6, R=(99,60,35,60), L=(99,45,40,0)),
  5: dict(lvl=64, R=(99,60,35,60), L=(99,50,40,0)), 6: dict(lvl=80)},
  fb=6, trans=24))
# 11 Hoover: pitch scoop up and drop on release
V.append(voice('HOOVER', 5, {
  1: dict(det=4, R=(80,60,40,45), L=(99,92,90,0)), 2: dict(lvl=78),
  3: dict(det=10, lvl=95, R=(80,60,40,45), L=(99,92,90,0)), 4: dict(coarse=2, lvl=70),
  5: dict(coarse=0, det=7, lvl=92, R=(80,60,40,45), L=(99,92,90,0)), 6: dict(lvl=74)},
  fb=6, peg=((62,99,99,40),(50,50,50,36)), lfo=dict(speed=40, pmd=6, pms=3, wave=0)))
# 12 Ring bleep: fixed-frequency modulator, inharmonic across the keys
V.append(voice('RING BLEEP', 1, {
  1: dict(R=(99,58,99,60), L=(99,0,0,0)), 2: dict(fixed=1, coarse=3, fine=33, lvl=84)},
  trans=24))
# 13 Feedback scream
V.append(voice('FB SCREAM', 32, {
  1: dict(det=9, lvl=86, R=(99,60,50,55), L=(99,90,85,0)), 6: dict(lvl=99, R=(99,60,50,55), L=(99,90,85,0))},
  fb=7, lfo=dict(speed=30, pmd=8, pms=4, wave=0)))
# 14 Dirty chord: detuned stacked partials plus a noisy one
V.append(voice('DIRTY CHRD', 32, {
  1: dict(coarse=0, det=5, lvl=88, R=(99,50,40,55), L=(99,80,75,0)), 2: dict(coarse=1, det=9, lvl=86, R=(99,50,40,55), L=(99,80,75,0)),
  3: dict(coarse=1, fine=50, det=4, lvl=82, R=(99,50,40,55), L=(99,80,75,0)), 4: dict(coarse=2, det=10, lvl=80, R=(99,50,40,55), L=(99,80,75,0)),
  5: dict(coarse=3, det=6, lvl=74, R=(99,50,40,55), L=(99,80,75,0)), 6: dict(coarse=1, lvl=76, R=(99,50,40,55), L=(99,80,75,0))},
  fb=7))
# 15 S&H glitch: random pitch steps
V.append(voice('S&H GLITCH', 1, {
  1: dict(R=(99,60,40,60), L=(99,85,80,0)), 2: dict(coarse=3, fine=50, lvl=84)},
  lfo=dict(speed=78, pmd=99, pms=6, wave=5)))
# 16 Talking growl: sawtooth LFO on the modulators
V.append(voice('TALK GROWL', 1, {
  1: dict(coarse=0, **BASS), 2: dict(coarse=0, lvl=90, ams=3),
  3: dict(coarse=0, lvl=90, **BASS), 4: dict(coarse=1, lvl=84, ams=2), 5: dict(coarse=2, lvl=66, ams=3), 6: dict(coarse=1, lvl=70)},
  fb=6, trans=24, lfo=dict(speed=42, amd=80, wave=2)))

# Variants 17-32: harder versions (more modulation and feedback, wider detune)
variants = []
XNAMES = ['REESE HARD','WOBBLE HRD','NEURO HARD','GRIT NOISE','ZAP HARD','HAT HARD','KICK HARD','SNARE HARD','LEAD HARD','ACID HARD','HOOVER HRD','BLEEP HARD','SCREAM HRD','CHORD HARD','GLITCH HRD','GROWL HARD']
for v in V:
    w = copy.deepcopy(v)
    w['name'] = XNAMES[len(variants)]
    w['fb'] = 7
    for i, o in w['ops'].items():
        o['det'] = max(0, min(14, 7 + (o['det'] - 7) * 2))
    # boost modulators: any op that isn't a carrier in the algorithm
    carriers = {1:{1,3},5:{1,3,5},16:{1},32:{1,2,3,4,5,6}}[w['alg']]
    for i, o in w['ops'].items():
        if i not in carriers and o['lvl'] > 0:
            o['lvl'] = min(99, o['lvl'] + 7)
    variants.append(w)
V += variants
assert len(V) == 32

data = []
for v in V:
    data += pack(v)
cs = (128 - (sum(data) & 0x7F)) & 0x7F
syx = bytes([0xF0, 0x43, 0x00, 0x09, 0x20, 0x00] + data + [cs, 0xF7])
assert len(syx) == 4104
open(sys.argv[1], 'wb').write(syx)
print('\n'.join('%2d %s' % (i+1, v['name']) for i, v in enumerate(V)))
