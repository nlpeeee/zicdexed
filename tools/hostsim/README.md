# Host simulator and QEMU smoke boot

## sim_sd: the real UI against a fake SD card (native, a few seconds)

```
git clone https://github.com/probonopd/MiniDexed ../MiniDexed && git -C ../MiniDexed checkout 8e3fa30   # the build.sh pin
make -C tools/hostsim MINIDEXED=$PWD/../MiniDexed run
```

Runs `src/ui` + `src/logic` unchanged, MiniDexed's own `CSysExFileLoader` on `sdcard/sysex/voice`, and the real `zicsave.cpp`.
It drives key and encoder sequences, asserts on the result and writes screenshots to `/tmp/zicdexed-hostsim/shots`
(override with `WORK=`). Covers: bank `000001_nasty.syx` loading as bank 1 in BROWSE (audition, LOAD, undo), Shift+d into
`000099_ZICUSER.syx` (same name replaces, new name takes the next slot, appears as bank 99 after a restart), Shift+f with
and without the card's `performance` folder, Shift+r, and the WiFi status line states.

Not covered: the Dexed engine (sound) and `CMiniDexed` itself. The ISynth here re-implements the glue's bank/save mapping.

## tools/qemu: boot kernel8.img on an emulated Pi 3B

`mksd.sh` builds an MBR + FAT32 card image, `patchk.py` makes a test copy of `kernel8.img` that skips the SPI0 and I2C1 wait
loops (QEMU has no model of either, so the stock driver spins forever), `run.sh` boots it and screenshots Circle's
framebuffer log. QEMU reaches `ui: LCD initialized`, `zicui: ZicDexed UI started`, the bank load and the performance load.
The patch addresses are for build 3 only. The panel itself and audio output are not emulated.
