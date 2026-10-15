//
// zicsynth.h - everything the UI needs from the synth engine, as one interface.
//
// The kernel implements it on top of MiniDexed (src/glue/zicsynth_dexed.cpp);
// the PC simulator and the tests implement it with a fake. The shapes line up
// with the logic layer's VoiceSink / ParamBus / BankSource so those are thin
// wrappers around this.
//
#pragma once
#include <stdint.h>

namespace zic {

constexpr int kMaxTG = 8;
constexpr int kVoiceSize = 155;      // unpacked DX7 voice (VCED), name at 145..154
constexpr int kNameLen = 10;
constexpr int kBankVoices = 32;

// Per-TG parameters. All values are the engine's own units (see range()).
enum TGParam {
	P_VOLUME,        // 0..127
	P_PAN,           // 0..127, 64 = centre
	P_REVERB,        // 0..99 reverb send
	P_CUTOFF,        // 0..99
	P_RESO,          // 0..99
	P_DETUNE,        // -99..99 master tune
	P_CHANNEL,       // 0..15 = MIDI ch 1..16, 16 = omni, 17 = off
	P_PROGRAM,       // 0..31 in the current bank
	P_BANK,          // 0..16383
	P_MONO,          // 0 poly, 1 mono
	P_PORTA_TIME,    // 0..99
	P_PORTA_MODE,    // 0 retain, 1 follow
	P_PB_RANGE,      // 0..12 semitones
	P_TRANSPOSE,     // -24..24 semitones (voice parameter)
	kTGParamCount
};

struct ParamRange { int min, max; };
const ParamRange& paramRange(TGParam p);

// Effects and other values that are not per TG.
enum FxParam {
	FX_COMPRESSOR,       // 0/1
	FX_REVERB_ENABLE,    // 0/1
	FX_REVERB_SIZE,      // 0..99
	FX_REVERB_HIGHDAMP,  // 0..99
	FX_REVERB_LOWDAMP,   // 0..99
	FX_REVERB_LOWPASS,   // 0..99
	FX_REVERB_DIFFUSION, // 0..99
	FX_REVERB_LEVEL,     // 0..99
	kFxParamCount
};

class ISynth {
public:
	virtual ~ISynth() {}

	virtual int tgCount() = 0;                                  // active tone generators, 1..8
	virtual int get(TGParam p, int tg) = 0;
	virtual void set(TGParam p, int tg, int value) = 0;         // clamps to paramRange()
	virtual int getFx(FxParam p) = 0;
	virtual void setFx(FxParam p, int value) = 0;

	virtual void voiceName(int tg, char out[kNameLen + 1]) = 0;
	virtual void bankName(int bank, char out[32]) = 0;

	// Voices and notes on a TG (the TG's own MIDI channel is used)
	virtual void loadVoice(int tg, const uint8_t v[kVoiceSize]) = 0;
	virtual void readVoice(int tg, uint8_t v[kVoiceSize]) = 0;
	virtual void noteOn(int tg, int note, int velocity) = 0;
	virtual void noteOff(int tg, int note) = 0;
	virtual void panic() = 0;                                   // all notes off, all TGs

	// COMPARE: hold = hear the stored voice, release = get the edit back
	virtual void compareBegin(int tg) = 0;
	virtual void compareEnd(int tg) = 0;
	virtual void revert(int tg) = 0;                            // drop the edit, reload the stored voice

	// Bank reading for BROWSE
	virtual int bankCount() = 0;                                // highest valid bank id + 1
	virtual bool bankValid(int bank) = 0;
	virtual bool voiceNameIn(int bank, int index, char out[kNameLen + 1]) = 0;
	virtual bool readVoiceIn(int bank, int index, uint8_t v[kVoiceSize]) = 0;

	// Mute is a gain stage in front of the TG, independent of the volume lane.
	virtual void setMute(int tg, bool muted) = 0;

	// One line about the network for the Shift screen ("WiFi 192.168.1.5"), or "".
	virtual void netStatus(char out[32]) { out[0] = 0; }

	// Live metering. level() is the peak since the last call, 0..1 (linear).
	virtual float level(int tg) = 0;
	// The TG whose output feeds the scope, and the latest n samples of it,
	// decimated; newest last. Returns how many were written.
	virtual void setScopeTG(int tg) = 0;
	virtual int scope(int16_t* out, int n) = 0;
};

} // namespace zic
