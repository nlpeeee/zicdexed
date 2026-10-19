# ZicDexed V1.2

- Install and build instructions rewritten (see README): what to download, what to replace and what to merge, and how to build from source.
- The card package now includes the stock MiniDexed performance banks (001_Seed, 002_Greenhouse, 003_Oasis, 128_Laboratory; GPL-3.0, from MiniDexed 1277), so BROWSE shows performances on a fresh card.
- Walkthrough: adding performances and performance banks.
- WiFi firmware helper scripts (`tools/get-wifi-firmware.ps1` / `.sh`) included.

## Install
Format a card FAT32, unzip a [MiniDexed release](https://github.com/probonopd/MiniDexed/releases) onto it, then copy the contents of `ZicDexed_v1_2_card.zip` over it following the README. Existing cards: replace `kernel8.img`, `minidexed.ini` and `performance.ini`; merge `sysex/voice` and `performance`, never replace those folders.

## Not in this release
Yamaha factory banks (build from your own Dexed cartridges with `tools/stock_banks.py`).
