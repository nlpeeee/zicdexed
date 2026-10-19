# ZicDexed

MiniDexed (8 x DX7 FM synth, bare metal) for the **ZicPixel** hardware. It uses the 320x170 screen, all 4 encoders and all 12 keys, with a control layout taken from zicBox.

Hardware: Pi Zero 2 W, ST7789 1.9" 170x320 on SPI0 (DC GPIO3, RST GPIO17, no CS, SPI mode 3), PCM5102 I2S DAC, 4 encoders without push (26/13, 6/5, 0/9, 27/4), 12 keys, numbered 1-12 from top left to bottom right, on GPIO 20 16 25 14 / 12 1 24 15 / 7 8 23 22 (active low).

## Status

The full UI runs on the hardware: the screen framework (lanes, key band, per-TG colour, Shift key map), all 12 keys and 4 encoders with acceleration, the VOICE, FM, OPS (with envelopes), MOD (LFO, pitch EG, FX), MIX, BROWSE and KEYS pages, PLAY and COMPARE, INIT, MUTATE, OP MUTE, SAVE VOICE and SAVE PERFORMANCE, and WiFi with FTP updates. The stock menu is under Shift+key 12. The design is in [docs/ui-design.md](docs/ui-design.md), how it is built is in [docs/firmware-ui.md](docs/firmware-ui.md).

![simulated screens](docs/screens/simulated_contact_sheet.png)

*Rendered on a PC by the same `src/ui` code the kernel runs (`make -C tests/ui shots`).*

## Layout

| Path | What |
|---|---|
| `build.sh` | Clones MiniDexed at a pinned commit, applies `patches/`, adds `src/`, builds `out/kernel8.img` |
| `src/ui/` | Screen framework, pages, graphics and bitmap fonts. No Circle dependency |
| `src/glue/` | Keys, encoders, panel flush and the MiniDexed adaptor |
| `patches/circle/` | Circle patch (ST7789 170x320 row offset) |
| `patches/minidexed/` | MiniDexed hooks (ini keys, serial MIDI switch, UI hand-over, meters) |
| `config/minidexed.ini` | Settings for the ZicPixel |
| `sysex/voice/` | Extra voice banks (`000090_nasty.syx`: 32 DnB/electro patches). The stock Yamaha banks are not shipped: `python3 tools/stock_banks.py <Dexed cart dir> <out>` builds them as banks 1-27 with themed names. Bank 1 shown on the device is the firmware's built-in set only while no `000001_*.syx` is on the card |
| `tools/` | `fontgen.py` (Inter to bitmaps), `mockups/` (design renders), `sim/` (PC simulator), `hostsim/` ([UI on a fake SD card](docs/simulator.md)), `nasty_makebank.py`, `stock_banks.py` |
| `tests/ui/` | Host tests: `make -C tests/ui test` |

## Building

Needs the Arm GNU toolchain 13.3.rel1 `aarch64-none-elf` on `PATH`, then `./build.sh`. Set `SYNTH_DEXED_URL=https://github.com/probonopd/Synth_Dexed.git` if codeberg is unreachable. After a first full build, `ZIC_QUICK=1 ./build.sh` rebuilds only MiniDexed and the ZicDexed sources.

## SD card

1. Format the card as FAT32 and copy a MiniDexed release onto it.
2. Copy `kernel8.img` and `minidexed.ini` from `out/` onto the card root. This replaces the two files with the same name, which is what you want.
3. **Merge** `out/sysex/voice/` into the card's `sysex/voice/`. Never replace the folder: the stock banks live there. MiniDexed numbers banks from the 6-digit prefix of the file name, so a file with the same number as a stock bank hides it. The nasty bank uses 000090 for that reason, and SAVE VOICE writes bank 99.
4. If the stock banks have been lost or overwritten, rebuild them from your Dexed cartridge collection: `python3 tools/stock_banks.py <Dexed cart folder> <out folder>` writes 000001-000027 with themed names (Yamaha ROM3/ROM4 and VRC-101 to VRC-112). Copy them into `sysex/voice/`. The Yamaha banks are not stored in this repo.
5. Copy the empty `performance` folder from `out/` to the card root. SAVE PERFORMANCE needs it.
6. For WiFi, add a `firmware` folder and `wpa_supplicant.conf` as described in [docs/wifi.md](docs/wifi.md). Without them, set `NetworkEnabled=0` in `minidexed.ini`.

MIDI comes in over USB (USB host keyboard, or `USBGadget=1` to play from a PC), or over WiFi with AppleMIDI/RTP-MIDI once networking is turned on. Setup and firmware updates over WiFi are in [docs/wifi.md](docs/wifi.md).

## Licence

GPL-3.0, the same as MiniDexed and zicBox.
