# ZicDexed firmware UI: how it is built

The screens in [ui-design.md](ui-design.md) run as real code. This page is for whoever adds a page or touches the input.

## Layers

| Folder | Depends on | What |
|---|---|---|
| `src/ui/` | nothing (plain C++14) | `Gfx` (320x170 RGB565 framebuffer, anti-aliased shapes, Inter bitmap text, dirty rectangles), `Ui` (lanes, key band, Shift map, input routing, page switching), the pages, the `ISynth` interface |
| `src/glue/` | Circle + MiniDexed | `CZicUI` (12 keys, 4 encoders, panel flush), `CZicSynth` (`ISynth` on top of `CMiniDexed`), the audio meter tap |
| `patches/minidexed/` | | Small hooks: ini keys, serial MIDI switch, UI hand-over, meter call |
| `tools/sim/`, `tests/ui/` | a PC | Runs the real `src/ui` code against a fake synth: screenshots and tests |

`build.sh` copies `src/ui`, `src/glue` and `src/logic` flat into MiniDexed's `src/`, lists the objects in `zicobjs.mk` and builds with C++17.

## Screen model

- Top 44 px: one **lane** per encoder (label, value, bar). Turning a knob lights its lane for 0.6 s.
- Middle 108 px: the page's picture. Bottom 18 px: labels for q w e r, sub-page dots, toasts.
- Everything takes the current TG's colour (`kTGColor`).
- Only changed pixels go to the panel. `Ui::tick()` runs the live parts (scope, level ticks, TG dots) at 25 fps and skips them when nothing moves, so an idle screen sends nothing. `CZicUI::Flush()` sends at most 2400 pixels per main-loop pass so MIDI and USB are never starved by SPI.

## Adding a page

1. Implement `zic::Page` (`src/ui/zicpage.h`): `lane()` gives label/text/bar for each encoder, `turn()` applies an encoder step, `draw()` paints the picture, `tick()` updates its live parts, optional `keyLabels()` and `key()`.
2. Talk to the synth only through `ISynth` (`src/ui/zicsynth.h`). Voices are the 155-byte unpacked DX7 layout; there are calls for loading and reading voices, notes, bank reading, reverb/compressor, mute and metering. The logic layer's `VoiceSink`, `ParamBus` and `BankSource` map one to one onto them.
3. Add a factory to `zicpages.h` and one line to `RegisterPages()` in `src/ui/zicregistry.cpp`. The slot decides which key reaches it (`SLOT_FM` = w, `SLOT_OPS` = e, `SLOT_MOD` = r, `SLOT_BROWSE` = d, `SLOT_KEYS` = Shift+x). An empty slot shows a "not in this build" toast.
4. Add the page to `tools/sim/sim.cpp` for a screenshot and to `tests/ui/ui_test.cpp`.

`Ctx` hands a page the `Gfx`, the `ISynth`, the `Ui` (current TG, mute/solo, smoothed level meter) and the clock. Encoder steps arrive already accelerated; with Shift held the raw step arrives and `fine` is true.

## Input

| | |
|---|---|
| Keys | `q w e r / a s d f / z x c v` on GPIO `20 16 25 14 / 12 1 24 15 / 7 8 23 22`, active low, 6 ms debounce |
| Encoders | 1..4 on GPIO `26/13, 6/5, 0/9, 27/4`, interrupt driven. Turn speed sets the step: under 12 ms per click x6, under 25 ms x3, under 60 ms x2, else x1 |
| Direction | `ZicEncoderInvert` in `minidexed.ini`: bit n reverses encoder n+1 |

Implemented keys in this build: `q` VOICE (press again for sub-pages), `f` MIX, `a`/`s` TG-/TG+, `x` PLAY (hold), `c` COMPARE (hold), `v` EXIT home (hold 0.7 s = panic), `z` SHIFT (hold 0.25 s shows the key map; `Shift+a` TG mute, `Shift+s` TG solo, `Shift+c` revert, `Shift+v` stock menu). 

In the stock menu: `v` select, `c` back, `d` home, `a`/`s` program, `q`/`w` TG, `e`/`r` bank, any encoder scrolls, `Shift+v` returns to the ZicDexed screens.

## ini keys (config/minidexed.ini)

| Key | Default | |
|---|---|---|
| `ZicUI` | 0 | 1 = ZicDexed screens, 0 = stock text menu |
| `SerialMIDIEnabled` | 1 | 0 frees GPIO 14/15 for keys r and f |
| `ZicEncoderInvert` | 0 | bit mask, see above |

With `ZicUI=1` the stock `ButtonPin*` and `EncoderEnabled` must be off (the shipped ini does that) so nothing else claims the pins.

## Checks on the hardware

1. Screen shows VOICE in the TG1 colour, not shifted or flipped (`ST7789Rotation` if so).
2. Each key does something (q f a s x c v z); z, x, r, f are the ones that share pins with SPI and UART.
3. Each encoder lights its own lane; if one runs backwards set its bit in `ZicEncoderInvert`.
4. With a USB keyboard playing: the scope moves, the TG dots light, MIX level ticks follow.
5. Hold `x` and hear a note with nothing plugged in; hold `c` and hear the stored voice, release and get your edit back.

## Sound-design pages (FM, OPS, ENV, MOD, BROWSE, KEYS)

`src/logic/` holds the lane tables and edit logic; `src/ui/soundhub.*` and `src/ui/soundpages.cpp` map them onto the framework. Screenshots: [sound_pages.png](screens/sound_pages.png), all 32 algorithms in [algorithms.png](screens/algorithms.png) (`make -C tests/ui sound_shots`).

| Key | Page |
|---|---|
| `w` | FM: Algo, Feedback, Bright, Snarl. Press again: Bite, Tail, Vel, Sync |
| `e` | OPS: Op, Level, Ratio (Shift = fine), Detune. Press again: operator envelope, rates (Shift = levels) |
| `r` | MOD: LFO (wave, speed, pitch, amp), pitch EG, FX (reverb size, damping, level, compressor) |
| `d` | BROWSE: turning Voice previews on the target TG; `c` loads, leaving the page undoes the preview |
| `Shift+x` | KEYS: `q w e r a s d f` play the current scale on the current TG |
| `Shift+q/w/e` | INIT, MUTATE, OP MUTE (selected operator) |
| `Shift+r` | FX: jumps to the MOD page's effects sub-page |
| `Shift+d` | SAVE VOICE: writes the edit voice into the user bank `sysex/voice/000099_ZICUSER.syx` (bank 99; same name = same slot, otherwise the next free one). It shows up in BROWSE after a restart |
| `Shift+f` | SAVE PERFORMANCE: overwrites the current performance, i.e. every TG's voice (edits included), levels, pans and the effects |

The hub keeps one `EditSession` per TG and re-reads the synth's live voice on every entry point, so COMPARE, revert, program changes and SysEx all show up. FM macro amounts reset to 0 when that happens. OP MUTE only silences the sound. SAVE VOICE saves the edit voice, so muted operators are saved at their real level; SAVE PERFORMANCE saves what the synth is playing, so un-mute first.
