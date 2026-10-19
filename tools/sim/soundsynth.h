// A synth with real voice storage for the sound-design pages: loadVoice/readVoice,
// two banks of 32 voices (bank 2 does not exist), COMPARE and revert like the glue.
#pragma once
#include "fakesynth.h"
#include "dx7_voice.h"

namespace zic {

struct SoundSynth : FakeSynth {
	uint8_t voice[kMaxTG][kVoiceSize];
	uint8_t saved[kMaxTG][kVoiceSize];
	bool comparing[kMaxTG] = {};
	int perfSaves = 0, voiceSaves = 0, saveResult = 0;
	uint8_t lastSaved[kVoiceSize] = {};
	int notesOn = 0, notesOff = 0, lastNote = -1, loads = 0;
	int notesHeld[16] = {};

	SoundSynth()
	{
		for (int t = 0; t < kMaxTG; t++) bankVoice(0, tgv[t][P_PROGRAM], voice[t]);
	}

	// bank 3 is the user bank SAVE VOICE writes to
	uint8_t userVoice[32][kVoiceSize];
	bool userUsed[32] = {};
	int userStatus = 1;                // what userBankNames() reports
	int userSaves = 0, lastUserSlot = -1;

	void bankVoice(int bank, int idx, uint8_t* out)
	{
		if (bank == 3 && idx >= 0 && idx < 32 && userUsed[idx]) { memcpy(out, userVoice[idx], kVoiceSize); return; }
		zd::Voice v = zd::Voice::Init();
		v.set(zd::G_ALG, (idx * 5 + bank * 11) % 32);
		for (int o = 0; o < 6; o++) {
			v.set(zd::OpOff(o, zd::OP_OL), 60 + (idx * 3 + o * 7) % 40);
			v.set(zd::OpOff(o, zd::OP_PC), 1 + (idx + o) % 5);
		}
		v.set(zd::G_FB, idx % 8);
		char nm[11];
		snprintf(nm, sizeof nm, "B%d VOI %02d", bank, idx + 1);
		v.setName(nm);
		memcpy(out, v.d, kVoiceSize);
	}

	void set(TGParam p, int tg, int v) override
	{
		FakeSynth::set(p, tg, v);
		if (p == P_PROGRAM || p == P_BANK) {
			if (bankValid(tgv[tg][P_BANK])) bankVoice(tgv[tg][P_BANK], tgv[tg][P_PROGRAM], voice[tg]);
			nameFrom(tg);
		}
	}
	void nameFrom(int tg) { memcpy(names[tg], voice[tg] + 145, kNameLen); names[tg][kNameLen] = 0; }
	void loadVoice(int tg, const uint8_t* v) override { memcpy(voice[tg], v, kVoiceSize); loads++; nameFrom(tg); }
	void readVoice(int tg, uint8_t* v) override { memcpy(v, voice[tg], kVoiceSize); }
	void noteOn(int tg, int note, int) override { FakeSynth::noteOn(tg, note, 0); notesOn++; lastNote = note; notesHeld[tg]++; }
	void noteOff(int tg, int note) override { FakeSynth::noteOff(tg, note); notesOff++; lastNote = note; notesHeld[tg]--; }
	void compareBegin(int tg) override { memcpy(saved[tg], voice[tg], kVoiceSize); comparing[tg] = true; bankVoice(tgv[tg][P_BANK], tgv[tg][P_PROGRAM], voice[tg]); }
	void compareEnd(int tg) override { if (comparing[tg]) memcpy(voice[tg], saved[tg], kVoiceSize); comparing[tg] = false; }
	void revert(int tg) override { bankVoice(tgv[tg][P_BANK], tgv[tg][P_PROGRAM], voice[tg]); nameFrom(tg); }
	bool savePerformance() override { perfSaves++; return true; }
	int saveVoice(const uint8_t* v) override { voiceSaves++; memcpy(lastSaved, v, kVoiceSize); return saveResult; }
	int userBankNames(char names[32][kNameLen + 1]) override
	{
		if (userStatus != 1) return userStatus;
		for (int i = 0; i < 32; i++) {
			if (userUsed[i]) { memcpy(names[i], userVoice[i] + 145, kNameLen); names[i][kNameLen] = 0; }
			else strcpy(names[i], "INIT VOICE");
		}
		return 1;
	}
	int saveVoiceTo(const uint8_t* v, int slot) override
	{
		if (saveResult < 0) return saveResult;
		if (slot < 0) { for (slot = 0; slot < 32 && userUsed[slot]; slot++) {} if (slot >= 32) return -2; }
		memcpy(userVoice[slot], v, kVoiceSize);
		userUsed[slot] = true;
		userSaves++; lastUserSlot = slot;
		return slot;
	}
	int getFx(FxParam p) override { return fx[p]; }
	int bankCount() override { return 4; }
	bool bankValid(int b) override { return b >= 0 && b < 4 && b != 2; }
	void bankName(int b, char out[32]) override { snprintf(out, 32, b == 0 ? "ROM1A" : (b == 1 ? "nasty" : "bank3")); }
	bool voiceNameIn(int b, int i, char* out) override
	{
		if (!bankValid(b)) return false;
		uint8_t v[kVoiceSize];
		bankVoice(b, i, v);
		memcpy(out, v + 145, kNameLen);
		out[kNameLen] = 0;
		return true;
	}
	bool readVoiceIn(int b, int i, uint8_t* v) override
	{
		if (!bankValid(b)) return false;
		bankVoice(b, i, v);
		return true;
	}
};

} // namespace zic
