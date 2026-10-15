# ZicDexed UI/UX design (v2)

v2 takes artistic license. Nothing here is tied to zicBox's look or code. The mockups are in `mockups/`: [contact_sheet.png](mockups/contact_sheet.png) shows all pages, and [device_voice.png](mockups/device_voice.png) shows the screen in a rough outline of the box. Both are rendered at the real 320x170 by `tools/mockups/render_mockups.py` in the zicdexed repo.

## 1. Layout follows the hardware

The 4 encoders sit **above** the screen and the 12 keys sit **below** it. So the screen is split the same way:

| Band | Pixels | What |
|---|---|---|
| Encoder lanes | top 44 px | One lane per knob, directly under it: label, big value, value bar (bipolar ones grow from the centre). Turning a knob lights its lane up solid for a moment, which is the "you touched this" feedback |
| Picture | middle ~106 px | Each page's hero visual: voice name with a live scope, algorithm, operators, envelope, LFO, mixer strips |
| Key labels | bottom 16 px | Labels for the top key row (q w e r), the row nearest the screen. Dots show sub-pages |

## 2. Colour and type

- **Colour:** near-black background, with one hot colour per tone generator (TG1 acid green, TG2 orange, TG3 cyan, TG4 pink, and so on). The whole screen takes the current TG's colour, so you always know which TG you're editing without reading anything.
- **Type:** Inter, under the SIL Open Font License. Display Black for the voice name, Bold for values, and spaced small caps for labels. The firmware gets bitmaps pre-rendered from the same fonts, so the screen matches the mockups.
- **Grey levels:** grey is used only for labels and inactive items. Values are always white.

## 3. Key map

```
 q VOICE    w FM       e OPS       r MOD        ← direct page keys; press again for sub-page
 a < TG     s TG >     d BROWSE    f MIX
 z SHIFT    x PLAY     c COMPARE   v EXIT
```

**With Shift held,** the screen turns into a live map of these (mockup 09):

```
 q INIT     w MUTATE   e OP MUTE   r FX
 a TG MUTE  s TG SOLO  d SAVE VCE  f SAVE PRF
 z (held)   x KEYS     c REVERT    v MENU
```

- **Page keys** jump straight to a page, so there's no cycling. Pressing a page key again moves through its sub-pages:
  - VOICE: tone, play, mix
  - OPS: operators, envelope
  - MOD: LFO, pitch EG, FX
- **PLAY:** hold to sound a note on the current TG. You can sound-design with nothing plugged in.
- **COMPARE:** hold to hear the stored voice, release to get your edit back, as on the original DX7. Shift+COMPARE reverts the edit.
- **MUTATE:** each press nudges the current voice at random (ratios, levels, feedback, detune). It's a quick route to nasty sounds, and COMPARE/REVERT make it safe to use.
- **INIT:** loads a basic starting voice.
- **Shift + encoder:** fine steps.
- **Fast turns** accelerate. That matters for the DX7's 0-99 ranges on encoders with no push switch.
- **Long-press v:** panic (all notes off).

## 4. Pages (mockups 01-09)

| Page | Lanes (Enc1-4) | Picture |
|---|---|---|
| 01 VOICE | Cutoff, Reso, Detune, Volume. Sub-pages: Mono/Porta/PB/Transpose, then Pan/Reverb/Channel/Program | Voice name in large type, bank/program, play mode, live output scope, TG activity dots |
| 02 FM | Algo, Feedback, Bright, Snarl (macros). Sub-page: Bite, Tail, Vel, Sync | Algorithm diagram. Operator boxes fill with their level. The feedback loop is highlighted. 7-segment feedback meter |
| 03 OPS | Op, Level, Ratio, Detune | 6 operator cards: ratio, level bar, carriers marked OUT, selected card outlined |
| 04 OPS → envelope | Rate 1-4 (Shift: Level 1-4) | The selected operator's envelope drawn live |
| 05 MOD | Wave, Speed, Pitch depth, Amp depth | LFO waveform with a moving phase line. Sub-pages: pitch EG, then FX (reverb, compressor, GRIT later) |
| 06 MIX | TG, Volume, Pan, Reverb | 8 channel strips in their TG colours, with live level ticks and mute state. q/w mute/solo here |
| 07 BROWSE | Voice, Bank, Target TG, Volume | List. Turning the knob previews each voice live. c loads, v cancels |
| 08 KEYS | Octave, Scale, Root, Velocity | The 8 upper keys become a playable scale |
| 09 SHIFT | (encoders give fine steps) | Key map of the Shift functions |

The **macros** (Bright, Snarl, Bite, Tail) write ordinary DX7 voice parameters, so anything you make saves as a normal voice and loads on any DX7/Dexed.

## 5. Drawing and performance

- Drawing goes straight to Circle's ST7789 driver with dirty-rectangle updates. A full frame is about 60 ms at 15 MHz, so we avoid full redraws.
- Lanes update on change, about 3 KB each.
- The scope and LFO animate in a small window at around 25 fps.
- The UI runs on core 0 and audio on the others.

## 6. Wireless MIDI

Unchanged from v1:
- Circle has no Bluetooth support, so native Bluetooth MIDI is parked.
- WiFi AppleMIDI/RTP-MIDI is already built into MiniDexed.
- A class-compliant USB-to-BLE MIDI dongle should work with no code. That's inferred, not tested.

## 7. Build order

1. **Hardware:** all 12 keys and all 4 encoders with acceleration.
2. **UI framework:** lanes, key-label band, the VOICE and MIX pages, and COMPARE/PLAY. This is the first flashable build.
3. **Sound-design pages:** FM, OPS and envelope, plus the macros and MUTATE.
4. **BROWSE, KEYS and the Shift map.** The stock menu stays as a fallback under Shift+v.
5. **MOD page,** then the optional GRIT effect.
