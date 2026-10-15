# ZicDexed

MiniDexed (8 x DX7 FM synth, bare metal) for the **ZicPixel** hardware. It uses the 320x170 screen, all 4 encoders and all 12 keys, with a control layout taken from zicBox.

Hardware: Pi Zero 2 W, ST7789 1.9" 170x320 on SPI0 (DC GPIO3, RST GPIO17, no CS, SPI mode 3), PCM5102 I2S DAC, 4 encoders without push (26/13, 6/5, 0/9, 27/4), 12 keys q20 w16 e25 r14 / a12 s1 d24 f15 / z7 x8 c23 v22 (active low).

## Status

Proof of concept: stock MiniDexed UI plus a screen-offset fix, 1 encoder, 7 keys. The new UI is designed in [docs/ui-design.md](docs/ui-design.md), and the build order is listed at the end of that file.

## Layout

| Path | What |
|---|---|
| `build.sh` | Clones MiniDexed at a pinned commit, applies `patches/`, builds `out/kernel8.img` |
| `patches/circle/` | Patches to the Circle library (ST7789 170x320 row offset) |
| `patches/minidexed/` | Patches to MiniDexed (ZicDexed UI, encoders, keys) |
| `config/minidexed.ini` | Settings for the ZicPixel |
| `sysex/voice/` | Extra voice banks (`000000_nasty.syx`: 32 DnB/electro patches) |
| `tools/nasty_makebank.py` | Generator for the nasty bank |

## SD card

1. Format the card as FAT32 and copy a MiniDexed release onto it.
2. Copy the contents of `out/` (kernel8.img, minidexed.ini, sysex/) on top.

MIDI comes in over USB (USB host keyboard, or `USBGadget=1` to play from a PC), or over WiFi with AppleMIDI/RTP-MIDI once networking is turned on.

## Licence

GPL-3.0, the same as MiniDexed and zicBox.
