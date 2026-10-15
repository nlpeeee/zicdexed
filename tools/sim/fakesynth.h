// A fake synth for the PC simulator and the UI tests.
#pragma once
#include "zicsynth.h"
#include <math.h>
#include <string.h>
#include <stdio.h>

namespace zic {

struct FakeSynth : ISynth {
	int n = 8;
	int tgv[kMaxTG][kTGParamCount];
	char names[kMaxTG][kNameLen + 1];
	bool muted[kMaxTG] = {};
	bool held[kMaxTG] = {};
	float lvl[kMaxTG] = {};
	int scopeTG = 0;
	double phase = 0;
	int log_notes = 0, compares = 0, reverts = 0, panics = 0;
	int fx[kFxParamCount] = {};

	FakeSynth()
	{
		const char* nm[kMaxTG] = { "REESE BS", "WOBBLE BS", "KICK", "HAT", "", "SNARE", "HOOVER", "SCREAM" };
		const int vol[kMaxTG] = { 100, 90, 70, 80, 0, 60, 75, 50 };
		const int pan[kMaxTG] = { 64, 50, 80, 64, 64, 40, 90, 64 };
		for (int i = 0; i < kMaxTG; i++) {
			memset(tgv[i], 0, sizeof tgv[i]);
			tgv[i][P_VOLUME] = vol[i];
			tgv[i][P_PAN] = pan[i];
			tgv[i][P_CUTOFF] = 72;
			tgv[i][P_RESO] = 45;
			tgv[i][P_DETUNE] = 12;
			tgv[i][P_REVERB] = 20;
			tgv[i][P_CHANNEL] = i;
			tgv[i][P_PROGRAM] = i;
			tgv[i][P_MONO] = i == 0;
			tgv[i][P_PORTA_TIME] = i == 0 ? 35 : 0;
			tgv[i][P_PB_RANGE] = 12;
			snprintf(names[i], sizeof names[i], "%s", nm[i]);
		}
	}
	int tgCount() override { return n; }
	int get(TGParam p, int tg) override { return tgv[tg][p]; }
	void set(TGParam p, int tg, int v) override
	{
		const ParamRange& r = paramRange(p);
		tgv[tg][p] = v < r.min ? r.min : (v > r.max ? r.max : v);
	}
	int getFx(FxParam p) override { return fx[p]; }
	void setFx(FxParam p, int v) override { fx[p] = v; }
	void voiceName(int tg, char out[kNameLen + 1]) override { memcpy(out, names[tg], kNameLen + 1); }
	void bankName(int, char out[32]) override { strcpy(out, "nasty"); }
	void loadVoice(int, const uint8_t*) override {}
	void readVoice(int, uint8_t* v) override { memset(v, 0, kVoiceSize); }
	void noteOn(int tg, int, int) override { held[tg] = true; log_notes++; }
	void noteOff(int tg, int) override { held[tg] = false; log_notes++; }
	void panic() override { panics++; for (int i = 0; i < kMaxTG; i++) held[i] = false; }
	void compareBegin(int) override { compares++; }
	void compareEnd(int) override { compares--; }
	void revert(int) override { reverts++; }
	int bankCount() override { return 1; }
	bool bankValid(int) override { return true; }
	bool voiceNameIn(int, int, char*) override { return false; }
	bool readVoiceIn(int, int, uint8_t*) override { return false; }
	void setMute(int tg, bool m) override { muted[tg] = m; }
	float level(int tg) override { float l = lvl[tg]; return l; }
	void setScopeTG(int tg) override { scopeTG = tg; }
	int scope(int16_t* out, int cnt) override
	{
		// a reese-ish wave
		for (int i = 0; i < cnt; i++) {
			double t = (double)i / 164.0;
			double v = sin(t * 2 * M_PI * 3) * 0.55 + sin(t * 2 * M_PI * 3.02 * 2 + 1) * 0.35 + sin(t * 2 * M_PI * 9.1) * 0.12;
			v *= 1.25;
			if (v > 1) v = 1;
			if (v < -1) v = -1;
			out[i] = (int16_t)(v * 9000);
		}
		return held[scopeTG] || lvl[scopeTG] > 0.01f ? cnt : 0;
	}
};

} // namespace zic
