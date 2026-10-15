#!/bin/bash
# Build the ZicDexed kernel8.img (Pi Zero 2 W / Pi 3, 64-bit).
# Needs the ARM GNU toolchain 13.3.rel1 aarch64-none-elf on PATH, e.g.
#   export PATH=$PWD/toolchain/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf/bin:$PATH
set -euo pipefail

MINIDEXED_REPO=https://github.com/probonopd/MiniDexed.git
MINIDEXED_REF=8e3fa301ddcd2c0e3b9c1023148df2302095b07b

ROOT=$(cd "$(dirname "$0")" && pwd)
SRC=$ROOT/build/MiniDexed
OUT=$ROOT/out

command -v aarch64-none-elf-gcc >/dev/null || { echo "aarch64-none-elf-gcc not on PATH"; exit 1; }

if [ ! -d "$SRC/.git" ]; then
	git clone "$MINIDEXED_REPO" "$SRC"
fi
cd "$SRC"
git checkout -f "$MINIDEXED_REF"
./submod.sh

# Patches to MiniDexed itself, then to the Circle library it bundles
for p in "$ROOT"/patches/minidexed/*.patch; do
	[ -e "$p" ] && git apply --verbose "$p"
done
for p in "$ROOT"/patches/circle/*.patch; do
	[ -e "$p" ] && git -C circle-stdlib/libs/circle apply --verbose "$p"
done

RPI=3 ./build.sh

rm -rf "$OUT"; mkdir -p "$OUT"
cp src/kernel8.img "$OUT"/
cp "$ROOT"/config/minidexed.ini "$OUT"/
cp -r "$ROOT"/sysex "$OUT"/
echo "SD card overlay ready in $OUT"
