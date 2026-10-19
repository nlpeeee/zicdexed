//
// voice_pack.h - DX7 bank files (32 packed voices, 4104 bytes with SysEx header
// and footer) and packing of one edited voice into a slot. This is what
// SAVE VOICE writes into the user bank file on the SD card; the file I/O itself
// is in src/glue/zicsave.cpp.
//
#pragma once
#include "dx7_voice.h"

namespace zd {

constexpr int kPackedVoice = 128;
constexpr int kBankSlots = 32;
constexpr int kBankFileSize = 4104;   // F0 43 00 09 20 00, 32 x 128, checksum, F7

// Pack one voice into the DX7's 128-byte bulk format.
void PackVoice(const Voice& v, uint8_t out[kPackedVoice]);

// The inverse: one 128-byte packed voice back to the 155-byte voice.
void UnpackVoice(const uint8_t packed[kPackedVoice], Voice& out);

// A bank file full of INIT VOICE.
void InitBankFile(uint8_t file[kBankFileSize]);
bool IsBankFile(const uint8_t* file, int size);

// Slot a voice should be saved to: the slot already holding the same name, else
// the first INIT VOICE slot, else -1 when the bank is full.
int FindSaveSlot(const uint8_t file[kBankFileSize], const char name[kNameLen + 1]);

// Name of a slot (10 characters, as stored, plus a terminator).
void SlotName(const uint8_t file[kBankFileSize], int slot, char out[kNameLen + 1]);
bool SlotIsFree(const uint8_t file[kBankFileSize], int slot);   // holds "INIT VOICE"

// Write the voice into the slot and fix the checksum.
void StoreVoice(uint8_t file[kBankFileSize], int slot, const Voice& v);

} // namespace zd
