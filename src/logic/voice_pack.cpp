#include "voice_pack.h"

namespace zd {

static const int kHdr = 6;

static void Checksum(uint8_t file[kBankFileSize])
{
	int sum = 0;
	for (int i = 0; i < kBankSlots * kPackedVoice; i++) sum += file[kHdr + i];
	file[kHdr + kBankSlots * kPackedVoice] = (uint8_t)((-sum) & 0x7F);
}

void PackVoice(const Voice& v, uint8_t out[kPackedVoice])
{
	memset(out, 0, kPackedVoice);
	for (int op = 0; op < kOps; op++) {
		const uint8_t* s = &v.d[op * kOpSize];  // OP6 first, same order as the bulk format
		uint8_t* o = &out[op * 17];
		memcpy(o, s, 11);                       // EG, breakpoint, left/right depth
		o[11] = (uint8_t)((s[OP_LC] & 3) | ((s[OP_RC] & 3) << 2));
		o[12] = (uint8_t)((s[OP_RS] & 7) | ((s[OP_PD] & 15) << 3));
		o[13] = (uint8_t)((s[OP_AMS] & 3) | ((s[OP_KVS] & 7) << 2));
		o[14] = s[OP_OL];
		o[15] = (uint8_t)((s[OP_PM] & 1) | ((s[OP_PC] & 31) << 1));
		o[16] = s[OP_PF];
	}
	memcpy(&out[102], &v.d[G_PR1], 8);          // pitch EG
	out[110] = v.d[G_ALG] & 31;
	out[111] = (uint8_t)((v.d[G_FB] & 7) | ((v.d[G_OKS] & 1) << 3));
	memcpy(&out[112], &v.d[G_LFS], 4);          // LFO speed, delay, pitch depth, amp depth
	out[116] = (uint8_t)((v.d[G_LFKS] & 1) | ((v.d[G_LFW] & 7) << 1) | ((v.d[G_LPMS] & 7) << 4));
	out[117] = v.d[G_TRNSP];
	memcpy(&out[118], &v.d[G_NAME], kNameLen);
}

void InitBankFile(uint8_t file[kBankFileSize])
{
	file[0] = 0xF0; file[1] = 0x43; file[2] = 0x00; file[3] = 0x09; file[4] = 0x20; file[5] = 0x00;
	uint8_t packed[kPackedVoice];
	PackVoice(Voice::Init(), packed);
	for (int i = 0; i < kBankSlots; i++) memcpy(&file[kHdr + i * kPackedVoice], packed, kPackedVoice);
	file[kBankFileSize - 1] = 0xF7;
	Checksum(file);
}

bool IsBankFile(const uint8_t* f, int size)
{
	return size == kBankFileSize && f[0] == 0xF0 && f[1] == 0x43 && f[3] == 0x09 && f[kBankFileSize - 1] == 0xF7;
}

int FindSaveSlot(const uint8_t file[kBankFileSize], const char name[kNameLen + 1])
{
	int firstInit = -1;
	for (int i = 0; i < kBankSlots; i++) {
		const uint8_t* nm = &file[kHdr + i * kPackedVoice + 118];
		if (memcmp(nm, name, kNameLen) == 0) return i;
		if (firstInit < 0 && memcmp(nm, "INIT VOICE", kNameLen) == 0) firstInit = i;
	}
	return firstInit;
}

void StoreVoice(uint8_t file[kBankFileSize], int slot, const Voice& v)
{
	if (slot < 0 || slot >= kBankSlots) return;
	PackVoice(v, &file[kHdr + slot * kPackedVoice]);
	Checksum(file);
}

} // namespace zd
