# PC simulator (`tools/hostsim`)

Runs the real firmware UI (`src/ui` and `src/logic`, unchanged) on a PC against a fake SD card folder, so a screen, a key sequence or SAVE VOICE can be checked without the hardware. Bank loading is MiniDexed's own `CSysExFileLoader`, and SAVE VOICE is the real `src/glue/zicsave.cpp`.

It does not run the Dexed engine (no sound) or `CMiniDexed`; `tools/hostsim/sdsynth.h` re-implements the glue's bank, save and WiFi-status mapping on top of the test synth. The older `tools/sim` and `tests/` cover the UI against a pure fake synth.

## Build and run

Needs g++ (C++17), git, make and python3 with Pillow. The first build clones MiniDexed at the `build.sh` pin into `build/MiniDexed` for its `sysexfileloader.cpp`; pass `MINIDEXED=/path/to/checkout` to reuse one.

```
make -C tools/hostsim run                       # built-in checks (25) plus screenshots
make -C tools/hostsim script SCRIPT=my.txt      # play your own script
```

Output goes to `/tmp/zicdexed-hostsim` (change with `WORK=`): `sdcard/` is the fake card (a copy of `sysex/voice/000090_nasty.syx`, plus whatever the run saves) and `shots/` holds a PNG per screenshot, `name.png` at 1x and `name@3x.png`.

## Scripts

One command per line, `#` starts a comment. Keys are numbered 1 to 12 in the order of the device: `q w e r` = 1 to 4, `a s d f` = 5 to 8, `z x c v` = 9 to 12, so **Shift is key 9**. Encoders are 1 to 4.

| Command | |
|---|---|
| `press N` | tap key N |
| `down N` / `up N` | hold and release key N (hold 9 for 0.25 s and the Shift key map appears) |
| `shift N` | Shift+N (Shift+7 = SAVE VOICE, Shift+8 = SAVE PERFORMANCE, Shift+4 = FX) |
| `turn E STEPS` | turn encoder E by STEPS clicks (negative = counter-clockwise) |
| `wait MS` | let the UI run for MS milliseconds |
| `shot NAME` | write `shots/NAME.png` |
| `print` | list each TG's voice name, bank and program |
| `reboot` | power-cycle: reload the card (a saved voice appears as bank 99 afterwards) |
| `mkdir DIR` | create a folder on the card, e.g. `mkdir performance` (SAVE PERFORMANCE is refused without it) |
| `net TEXT` | set the WiFi status line, e.g. `net WiFi 192.168.1.42` |

`tools/hostsim/scripts/save-and-browse.txt` is a worked example.

## What the built-in checks cover

The nasty bank loading with the right names, BROWSE audition, LOAD and undo, SAVE VOICE (new file, same name replaces, new name takes the next slot, bank 99 after a reboot), SAVE PERFORMANCE with and without the `performance` folder, Shift+4 FX and the WiFi status line. Address sanitizer is on, so memory errors in the UI fail the run.
