# src/logic

Pure C++17 logic for the sound-design pages (FM, OPS, ENV, MOD, BROWSE, KEYS, SHIFT map) and the PLAY / COMPARE / MUTATE actions. It has no Circle or MiniDexed dependency, no exceptions and no heap use, so it builds on a PC (`make -C tests test`) and in the kernel.

| File | What |
|---|---|
| `dx7_voice` | 155-byte DX7 voice layout, parameter ranges, algorithm carrier/feedback table, ratio maths |
| `macros` | Bright, Snarl, Bite, Tail, Vel. Recomputed from a baseline, so returning a macro to 0 restores the voice exactly |
| `mutate` | MUTATE: seeded random nudge of modulator ratios/levels/envelopes, feedback and detune. Carriers stay audible and in tune |
| `edit_session` | Stored vs edit voice, COMPARE, REVERT, INIT, MUTATE, OP MUTE, PLAY |
| `page_model` | Lane tables (label, value text, bar fraction, encoder turn) for every page |
| `keys_model` | KEYS page scales and held-note tracking |
| `browse_model` | BROWSE list window, live preview, load/cancel |
| `alg_layout` | Box positions for drawing any of the 32 algorithms |
| `shift_map` | Key map for the SHIFT screen and the action behind each Shift+key |

## What the framework provides

- `VoiceSink` (apply a voice to the TG engine, note on/off on the TG channel).
- `ParamBus` (reverb, compressor and TG volume, which are not voice parameters).
- `BankSource` (bank and voice names, voice reads) for BROWSE.
- A `UiCtx` per screen: current TG, selected operator, whether Shift is held.

Per encoder turn the framework calls `AdjustLane(GetPage(id), lane, ctx, delta)` with the accelerated delta and redraws the lane with `ReadLane`.

## Behaviour to know

- Editing a non-macro FM parameter (Algo, Feedback, any OPS/ENV value) bakes the current macro result into the voice and resets the macros to 0. Sync does not bake.
- While COMPARE is held, edits and actions are ignored, as on the DX7.
- ENV and PITCH EG lanes show rates; with Shift held they show and edit levels.
- Ratio lane edits coarse; with Shift it edits fine. Algorithm 32 has no modulators, so Bright/Snarl/Bite only affect feedback there.
- The algorithm routing table is the Dexed/MSFA engine table (Synth_Dexed `fm_core.cpp`), so carriers, modulators and feedback match what the synth plays. `alg_layout` turns it into box positions for the FM picture (`docs/screens/algorithms.png` shows all 32).
