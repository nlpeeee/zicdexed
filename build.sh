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
# Synth_Dexed is on codeberg; set SYNTH_DEXED_URL=https://github.com/probonopd/Synth_Dexed.git
# where codeberg is not reachable.
git submodule init Synth_Dexed
[ -n "${SYNTH_DEXED_URL:-}" ] && git config submodule.Synth_Dexed.url "$SYNTH_DEXED_URL"
./submod.sh

# The newlib build wants makeinfo for docs it never ships; a stub is enough.
if ! command -v makeinfo >/dev/null; then
	mkdir -p "$ROOT/build/shim"
	printf '#!/bin/bash\nwhile [ $# -gt 0 ]; do [ "$1" = -o ] && : > "$2"; shift; done\nexit 0\n' > "$ROOT/build/shim/makeinfo"
	chmod +x "$ROOT/build/shim/makeinfo"
	export PATH="$ROOT/build/shim:$PATH"
fi

# Patches to MiniDexed itself, then to the Circle library it bundles
for p in "$ROOT"/patches/minidexed/*.patch; do
	[ -e "$p" ] && git apply --verbose "$p"
done
for p in "$ROOT"/patches/circle/*.patch; do
	[ -e "$p" ] && git -C circle-stdlib/libs/circle apply --verbose "$p"
done

# ZicDexed sources: the screen framework (src/ui), the Circle/MiniDexed glue
# (src/glue) and, when present, the pure logic layer (src/logic). They are
# copied flat into MiniDexed's src/ and listed in zicobjs.mk, which the patched
# Makefile includes. C++17 for the logic layer.
ZSRC=$SRC/src
ZIC_OBJS=""
for d in ui glue logic; do
	[ -d "$ROOT/src/$d" ] || continue
	cp "$ROOT"/src/$d/*.h "$ROOT"/src/$d/*.cpp "$ZSRC"/ 2>/dev/null || true
	for f in "$ROOT"/src/$d/*.cpp; do
		[ -e "$f" ] && ZIC_OBJS="$ZIC_OBJS $(basename "${f%.cpp}").o"
	done
done
printf 'ZIC_OBJS =%s\nSTANDARD = -std=c++17 -Wno-aligned-new\n' "$ZIC_OBJS" > "$ZSRC/zicobjs.mk"

if [ "${ZIC_QUICK:-0}" = 1 ]; then
	# libraries were built by an earlier run: only rebuild MiniDexed itself
	make -C "$ZSRC" RPI=3 -j"$(nproc)"
else
	RPI=3 ./build.sh
fi

# an empty performance folder: Shift+f (SAVE PERFORMANCE) is refused without it on the card
rm -rf "$OUT"; mkdir -p "$OUT" "$OUT"/performance
cp src/kernel8.img "$OUT"/
cp "$ROOT"/config/minidexed.ini "$OUT"/
cp -r "$ROOT"/sysex "$OUT"/
echo "SD card overlay ready in $OUT"
