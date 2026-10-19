# ZicDexed

**An 8-voice FM monster in a box with 12 keys and 4 knobs.**

ZicDexed is [MiniDexed](https://github.com/probonopd/MiniDexed) (8 x DX7 FM tone generators, bare metal, no OS) rebuilt around the **ZicPixel**: a Pi Zero 2 W, a 320x170 colour screen, four encoders and twelve keys. No mouse, no menu diving. Every knob has a label right under it, and the whole screen takes the colour of the voice you are editing.

![ZicDexed screens, rendered by the firmware's own UI code](docs/screens/emulator_contact_sheet.png)

## Why you'll like it

- **Four lanes, four knobs.** The top of the screen is one lane per encoder, with a big value and a bar. Turn a knob and its lane lights up.
- **Eight tone generators, eight colours.** Lime, orange, sky, pink, yellow, violet, mint, red. You always know which one you are on.
- **Sound-design keys that are fun to abuse.** MUTATE nudges a voice at random, INIT gives a clean start, COMPARE (hold) shows you what you started with, REVERT takes you back. Hold PLAY to hear a note with nothing plugged in.
- **Macros for nasty sounds.** Bright, Snarl, Bite, Tail and Vel move several DX7 parameters at once, and the result still saves as a normal voice.
- **Browse by ear.** Every step through the voice list previews on the spot. Leave without LOAD and your old voice comes back.
- **Safe saves.** SAVE VOICE has a naming screen, writes through a temporary file and keeps a `.bak`. SAVE PERFORMANCE refuses to save while an operator is muted.
- **WiFi updates.** Push firmware, config and banks over FTP. WiFi MIDI too (AppleMIDI), since Circle has no Bluetooth.
- **Ships with 32 nasty DnB/electro voices** (bank 90): REESE BS, WOBBLE BS, NEURO STB, FM KICK, METAL SNR, HOOVER and friends.

## The box

Keys are numbered 1 to 12, top left to bottom right. Hold **9 (Shift)** and the screen turns into a live map of what the other keys do.

![key map](docs/screens/keymap.png)

| Key | Does | With Shift |
|---|---|---|
| 1 | VOICE (press again: more sub-pages) | INIT |
| 2 | FM macros and algorithm | MUTATE |
| 3 | OPS, then envelopes | OP MUTE |
| 4 | MOD: LFO, pitch EG, FX | FX |
| 5 / 6 | previous / next TG | TG MUTE / TG SOLO |
| 7 | BROWSE | SAVE VOICE |
| 8 | MIX | SAVE PERFORMANCE |
| 9 | Shift | |
| 10 | PLAY (hold) | KEYS: keys 1-8 become scale pads |
| 11 | COMPARE (hold) | REVERT |
| 12 | EXIT (hold = panic) | stock MiniDexed menu |

Encoders speed up when you spin them. Hold Shift while turning for single steps.

## A look around

| | |
|---|---|
| ![voice](docs/screens/emu_voice.png) **VOICE**: name, bank, scope, per-TG settings | ![operators](docs/screens/emu_ops.png) **OPS**: six operator cards, carriers marked OUT |
| ![browse](docs/screens/emu_browse.png) **BROWSE**: scroll to preview, key 11 loads | ![mix](docs/screens/emu_mix.png) **MIX**: eight strips, mute and solo |

![sound pages](docs/screens/sound_pages.png)

The screens above are not mock-ups: they are rendered on a PC by the same `src/ui` code the kernel runs (`make -C tests/ui shots`). The design notes are in [docs/ui-design.md](docs/ui-design.md), and how it is built is in [docs/firmware-ui.md](docs/firmware-ui.md).

## Hardware

Pi Zero 2 W, ST7789 1.9" 170x320 on SPI0 (DC GPIO3, RST GPIO17, no CS, SPI mode 3), PCM5102 I2S DAC, 4 encoders without push (26/13, 6/5, 0/9, 27/4), 12 keys on GPIO 20 16 25 14 / 12 1 24 15 / 7 8 23 22 (active low). MIDI comes in over USB: a USB host keyboard, `USBGadget=1` to play from a PC, or AppleMIDI over WiFi. There is no DIN/serial MIDI on this build.

## Install a release on an SD card

Use this if you just want to play. You need an SD card (FAT32), a card reader and a PC. No build tools.

**1. Make a card that boots a Pi.** ZicDexed is an overlay on a [MiniDexed release](https://github.com/probonopd/MiniDexed/releases). Format the card as FAT32, then unzip the latest MiniDexed release onto its root. That gives you the Raspberry Pi boot files (`bootcode.bin`, `start*.elf`, `config.txt` and so on), a `firmware` folder with the WiFi chip firmware, and the stock `performance` folder. It has **no** `sysex/voice` folder: see step 5. Step 3 replaces MiniDexed's own `kernel8.img`, `minidexed.ini` and `performance.ini` with the ZicDexed ones (`kernel8.img` is the build for the Pi Zero 2 W).

**2. Download the ZicDexed card package.** On the [Releases page](https://github.com/nlpeeee/zicdexed/releases) get the newest `ZicDexed_*_card.zip` and unzip it. Inside is one folder, `ZicDexed_<version>/`, and what you copy is its **contents**, not the folder itself.

**3. Copy it onto the card root.**

| From the package | To the card | How |
|---|---|---|
| `kernel8.img`, `minidexed.ini`, `performance.ini` | root | Replace the files with the same name |
| `sysex/voice/000090_nasty.syx` | `sysex/voice/` | **Merge**: add the file, never replace the folder |
| `performance/` | `performance/` | **Merge**: add files, never replace the folder or overwrite performances you saved |
| `wpa_supplicant.conf.example` | root | Only for WiFi, see below |

The card must end up with a `performance` folder, or Shift+8 (SAVE PERFORMANCE) says "NO PERFORMANCE FOLDER ON SD". Copy without overwriting: on Windows `robocopy performance X:\performance /E /XC /XN /XO` (`X:` is the card), on Linux or macOS `cp -rn performance/. /path/to/card/performance/`. Any file manager's "skip existing" option does the same.

Why merge: MiniDexed numbers banks from the 6-digit start of the file name, so a file with the same number hides the old one. The nasty bank is 000090 on purpose, and SAVE VOICE writes your own voices to bank 99 (`000099_ZICUSER.syx`), which a replaced folder would delete.

**4. Optional: WiFi (FTP updates, AppleMIDI).** If you unzipped the MiniDexed release in step 1, its `firmware` folder is already on the card. The ZicDexed package does not include the WiFi chip firmware, so otherwise:
1. Get it: run `tools/get-wifi-firmware.ps1` in PowerShell (Windows) or `tools/get-wifi-firmware.sh` (Linux, macOS) from a clone of this repo, or take `out/firmware/` from a source build. Copy the resulting `firmware` folder to the card root.
2. Copy `wpa_supplicant.conf.example` to `wpa_supplicant.conf` on the card, and set `country`, `ssid` and `psk`. Keep only those lines and the `network={}` block.
3. No WiFi wanted? Set `NetworkEnabled=0` in `minidexed.ini` on the card. With WiFi on and no `firmware` folder the box can stop at the first screen.

More in [docs/wifi.md](docs/wifi.md).

**5. Factory Yamaha banks.** The Yamaha cartridges are not redistributed here. The MiniDexed release does not ship `sysex/voice` (its `getsysex.sh` script downloads the Dexed banks); the ZicDexed package adds the nasty bank. If you own the Dexed cartridge collection, `python3 tools/stock_banks.py <Dexed cart folder> <out folder>` writes 27 themed banks, `000001` to `000027`, to merge into `sysex/voice/`. Bank 1 on the box is the firmware's built-in set only while no `000001_*.syx` is on the card.

**6. Power up.** Put the card in the Pi and switch on. You land on VOICE for TG 1. Hold key 10 to hear a note. After uploading banks over FTP or by hand, reboot: MiniDexed reads bank files only at start-up. The [walkthrough](docs/walkthrough.md) takes you through the rest.

MIDI comes in over USB: a USB host keyboard, or `USBGadget=1` in `minidexed.ini` to play from a PC.

## Build from source

Use this to change the firmware. It runs on Linux (or WSL on Windows) and needs a network connection and some patience: the first build compiles the Circle libraries.

**Prerequisites:** `bash`, `git`, `make`, `gcc` and `python3`, plus the **Arm GNU toolchain 13.3.rel1, `aarch64-none-elf`** (from [developer.arm.com](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads), the "AArch64 bare-metal target (aarch64-none-elf)" download for your host). Unpack it and put its `bin` on your `PATH`:

```
export PATH=$HOME/arm-gnu-toolchain-13.3.rel1-x86_64-aarch64-none-elf/bin:$PATH
aarch64-none-elf-gcc --version     # must print 13.3.rel1
```

**Build:**

```
git clone https://github.com/nlpeeee/zicdexed
cd zicdexed
./build.sh
```

`build.sh` clones MiniDexed at a pinned commit into `build/`, applies the patches in `patches/`, adds the code in `src/` and builds. It also downloads the WiFi firmware. When it finishes, `out/` is the SD card overlay:

| In `out/` | What |
|---|---|
| `kernel8.img` | The firmware |
| `minidexed.ini`, `performance.ini` | Settings and the default performance (from `config/`) |
| `sysex/voice/` | The nasty bank |
| `performance/` | The stock MiniDexed performance banks |
| `firmware/` | WiFi chip firmware |
| `wpa_supplicant.conf.example` | WiFi network template |

Copy `out/` onto the card exactly as in steps 3 and 4 above, with `out/` standing in for the unzipped package.

**Options and tips**
- `ZIC_QUICK=1 ./build.sh` rebuilds only MiniDexed and the ZicDexed sources. Use it after a first full build.
- `WIFI_FIRMWARE=0 ./build.sh` skips the firmware download (offline builds).
- `SYNTH_DEXED_URL=https://github.com/probonopd/Synth_Dexed.git ./build.sh` if codeberg.org is not reachable.
- Without a network, or to check the UI only: `make -C tests/ui test` runs the host tests, and [docs/simulator.md](docs/simulator.md) runs the real UI on a PC with a fake card.
- With the box on WiFi, [docs/wifi.md](docs/wifi.md) shows how to push a new `kernel8.img` over FTP without taking the card out.

## Repo layout

| Path | What |
|---|---|
| `build.sh` | Clones MiniDexed at a pinned commit, applies `patches/`, adds `src/`, builds `out/kernel8.img` |
| `src/ui/` | Screen framework, pages, graphics and bitmap fonts. No Circle dependency |
| `src/glue/` | Keys, encoders, panel flush and the MiniDexed adaptor |
| `patches/circle/` | Circle patch (ST7789 170x320 row offset) |
| `patches/minidexed/` | MiniDexed hooks (ini keys, serial MIDI switch, UI hand-over, meters) |
| `config/minidexed.ini` | Settings for the ZicPixel |
| `performance/` | The stock MiniDexed performance banks (GPL-3.0, see [performance/README.md](performance/README.md)), copied to `out/performance/` |
| `sysex/voice/` | Extra voice banks (`000090_nasty.syx`: 32 DnB/electro patches). The stock Yamaha banks are not shipped: `python3 tools/stock_banks.py <Dexed cart dir> <out>` builds them as banks 1-27 with themed names. Bank 1 shown on the device is the firmware's built-in set only while no `000001_*.syx` is on the card |
| `tools/` | `get-wifi-firmware.ps1` / `.sh` (WiFi firmware download), `fontgen.py` (Inter to bitmaps), `mockups/` (design renders), `sim/` (PC simulator), `hostsim/` ([UI on a fake SD card](docs/simulator.md)), `nasty_makebank.py`, `stock_banks.py` |
| `tests/ui/` | Host tests: `make -C tests/ui test` |

## Licence

GPL-3.0, the same as MiniDexed and zicBox.
