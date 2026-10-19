# Stock performances

The four banks here (`001_Seed`, `002_Greenhouse`, `003_Oasis`, `128_Laboratory`, 281 performances) are the stock performances from the MiniDexed 1277 release (2025-07-30, 8f2c638), copied unchanged. They come from [MiniDexed](https://github.com/probonopd/MiniDexed) and are licensed under GPL-3.0, like the rest of this repo.

`build.sh` copies the four bank folders (not this README) into `out/performance/`. The performance loaded at power-on is not in here: it is `performance.ini` in the card root, built from `config/performance.ini` (the stock "Rock Piano" from 001_Seed, with TGs 1 to 3 on OMNI).

On the card, merge these folders into the existing `performance/` folder without overwriting files that are already there, so performances you saved are kept.
