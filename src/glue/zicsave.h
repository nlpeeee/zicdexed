//
// zicsave.h - SAVE VOICE: add a voice to the user bank file on the SD card.
//
#pragma once
#include <stdint.h>

namespace zic {

// Packs the 155-byte voice into /sysex/voice/000099_ZICUSER.syx (created on first
// use, 32 slots). A voice with the same name replaces its slot, otherwise the
// first free slot is used. Returns the slot 0..31, -2 bank full, -3 file error.
int SaveVoiceToUserBank(const uint8_t voice[155]);

} // namespace zic
