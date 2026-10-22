# Agent guide: ZicDexed

MiniDexed (8 x DX7 FM, bare metal on Circle) rebuilt for the ZicPixel: Pi Zero 2 W,
ST7789 170x320 screen, 4 encoders, 12 keys. See `README.md` for hardware and the key map.

## Layout
- `src/ui/` – screen framework, pages, graphics, bitmap fonts. No Circle dependency.
- `src/glue/` – keys, encoders, panel flush, MiniDexed adaptor (Circle-dependent).
- `src/logic/` – pure C++17 logic, host-testable.
- `patches/minidexed/`, `patches/circle/` – patches applied to the pinned MiniDexed and Circle.
- `config/` – `minidexed.ini`, `performance.ini`, WiFi template.
- `tests/` – host tests for `src/logic`; `tests/ui/` – host tests and screenshot renderer for `src/ui`.
- `tools/` – simulator (`sim/`, `hostsim/`), font and bank generators, WiFi firmware download.
- `docs/` – design notes (`ui-design.md`, `firmware-ui.md`), simulator, WiFi, walkthrough.

## Build and test
Everything builds on Linux. From Windows, run through WSL (`wsl -d Debian -- ...`).
- Logic tests: `make -C tests test`
- UI tests: `make -C tests/ui test`
- UI screenshots: `make -C tests/ui shots` (output in `tests/ui/shots/`)
- Firmware: `./build.sh` needs the Arm GNU toolchain 13.3.rel1 `aarch64-none-elf` on PATH.
  The first build clones MiniDexed into `build/` and compiles Circle (slow, needs network);
  `ZIC_QUICK=1 ./build.sh` rebuilds only MiniDexed and `src/` after that.

Run the host tests for anything you change in `src/ui` or `src/logic`. Ask before a full
firmware build; prefer `ZIC_QUICK=1` when `build/` already exists in your worktree.

## Rules
- Edit ZicDexed code in `src/`, `patches/`, `config/`, `tools/`, `tests/`. Never edit files
  under `build/` (generated and overwritten by `build.sh`) or `out/` (build output).
- Changes to MiniDexed or Circle go in as patch files under `patches/`, not as edits to `build/`.
- Keep `src/ui` free of Circle includes so it keeps building on the host.
- Do not commit Yamaha factory banks, WiFi chip firmware, `wpa_supplicant.conf`, or anything
  in `build/`, `out/` or `toolchain/`.
- Do not push firmware to a device over FTP or touch an SD card without explicit approval.
