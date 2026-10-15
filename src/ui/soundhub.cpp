#include "soundhub.h"
#include "zicpages.h"
#include <string.h>

namespace zic {

// ---- adapters from the logic layer's interfaces onto ISynth -----------------

class SoundHub::Sink : public zd::VoiceSink {
public:
	ISynth* syn = nullptr;
	int tg = 0;
	void applyVoice(const zd::Voice& v) override { if (syn) syn->loadVoice(tg, v.d); }
	void noteOn(int note, int vel) override { if (syn) syn->noteOn(tg, note, vel); }
	void noteOff(int note) override { if (syn) syn->noteOff(tg, note); }
};

class SoundHub::Bus : public zd::ParamBus {
public:
	ISynth* syn = nullptr;
	int get(zd::BusParam p, int tg) override
	{
		if (!syn) return 0;
		if (p == zd::BP_TG_VOLUME) return syn->get(P_VOLUME, tg);
		return syn->getFx(Fx(p));
	}
	void set(zd::BusParam p, int tg, int v) override
	{
		if (!syn) return;
		if (p == zd::BP_TG_VOLUME) syn->set(P_VOLUME, tg, v);
		else syn->setFx(Fx(p), v);
	}
	int max(zd::BusParam p) override
	{
		return p == zd::BP_TG_VOLUME ? 127 : (p == zd::BP_COMPRESSOR ? 1 : 99);
	}
private:
	static FxParam Fx(zd::BusParam p)
	{
		switch (p) {
		case zd::BP_REVERB_SIZE: return FX_REVERB_SIZE;
		case zd::BP_REVERB_DAMP: return FX_REVERB_HIGHDAMP;
		case zd::BP_REVERB_LEVEL: return FX_REVERB_LEVEL;
		default: return FX_COMPRESSOR;
		}
	}
};

class SoundHub::Bank : public zd::BankSource {
public:
	ISynth* syn = nullptr;
	int bankCount() override { return syn ? syn->bankCount() : 0; }
	bool bankValid(int b) override { return syn && syn->bankValid(b); }
	void bankName(int b, char out[zd::kNameLen + 1]) override
	{
		char tmp[32] = {0};
		if (syn) syn->bankName(b, tmp);
		copyStr(out, tmp, zd::kNameLen + 1);
	}
	bool voiceName(int b, int i, char out[zd::kNameLen + 1]) override
	{
		char tmp[kNameLen + 1] = {0};
		if (!syn || !syn->voiceNameIn(b, i, tmp)) return false;
		trimName(out, tmp);
		return true;
	}
	bool readVoice(int b, int i, zd::Voice& v) override { return syn && syn->readVoiceIn(b, i, v.d); }
};

struct SoundHub::Impl {
	Sink sink[kMaxTG];
	Bus bus;
	Bank bank;
};

SoundHub::SoundHub() : d_(new Impl)
{
	for (int i = 0; i < kMaxTG; i++) {
		d_->sink[i].tg = i;
		sess_[i] = zd::EditSession(&d_->sink[i], 0x1234u + (uint32_t)i * 7919u);
		known_[i] = false;
	}
	browse_.setSource(&d_->bank);
	ctx_.bus = &d_->bus;
	ctx_.keys = &keys_;
	ctx_.browse = &browse_;
	ctx_.session = &sess_[0];
	memset(echo_, 0, sizeof echo_);
}

SoundHub& SoundHub::get()
{
	static SoundHub h;
	return h;
}

void SoundHub::bind(Ctx& c)
{
	syn_ = &c.syn;
	for (int i = 0; i < kMaxTG; i++) d_->sink[i].syn = syn_;
	d_->bus.syn = syn_;
	d_->bank.syn = syn_;
	ctx_.tg = c.tg;
	ctx_.shift = c.shift;
	ctx_.session = &sess_[c.tg];
	sync(c.tg);
}

void SoundHub::sync(int tg)
{
	uint8_t cur[kVoiceSize];
	syn_->readVoice(tg, cur);
	if (known_[tg] && memcmp(cur, echo_[tg], kVoiceSize) == 0) return;
	// The synth's voice is not the one we last left there: take it as the new
	// stored and edit voice. Nothing is pushed back.
	zd::Voice v;
	memcpy(v.d, cur, kVoiceSize);
	zd::EditSession& s = sess_[tg];
	s.setSink(nullptr);
	s.load(v);
	s.clearOpMute();
	s.setSink(&d_->sink[tg]);
	memcpy(echo_[tg], cur, kVoiceSize);
	known_[tg] = true;
}

void SoundHub::settle(int tg)
{
	syn_->readVoice(tg, echo_[tg]);
	known_[tg] = true;
}

// ---- Shift+q / w / e ------------------------------------------------------------

bool SoundDesignShiftKey(Ctx& c, int key)
{
	if (key != K_Q && key != K_W && key != K_E && key != K_R && key != K_D && key != K_F) return false;
	if (key == K_R) { c.ui.gotoSub(SLOT_MOD, 2); return true; }       // FX
	SoundHub& h = SoundHub::get();
	h.bind(c);
	zd::EditSession& s = h.session();
	if (key == K_D) {                                                // SAVE VOICE
		// the edit voice, not what is sounding: no op mutes in it
		int slot = c.syn.saveVoice(s.edit().d);
		char msg[32];
		if (slot >= 0) {
			s.markSaved();
			copyStr(msg, "SAVED USER ", sizeof msg);
			fmtInt(msg + 11, slot + 1);
			copyStr(msg + (slot + 1 < 10 ? 12 : 13), " - RESTART TO USE", sizeof msg - 13);
		} else {
			copyStr(msg, slot == -2 ? "USER BANK FULL" : (slot == -1 ? "SAVE VOICE - NOT IN THIS BUILD" : "SAVE FAILED"), sizeof msg);
		}
		c.ui.toast(msg);
		return true;
	}
	if (key == K_F) {                                                // SAVE PERFORMANCE
		c.ui.toast(c.syn.savePerformance() ? "PERFORMANCE SAVED" : "NO PERFORMANCE FOLDER ON SD");
		return true;
	}
	switch (key) {
	case K_Q:
		s.init();
		c.ui.toast("INIT VOICE");
		break;
	case K_W:
		s.mutate();
		c.ui.toast("MUTATE");
		break;
	default: {
		int op = h.ui().op;
		s.toggleOpMute(op);
		char msg[24];
		copyStr(msg, "OP ", sizeof msg);
		fmtInt(msg + 3, op + 1);
		copyStr(msg + 4, s.opMuted(op) ? " MUTED" : " ON", sizeof msg - 4);
		c.ui.toast(msg);
		break;
	}
	}
	h.settle(c.tg);
	c.ui.redrawAll();
	return true;
}

} // namespace zic
