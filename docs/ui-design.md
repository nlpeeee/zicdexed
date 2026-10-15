# ZicDexed UI/UX design

This is how MiniDexed should behave on the ZicPixel so that the 320x170 screen, all 4 encoders and all 12 keys earn their place. It replaces the "Proposed controls" section of `interface-plan.md`.

## 1. Guiding idea: make it feel like zicBox

The ZicPixel already has a well-tested control language in zicBox (`zp/config/pixel/ui`). ZicDexed should reuse it rather than invent a new one, so the box works the same whichever firmware is on the card. These are the conventions taken from the zicBox source:

| zicBox convention (source) | ZicDexed equivalent |
|---|---|
| Top two key rows (q w e r / a s d f) select **tracks 1-8** (`components/Track.tsx`, A1..B4) | Select **TG 1-8**. MiniDexed has exactly 8 tone generators on the Zero 2 W |
| Shift + track key = **mute** that track | Shift + TG key = mute TG. Shift + double-tap = solo |
| Bottom row: C1 = play note, C2 = next page, C3 = clips, C4 = **Shift** (latching) | z = audition note, x = next page, c = browse, v = Shift |
| Shift + C1 = play/pause, Shift + C2 = menu, Shift + C3 = save all | Shift+z = panic (all notes off), Shift+x = deep menu, Shift+c = save performance |
| Each view shows **12 values in 3 rows of 4**. The active row is drawn large with a graph and the 4 encoders edit it. C2 moves the active row down (`MultiSynthLayout.tsx`, `enc1mini`..`enc12mini`) | Same grid. x steps the active row 1→2→3→next view |
| Page dots top-left, coloured header per track (`Layout.tsx`) | Page dots plus header in the TG's colour |
| Shift screen relabels the bottom row and turns the encoders into master controls (`ShiftMainLayout.tsx`) | Same. While Shift is latched the encoders become a mixer (see 4.6) |
| Keyboard view: the keys play notes (`Keyboard.tsx`) | Keys view: play the synth with no MIDI keyboard attached |

zicBox's bitmap fonts (`zp/draw/fonts`, Poppins/DejaVu headers) and MiniDexed are both GPL-3, so we can reuse the fonts directly for the same look.

## 2. Key map

```
          normal                         with Shift latched (v)
 q TG1   w TG2   e TG3   r TG4      q-f  mute TG (double-tap = solo)
 a TG5   s TG6   d TG7   f TG8
 z Play  x Page  c Browse v SHIFT   z Panic  x Menu  c Save  v (un)shift
```

- Tapping the current TG's key again cycles its views (Voice → FM → Operators → Envelope → LFO). The x key moves the row inside a view. So any parameter is two or three key presses away, with no menu diving.
- **Play (z):** hold to sound a note on the current TG. Encoder 4 sets the note while it's held. You can design sounds with nothing plugged in.
- **Browse (c):** a full-screen list of voices and performances (see 4.7).
- The bottom 16 px of the screen always shows what z/x/c/v do right now, as zicBox does in Shift mode.
- MiniDexed's own Prev/Next/Back/Select/Home buttons go away. Navigation becomes "pick a TG, pick a view, turn a knob".

## 3. Encoders

- In grid views the four encoders edit the four values in the active row. Each column is in the same position as its knob.
- **Acceleration:** slow turns step by 1 and fast spins step by up to 8. This matters for 0-99 DX7 ranges with no encoder push.
- **Fine:** holding x while turning forces steps of 1 with no acceleration.
- **Touch overlay:** turning a knob briefly shows that parameter's name and value large in the middle of the screen, then it fades. The screen stays readable at arm's length.
- **Lists** (Browse, the deep menu): Enc1 scrolls, Enc2 jumps by bank (32), Enc3 picks which TG receives the selection, and Enc4 is master volume.

## 4. Views (per TG)

Every view has the same header: TG number in its colour, the voice name, MIDI channel, and 8 small activity dots that light when a TG plays.

### 4.1 Voice (home)
| Row | Enc1 | Enc2 | Enc3 | Enc4 |
|---|---|---|---|---|
| 1 Sound | Voice | Bank | Volume | Pan |
| 2 Tone | Cutoff | Resonance | Detune | Reverb send |
| 3 Play | Poly/Mono | Portamento | Pitch-bend range | Transpose |

The large voice name is in the middle. Every parameter here already exists in MiniDexed's TG menu (`uimenu.cpp` `s_TGMenu`).

### 4.2 FM (the "nasty" page)
- The algorithm is drawn as a diagram of 6 boxes with carriers on the bottom row. Operators that are switched off are dimmed.
- Row 1: Algorithm, **Feedback**, Osc key sync, Transpose.
- Row 2: modulator macros. **Bright** offsets all modulator levels and **Snarl** detunes all modulator ratios slightly off-integer. These are the fastest route to grit.
- Row 3: **Bite** (attack rate of all operators), **Tail** (release of all operators), velocity sensitivity, mono legato.
- Macros write ordinary DX7 voice parameters (`SetVoiceParameter`), so results save as normal voices.

