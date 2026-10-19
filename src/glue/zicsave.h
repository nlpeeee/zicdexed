//
// zicsave.h - SAVE VOICE: add a voice to the user bank file on the SD card.
//
#pragma once
#include <stdint.h>

namespace zic {

// The user bank is /sysex/voice/000099_ZICUSER.syx (bank 99 in BROWSE), created
// on first use, 32 slots. MiniDexed reads bank files once at boot, so a saved
// voice is served from a copy kept here (UserBankFile) until the next restart.

// Names of the 32 slots, 10 characters each plus a NUL ("INIT VOICE" = free).
// A missing file reads as all free. Returns false when the file is not a bank.
bool ReadUserBankNames(char names[32][11]);

// Saves into `slot` 0..31, or the first free slot for -1. The name is the one in
// the voice. Returns the slot, -2 bank full, -3 file error.
int SaveVoiceToUserSlot(const uint8_t voice[155], int slot);

// Same name replaces its slot, otherwise the first free slot.
int SaveVoiceToUserBank(const uint8_t voice[155]);

// A file that is not a bank is never overwritten by a save. ResetUserBank()
// replaces it: with the .bak from before the last save when that is a good bank
// (returns 1), else with an empty bank (2). -3 file error.
int ResetUserBank();

// The bank file as last read or written by this session (4104 bytes), or null
// when nothing was saved yet.
const uint8_t* UserBankFile();

} // namespace zic
