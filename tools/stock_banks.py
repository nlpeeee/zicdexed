#!/usr/bin/env python3
"""Build the stock voice banks for the SD card (sysex/voice/).

Usage: stock_banks.py <Dexed cart folder (searched recursively)> <out dir>

MiniDexed numbers banks from 1, skips a number that is already taken, and
shows whatever follows the number in the file name as the bank name. This
copies the Yamaha ROM3/ROM4 and VRC-101..112 banks (the set getsysex.sh
downloads) to 000001..000027 with a themed name each, and checks they are
4104 bytes. Bank 90 (nasty) and 99 (ZICUSER) are left for the build.
"""
import os, re, shutil, sys

# (number, source key, themed name); source key = ROM3A.. or VRC-1xx A/B
BANKS = [
    (1, "ROM3A", "Mixed Classics"), (2, "ROM3B", "Keys & Guitars"),
    (3, "ROM4A", "Orchestral"), (4, "ROM4B", "Splits & FX"),
    (5, "VRC-101B", "Bass & Plucked"), (6, "VRC-102A", "Woodwind & Horns"),
    (7, "VRC-102B", "Brass & Folk"), (8, "VRC-103A", "Strings & Vox"),
    (9, "VRC-103B", "Choirs & Organs"), (10, "VRC-104A", "Drums"),
    (11, "VRC-104B", "Percussion"), (12, "VRC-105A", "SFX Weather Cars"),
    (13, "VRC-105B", "SFX Animals Misc"), (14, "VRC-106A", "Synth Str Brass"),
    (15, "VRC-106B", "Synth Bass Misc"), (16, "VRC-107A", "Bristow Keys Vox"),
    (17, "VRC-107B", "Bristow Pluck Bs"), (18, "VRC-108A", "Leuenberger 1"),
    (19, "VRC-108B", "Leuenberger 2"), (20, "VRC-109A", "Studio64 Keys"),
    (21, "VRC-109B", "Studio64 Wind"), (22, "VRC-110A", "Tomlyn Keys"),
    (23, "VRC-110B", "Tomlyn Pads Lead"), (24, "VRC-111A", "Tomlyn2 Mallets"),
    (25, "VRC-111B", "Tomlyn2 Pads"), (26, "VRC-112A", "Special Sel A"),
    (27, "VRC-112B", "Special Sel B"),
]

def find(src, key):
    m = re.match(r"(ROM\d)([AB])$", key)
    if m:
        pat = re.compile(r"^%s%s\.syx$" % m.groups(), re.I)
    else:
        m = re.match(r"(VRC-\d+)([AB])$", key)
        pat = re.compile(r"^%s\b.*\(%s\)\.syx$" % m.groups(), re.I)
    hits = [os.path.join(d, f) for d, _, fs in os.walk(src) for f in fs if pat.match(f)]
    if len(hits) != 1:
        sys.exit("expected one file for %s in %s, found %r" % (key, src, hits))
    return hits[0]

def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    src, out = sys.argv[1:]
    os.makedirs(out, exist_ok=True)
    for n, key, name in BANKS:
        path = find(src, key)
        if os.path.getsize(path) != 4104:
            sys.exit("%s is not 4104 bytes" % path)
        shutil.copyfile(path, os.path.join(out, "%06d_%s.syx" % (n, name)))
    print("wrote %d banks to %s" % (len(BANKS), out))

if __name__ == "__main__":
    main()