### 4.3 Operators (6 operators, 4 knobs)
- The top of the screen is a strip of 6 columns, one per operator, each with a level bar and its ratio (e.g. `1.00`, `3.50`). The selected operator is outlined, and Enc1 picks it in every row.
- Row 1: Op, Level, Coarse, Fine.
- Row 2: Op, Detune, Velocity sens, Amp mod sens.
- Row 3: Op, Rate scaling, Key-level depth L, Key-level depth R.
- With Shift latched, keys q, w, e, a, s, d toggle operators 1-6 on/off (the DX7 operator mute). The on-screen labels change to say so.

### 4.4 Envelope
- The selected operator's envelope is drawn as a shape. The pitch envelope is a seventh choice on Enc1.
- Row 1: Op, R1, R2, R3 (R4 sits in the next row).
- Row 2: Op, R4, L1, L2.
- Row 3: Op, L3, L4, "copy to all".
- The shape redraws live as you turn the knob.

### 4.5 LFO / Mod
- A small LFO waveform preview.
- Row 1: Wave, Speed, Delay, Sync.
- Row 2: Pitch mod depth, Amp mod depth, Pitch mod sens, Mod wheel range.
- Row 3: wheel/aftertouch/breath targets (MiniDexed's Modulation menu).

### 4.6 Mixer (Shift held or latched)
- Eight vertical strips, one per TG, showing volume and pan, with mute/solo state. The current TG is highlighted.
- Enc1: current TG volume. Enc2: current TG pan. Enc3: reverb level. Enc4: master volume.
- The TG keys act as mute buttons here, which matches zicBox's Shift behaviour.

### 4.7 Browse (c)
- Tabs at the top for Voices and Performances. A list of 8 rows, readable at this screen size.
- Turning the knob previews the voice on the current TG immediately (DX7 voice loads are instant), so you browse by ear.
- Select with c, cancel with v. A performance load asks for confirmation, because it replaces all 8 TGs.

### 4.8 Keys (Shift + Play)
- The 8 TG keys become a scale on the current TG, using the same layout idea as zicBox's Keyboard view.
- z/x shift the octave down/up. c changes the scale (chromatic, minor, Phrygian and so on). v leaves the view.
- This makes the box a standalone instrument when no keyboard is around.

### 4.9 Deep menu (Shift + x)
- The stock MiniDexed menu tree is kept as an escape hatch for everything else: MIDI settings, effects, saving and so on.
- It's drawn with the 8x16 font at 40x10, using rows 2-9 for the list, so you see a whole menu page at once instead of 4 lines.

## 5. Drawing on the screen

- Draw straight to Circle's `CST7789Display` (it has `SetArea`, `SetPixel` and `DrawText`). A small widget layer is enough: rect, text in the zicBox fonts, bar, and envelope/algorithm line drawing.
- A full frame is 109 KB, about 60 ms at 15 MHz. So we redraw only what's dirty, per widget, with knob changes limited to about 30 fps. The UI runs on core 0 while audio runs on the other cores, so screen traffic doesn't touch the sound.
- Colours: the zicBox track colours (`ColorTrack1..8`) for TG1-8, on a black background.
- Before any firmware work, every view is rendered here to PNG at 320x170 from the same layout code, for you to review.

## 6. Wireless MIDI (instead of Bluetooth)

- **Native Bluetooth:** Circle, the bare-metal library MiniDexed runs on, lists Bluetooth as "Not supported" (checked in its README). Bluetooth MIDI would mean writing a BLE stack from scratch. Parked.
- **Network MIDI over WiFi works today.** MiniDexed has AppleMIDI/RTP-MIDI and UDP MIDI built in (`src/net/applemidi.cpp`, `udpmidi.cpp`). Macs and iPads connect natively, and Windows connects with the free rtpMIDI driver. We're turning WiFi on for updates anyway, so this comes for free.
- **BLE MIDI controllers:** a class-compliant USB-to-BLE MIDI dongle in the USB port (for example the CME WIDI Bud Pro) shows up to MiniDexed as an ordinary USB MIDI device. That needs no code. I haven't tested a particular dongle with MiniDexed, so this is inferred from how they present over USB. The Zero 2 W has one data port, so using it with a USB keyboard as well needs a hub.

## 7. Build order

1. **Hardware patch:** all 12 keys (stop serial MIDI so r/f are free; check z/x) and all 4 encoders with acceleration. Generic key and encoder events go to the new UI.
2. **PNG mockups** of views 4.1-4.8 for your review.
3. **Framework plus the Voice and Mixer views,** with the touch overlay. This is the first flashable build.
4. **FM, Operators and Envelope views** with the macros.
5. **Browse and Keys views,** with the deep menu kept as a fallback.
6. **LFO view,** then the optional GRIT effect.
