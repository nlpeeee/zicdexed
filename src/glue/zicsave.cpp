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
// A save goes to kTmp first, then replaces kPath; kBak holds the file before the
// last save. (FAT rename does not overwrite, so the old file is moved aside first.)
const char kTmp[] = ZIC_USER_BANK_PATH ".tmp";
const char kBak[] = ZIC_USER_BANK_PATH ".bak";
uint8_t g_File[zd::kBankFileSize];     // scratch: the file being read or edited
uint8_t g_Served[zd::kBankFileSize];   // the bank as of our last save
bool g_HaveServed = false;

// Reads `path` into `out`; true when it is a complete bank file.
bool ReadBank(const char* path, uint8_t out[zd::kBankFileSize])
{
	FILE* f = fopen(path, "rb");
	if (!f) return false;
	uint8_t buf[zd::kBankFileSize];
	const size_t n = fread(buf, 1, sizeof buf, f);
	fclose(f);
	if (!zd::IsBankFile(buf, (int)n)) return false;
	memcpy(out, buf, zd::kBankFileSize);
	return true;
}

bool Exists(const char* path)
{
	FILE* f = fopen(path, "rb");
	if (f) fclose(f);
	return f != nullptr;
}

// Writes `data` to kTmp and reads it back, so a short write never gets to replace the bank.
bool WriteTmp(const uint8_t data[zd::kBankFileSize])
{
	FILE* f = fopen(kTmp, "wb");
	if (!f) return false;
	const size_t w = fwrite(data, 1, zd::kBankFileSize, f);
	const int rc = fclose(f);
	if (w != (size_t)zd::kBankFileSize || rc != 0) { remove(kTmp); return false; }
	uint8_t back[zd::kBankFileSize];
	if (!ReadBank(kTmp, back) || memcmp(back, data, sizeof back) != 0) { remove(kTmp); return false; }
	return true;
}

// Replaces the bank file with `data`. The previous good file stays as kBak. On any
// failure the old file is still in place (or recoverable by LoadFile).
bool WriteSafely(const uint8_t data[zd::kBankFileSize], bool keepOld)
{
	if (!WriteTmp(data)) return false;
	if (Exists(kPath)) {
		if (keepOld) {
			remove(kBak);
			if (rename(kPath, kBak) != 0) { remove(kTmp); return false; }
		} else if (remove(kPath) != 0) { remove(kTmp); return false; }
	}
	if (rename(kTmp, kPath) != 0) {
		if (keepOld && Exists(kBak)) rename(kBak, kPath);     // put the old one back
		remove(kTmp);
		return false;
	}
	return true;
}

// 1 = loaded, 0 = no file (g_File = a fresh bank), -1 = a file we do not understand
int LoadFile()
{
	if (ReadBank(kPath, g_File)) return 1;
	if (Exists(kPath)) return -1;
	// no bank file: a save may have been cut between its two renames (.tmp is the new file, complete)
	if (ReadBank(kTmp, g_File) && rename(kTmp, kPath) == 0) return 1;
	zd::InitBankFile(g_File);
	return 0;
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

	if (!WriteSafely(g_File, true)) return -3;
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

int ResetUserBank()
{
	uint8_t bak[zd::kBankFileSize];
	const bool haveBak = ReadBank(kBak, bak);
	if (haveBak) memcpy(g_File, bak, sizeof g_File);
	else zd::InitBankFile(g_File);
	// the broken file is replaced, not kept as the backup
	if (!WriteSafely(g_File, false)) return -3;
	memcpy(g_Served, g_File, sizeof g_Served);
	g_HaveServed = true;
	return haveBak ? 1 : 2;
}

const uint8_t* UserBankFile() { return g_HaveServed ? g_Served : nullptr; }

} // namespace zic
