# ZicPixel MiniDexed: interface refinement plan

Starting point: the POC (custom `kernel8.img` from MiniDexed main `8e3fa30` plus the 170x320 row-offset patch, and `minidexed.ini`). Today it uses one encoder and 7 of the 12 keys. The screen shows the stock MiniDexed 20x4 text menu.

Goal: make it feel like a ZicPixel instrument. That means all 4 encoders doing something useful, all 12 keys, and a home screen built for playing (zicBox style: four parameter columns sitting over the four encoders) instead of a menu.

## What the MiniDexed code allows (checked in source)

| Area | Stock behaviour | Implication |
|---|---|---|
| Encoders | One KY-040 encoder only (`userinterface.cpp:179`) | Encoders 2-4 need a firmware patch |
| Keys | Fixed roles: Prev, Next, Back, Select, Home, Shortcut, Pgm+/-, Bank+/-, TG+/-. Each takes click, double-click or long-press (`minidexed.ini`) | 11 roles, so the layout can be tidied with the ini alone |
| Keys z, x (GPIO 7/8, SPI CE) | Buttons are set up after SPI and reset their pin to input with pull-up (`uibuttons.cpp:84`). The screen has no CS line, so SPI doesn't need these pins | They probably work as keys with **no code change**. Needs a test on the hardware |
| Keys r, f (GPIO 14/15, UART) | Serial MIDI starts **after** the UI (`minidexed.cpp:279` then `:286`) and takes the pins back | Needs a small patch: an ini switch to turn serial MIDI off |
| Enc3 (GPIO 0/9) | GPIO9 is SPI MISO, which the screen doesn't use | Should work once multi-encoder support exists. Test it |
| Screen | Text-only character grid (`CST7789Device`, Font8x16). Circle's ST7789 driver underneath can draw colour areas and text | A graphical home screen means writing our own drawing code. A full redraw takes about 60 ms at 15 MHz, so only changed areas get redrawn |
| Sound shaping per TG | Volume, Pan, Reverb send, Detune, Cutoff, Resonance, Mono, Portamento, Pitch bend, plus full voice/operator edit. Effects: compressor and reverb | The "nasty" parameters already exist (Cutoff/Res, operator feedback, detune). There's no drive or bitcrush effect |
| Updating | Network plus FTP, with `updater.py` pushing a new kernel over WiFi (Pi Zero 2 W has WiFi) | We can iterate without pulling the SD card out each time |

## Proposed controls

Keeps what you already learned in the POC (c Back, v Select, d Home, a/s Program, q/w TG).

```
 [Enc1]      [Enc2]      [Enc3]      [Enc4]        perform screen: param 1..4 of the current page
                                                   menus: navigate | coarse value | TG | operator

 q TG-       w TG+       e Bank-     r Bank+
 a Prog-     s Prog+     d Home      f Audition    (hold = plays a note on the current TG, no keyboard needed)
 z Shift     x Page      c Back      v Select
```

Shift combinations:
- Shift+encoder: fine or coarse step
- Shift+q/w: mute or solo the current TG
- Shift+d: all notes off (panic)
- Shift+v: save performance
- Shift+x: previous page

Perform-screen pages, cycled with x. Each page is 4 knobs for the current TG:

| Page | Enc1 | Enc2 | Enc3 | Enc4 |
|---|---|---|---|---|
| MIX | Volume | Pan | Reverb send | Detune |
| TONE | Cutoff | Resonance | Op feedback | Transpose |
| PLAY | Mono/Poly | Portamento time | Porta mode | Pitch bend range |
| GRIT (phase 5) | Drive | Bit depth | Downsample | Wet/dry |

Screen layout (320x170):
- Top strip: TG1-8 boxes, lit when playing, with the current TG highlighted.
- Middle: large voice name with the bank and program number.
- Bottom ~70 px: four quarter-width columns, each showing the parameter name, a value bar and the number, in the zicBox track colours.

The text menu stays available for deep editing (Select from the home screen opens it).

## Phases

Each phase ends with something you can flash and try.

**Phase 1: ini only, no rebuild** (small)
- Move the keys to the layout above as far as stock roles allow: z gets Shortcut, x gets Next/Prev, e/r get Bank-/+.
- Confirm z and x actually register.
- Hardware checks: whether rotation 180 is right, which physical knob is Enc1, and encoder direction and resolution.
- Optional: turn on WiFi plus FTP so later builds can be pushed with `updater.py`. You'd put your WiFi details in `wpa_supplicant.conf` yourself.

**Phase 2: hardware patch set** (small firmware change)
- `SerialMIDIEnabled=0` ini switch, which frees r/f.
- `Encoder2/3/4PinClock/Data` ini keys, so all 4 encoders send events.
- Audition key: note on/off for the current TG's channel.
- Shift modifier handling.

**Phase 3: perform screen** (the main piece)
- Render a mockup PNG here first, so you can approve the look before anything is flashed.
- Then build it: a new home view drawn directly to the ST7789 with partial redraws, pages, and encoder-to-parameter mapping.
- The menu becomes the "deep edit" view.

**Phase 4: menu polish**
- Make Enc2-4 useful inside menus: coarse value, TG select, operator select in Edit Voice.
- Use the extra screen space better, e.g. more rows with the small font or the parameter value in a larger font.
- Show the parameter value as a bar.

**Phase 5: GRIT effect** (optional, fits the DnB/electro direction)
- Per-TG or master drive/bitcrush/downsample in the audio path.
- Check CPU headroom with 8 TGs on the Zero 2 W before committing to per-TG.
- Render-test it here with Synth_Dexed, the same way the nasty bank was tested.

## How we'd work

- **Code:** MiniDexed patches plus a build script in a `minidexed/` folder in the zp repo, on a branch with a PR per phase. Each phase is a small diff on top of the pinned MiniDexed commit, so it can be rebased onto newer MiniDexed later.
- **Build:** done here, using the ARM toolchain staged from `E:\minidexed`. The outputs (kernel8.img plus ini) go to `E:\minidexed\ZicPixel_overlay` as before.
- **Testing:** I can compile and render-test sound and screen mockups here. Only you can confirm keys, encoders and the real screen, so each phase comes with a short checklist for you to run.

## Decisions for you

1. **Where the code lives:** zp repo `minidexed/` folder (recommended) or a separate MiniDexed fork.
2. **Serial/DIN MIDI on this unit:** never used (recommended assumption, frees r/f) or keep it.
3. **Home screen:** custom perform screen (recommended) or keep the stock text menu and only improve it.
4. **Key layout:** the layout above as-is, or tell me what to change.
5. **WiFi updates:** enable FTP updating (recommended, faster iteration) or keep swapping the SD card.

### Decided (2026-10-15)
1. Code lives in its own repo, `nlpeeee/zicdexed`.
2. MIDI is over USB only, so serial MIDI is turned off and r/f become keys. Bluetooth would be nice but is a stretch (see ui-design.md section 6).
3. Custom perform UI that makes full use of the screen and all 4 encoders. The full design is in ui-design.md, and it replaces "Proposed controls" above.
4. Key layout redesigned around zicBox conventions in ui-design.md.
5. WiFi/FTP updates: yes.
