// ISynth for the host simulator that behaves like the firmware glue on a fake SD card
// directory: banks come from the REAL MiniDexed CSysExFileLoader, SAVE VOICE is the REAL
// zicsave.cpp, SAVE PERFORMANCE follows MiniDexed's rule (needs <sd>/performance).
#pragma once
#include "soundsynth.h"
#include "sysexfileloader.h"
#include "zicsave.h"
#include <string>
#include <sys/stat.h>

namespace zic {

struct SdSynth : SoundSynth {
	std::string sd;
	CSysExFileLoader loader;
	std::string net;                     // what ZicGetNetStatus() holds
	bool netEnabled = true;
	int perfWrites = 0;

	explicit SdSynth(const std::string& sdDir) : sd(sdDir), loader((sdDir + "/sysex").c_str())
	{
		loader.Load(false);
		for (int t = 0; t < kMaxTG; t++) { tgv[t][P_BANK] = 0; tgv[t][P_PROGRAM] = t; LoadStored(t); nameFrom(t); }
	}

	// --- same logic as src/glue/zicsynth_dexed.cpp
	void LoadStored(int tg)
	{
		uint8_t v[kVoiceSize];
		if (readVoiceIn(tgv[tg][P_BANK], tgv[tg][P_PROGRAM], v)) memcpy(voice[tg], v, kVoiceSize);
	}
	void set(TGParam p, int tg, int v) override
	{
		FakeSynth::set(p, tg, v);          // skip SoundSynth's synthetic banks
		if (p == P_PROGRAM || p == P_BANK) { LoadStored(tg); nameFrom(tg); }
	}
	void compareBegin(int tg) override { if (comparing[tg]) return; memcpy(saved[tg], voice[tg], kVoiceSize); comparing[tg] = true; LoadStored(tg); nameFrom(tg); }
	void compareEnd(int tg) override { if (!comparing[tg]) return; comparing[tg] = false; memcpy(voice[tg], saved[tg], kVoiceSize); nameFrom(tg); }
	void revert(int tg) override { LoadStored(tg); nameFrom(tg); }
	int bankCount() override { return (int)loader.GetNumHighestBank() + 1; }
	bool bankValid(int b) override { return b >= 0 && b <= (int)CSysExFileLoader::MaxVoiceBankID && loader.IsValidBank(b); }
	void bankName(int b, char out[32]) override
	{
		std::string s = loader.GetBankName(b);
		unsigned n = 0;
		for (; n < s.size() && n < 31; n++) out[n] = s[n];
		out[n] = 0;
	}
	bool voiceNameIn(int b, int i, char* out) override
	{
		if (!bankValid(b)) return false;
		std::string s = loader.GetVoiceName(b, i);
		unsigned n = 0;
		for (; n < s.size() && n < (unsigned)kNameLen; n++) out[n] = (s[n] >= 32 && s[n] < 127) ? s[n] : ' ';
		for (; n < (unsigned)kNameLen; n++) out[n] = ' ';
		out[kNameLen] = 0;
		return true;
	}
	bool readVoiceIn(int b, int i, uint8_t* v) override
	{
		if (!bankValid(b)) return false;
		uint8_t buf[156];
		loader.GetVoice(b, i, buf);
		memcpy(v, buf, kVoiceSize);
		return true;
	}
	int saveVoice(const uint8_t* v) override { voiceSaves++; return SaveVoiceToUserBank(v); }
	bool savePerformance() override
	{
		struct stat st;
		if (stat((sd + "/performance").c_str(), &st) != 0 || !S_ISDIR(st.st_mode)) return false;   // CPerformanceConfig::GetInternalFolderOk()
		FILE* f = fopen((sd + "/performance.ini").c_str(), "w");
		if (!f) return false;
		fprintf(f, "BankNumber1=%d\nVoiceNumber1=%d\nVoiceName1=%.10s\n", tgv[0][P_BANK], tgv[0][P_PROGRAM] + 1, (const char*)voice[0] + 145);
		fclose(f);
		perfWrites++;
		return true;
	}
	void netStatus(char out[32]) override
	{
		const char* p = net.c_str();
		static const char connecting[] = "WiFi connecting";
		if (!p[0] && netEnabled) p = connecting;
		unsigned n = 0;
		for (; p[n] && n < 31; n++) out[n] = p[n];
		out[n] = 0;
	}
};

} // namespace zic
