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
uint8_t g_File[zd::kBankFileSize];
}

int SaveVoiceToUserBank(const uint8_t voice[155])
{
	bool exists = false, valid = false;
	if (FILE* f = fopen(kPath, "rb")) {
		exists = true;
		size_t n = fread(g_File, 1, sizeof g_File, f);
		valid = zd::IsBankFile(g_File, (int)n);
		fclose(f);
	}
	if (exists && !valid) return -3;       // never overwrite a file we do not understand
	if (!exists) zd::InitBankFile(g_File);

	zd::Voice v;
	memcpy(v.d, voice, zd::kVoiceSize);
	char name[zd::kNameLen + 1];
	v.name(name);
	const int slot = zd::FindSaveSlot(g_File, name);
	if (slot < 0) return -2;
	zd::StoreVoice(g_File, slot, v);

	FILE* f = fopen(kPath, "wb");
	if (!f) return -3;
	const size_t w = fwrite(g_File, 1, sizeof g_File, f);
	const int rc = fclose(f);
	return (w == sizeof g_File && rc == 0) ? slot : -3;
}

} // namespace zic
