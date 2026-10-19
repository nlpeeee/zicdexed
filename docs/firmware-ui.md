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
- Middle 108 px: the page's picture. Bottom 18 px: labels for keys 1 to 4, sub-page dots, toasts.
- Everything takes the current TG's colour (`kTGColor`).
- Only changed pixels go to the panel. `Ui::tick()` runs the live parts (scope, level ticks, TG dots) at 25 fps and skips them when nothing moves, so an idle screen sends nothing. `CZicUI::Flush()` sends at most 2400 pixels per main-loop pass so MIDI and USB are never starved by SPI.

## Adding a page

1. Implement `zic::Page` (`src/ui/zicpage.h`): `lane()` gives label/text/bar for each encoder, `turn()` applies an encoder step, `draw()` paints the picture, `tick()` updates its live parts, optional `keyLabels()` and `key()`.
2. Talk to the synth only through `ISynth` (`src/ui/zicsynth.h`). Voices are the 155-byte unpacked DX7 layout; there are calls for loading and reading voices, notes, bank reading, reverb/compressor, mute and metering. The logic layer's `VoiceSink`, `ParamBus` and `BankSource` map one to one onto them.
3. Add a factory to `zicpages.h` and one line to `RegisterPages()` in `src/ui/zicregistry.cpp`. The slot decides which key reaches it (`SLOT_FM` = key 2, `SLOT_OPS` = key 3, `SLOT_MOD` = key 4, `SLOT_BROWSE` = key 7, `SLOT_KEYS` = Shift+key 10). An empty slot shows a "not in this build" toast.
4. Add the page to `tools/sim/sim.cpp` for a screenshot and to `tests/ui/ui_test.cpp`.

`Ctx` hands a page the `Gfx`, the `ISynth`, the `Ui` (current TG, mute/solo, smoothed level meter) and the clock. Encoder steps arrive already accelerated; with Shift held the raw step arrives and `fine` is true.

## Input

| | |
|---|---|
| Keys | Keys 1 to 12, left to right, top to bottom: `1 2 3 4 / 5 6 7 8 / 9 10 11 12` on GPIO `20 16 25 14 / 12 1 24 15 / 7 8 23 22`, active low, 6 ms debounce (in the source they are still called q w e r / a s d f / z x c v) |
| Encoders | 1..4 on GPIO `26/13, 6/5, 0/9, 27/4`, interrupt driven. Turn speed sets the step: under 12 ms per click x6, under 25 ms x3, under 60 ms x2, else x1 |
| Direction | `ZicEncoderInvert` in `minidexed.ini`: bit n reverses encoder n+1 |

Implemented keys in this build: key 1 VOICE (press again for sub-pages), key 8 MIX, keys 5/6 TG-/TG+, key 10 PLAY (hold), key 11 COMPARE (hold), key 12 EXIT home (hold 0.7 s = panic), key 9 SHIFT (hold 0.25 s shows the key map; Shift+5 TG mute, Shift+6 TG solo, Shift+11 revert, Shift+12 stock menu). The other keys show a "not in this build" toast until their pages land.

In the stock menu: key 12 select, key 11 back, key 7 home, keys 5/6 program, keys 1/2 TG, keys 3/4 bank, any encoder scrolls, Shift+12 returns to the ZicDexed screens.

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
5. Hold key 10 and hear a note with nothing plugged in; hold key 11 and hear the stored voice, release and get your edit back.
