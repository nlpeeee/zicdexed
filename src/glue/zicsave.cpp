#include "zicsave.h"
#include "voice_pack.h"
#include <stdio.h>
#include <string.h>

namespace zic {

namespace {
// File numbers are 1-indexed in MiniDexed, so this is bank 99 in BROWSE.
// (The host tests point ZIC_USER_BANK_PATH at a temp file.)
#ifndef ZIC_USER_BANK_PATH
#define ZIC_USER_BANK_PATH "/sysex/voice/000099_ZICUSER.syx"
#endif
const char kPath[] = ZIC_USER_BANK_PATH;
uint8_t g_File[zd::kBankFileSize];     // scratch: the file being read or edited
uint8_t g_Served[zd::kBankFileSize];   // the bank as of our last save
bool g_HaveServed = false;

// 1 = loaded, 0 = no file (g_File = a fresh bank), -1 = a file we do not understand
int LoadFile()
{
	FILE* f = fopen(kPath, "rb");
	if (!f) { zd::InitBankFile(g_File); return 0; }
	size_t n = fread(g_File, 1, sizeof g_File, f);
	fclose(f);
	if (!zd::IsBankFile(g_File, (int)n)) return -1;
	return 1;
}
}

bool ReadUserBankNames(char names[32][11])
{
	if (LoadFile() < 0) return false;
	for (int i = 0; i < zd::kBankSlots; i++) zd::SlotName(g_File, i, names[i]);
	return true;
}

int SaveVoiceToUserSlot(const uint8_t voice[155], int slot)
{
	if (LoadFile() < 0) return -3;         // never overwrite a file we do not understand
	if (slot < 0) {
		slot = -1;
		for (int i = 0; i < zd::kBankSlots && slot < 0; i++) if (zd::SlotIsFree(g_File, i)) slot = i;
		if (slot < 0) return -2;
	}
	if (slot >= zd::kBankSlots) return -3;

	zd::Voice v;
	memcpy(v.d, voice, zd::kVoiceSize);
	zd::StoreVoice(g_File, slot, v);

	FILE* f = fopen(kPath, "wb");
	if (!f) return -3;
	const size_t w = fwrite(g_File, 1, sizeof g_File, f);
	const int rc = fclose(f);
	if (w != sizeof g_File || rc != 0) return -3;
	memcpy(g_Served, g_File, sizeof g_Served);
	g_HaveServed = true;
	return slot;
}

int SaveVoiceToUserBank(const uint8_t voice[155])
{
	if (LoadFile() < 0) return -3;
	zd::Voice v;
	memcpy(v.d, voice, zd::kVoiceSize);
	char name[zd::kNameLen + 1];
	v.name(name);
	const int slot = zd::FindSaveSlot(g_File, name);
	if (slot < 0) return -2;
	return SaveVoiceToUserSlot(voice, slot);
}

const uint8_t* UserBankFile() { return g_HaveServed ? g_Served : nullptr; }

} // namespace zic
