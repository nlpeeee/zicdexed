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
	// Differences confined to muted operators' levels are our own mute, not a foreign voice.
	if (known_[tg] && sess_[tg].opMuteMask()) {
		zd::Voice a, b;
		memcpy(a.d, cur, kVoiceSize);
		memcpy(b.d, echo_[tg], kVoiceSize);
		for (int o = 0; o < 6; o++)
			if (sess_[tg].opMuted(o)) { a.op(o, zd::OP_OL) = 0; b.op(o, zd::OP_OL) = 0; }
		if (memcmp(a.d, b.d, kVoiceSize) == 0) return;
	}
	// The synth's voice is not the one we last left there: take it as the new
	// stored and edit voice. Nothing is pushed back.
	zd::Voice v;
	memcpy(v.d, cur, kVoiceSize);
	zd::EditSession& s = sess_[tg];
	s.setSink(nullptr);
	s.load(v);
	if (!known_[tg]) s.clearUndo();     // first sight of this TG: nothing before it to undo to
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

bool SoundDesignDirty(Ctx& c)
{
	SoundHub& h = SoundHub::get();
	h.bind(c);
	return h.session().dirty();
}

// COMPARE plays the session's stored voice, so it is right even after SAVE VOICE
// (the TG's bank slot may not hold that voice).
bool SoundDesignCompare(Ctx& c, int tg, bool begin)
{
	SoundHub& h = SoundHub::get();
	h.bind(c);
	if (begin) h.session(tg).compareBegin(); else h.session(tg).compareEnd();
	h.settle(tg);
	return true;
}

namespace {

// SAVE PERFORMANCE stores what the synth plays: a muted operator (level 0) would be saved silent.
bool refuseMutedSave(Ctx& c)
{
	SoundHub& h = SoundHub::get();
	const int n = c.syn.tgCount();
	for (int tg = 0; tg < n && tg < kMaxTG; tg++) {
		const uint8_t m = h.session(tg).opMuteMask();
		if (!m) continue;
		int op = 0;
		while (!((m >> op) & 1)) op++;
		char msg[32] = "";
		if (tg != c.tg) { copyStr(msg, "TG ", sizeof msg); fmtInt(msg + 3, tg + 1); strcat(msg, " "); }
		strcat(msg, "OP ");
		fmtInt(msg + strlen(msg), op + 1);
		strcat(msg, tg != c.tg ? " MUTED" : " MUTED - UNMUTE FIRST");
		c.ui.toast(msg);
		return true;
	}
	return false;
}

} // namespace

bool SoundDesignShiftKey(Ctx& c, int key)
{
	if (key != K_Q && key != K_W && key != K_E && key != K_R && key != K_D && key != K_F && key != K_C) return false;
	if (key == K_R) { c.ui.gotoSub(SLOT_MOD, 2); return true; }       // FX
	SoundHub& h = SoundHub::get();
	h.bind(c);
	zd::EditSession& s = h.session();
	if (key == K_D) { OpenSaveScreen(c); return true; }              // SAVE VOICE: name it, then save
	if (key == K_C) {                                                // UNDO the last INIT/MUTATE/load, then REVERT
		static const char* const kUndone[] = { "", "UNDID INIT", "UNDID MUTATE", "UNDID LOAD" };
		const zd::EditSession::UndoKind k = s.undo();
		if (k != zd::EditSession::UNDO_NONE) {
			char msg[32];
			copyStr(msg, kUndone[k], sizeof msg);
			if (s.dirty()) strcat(msg, " - AGAIN = REVERT");
			c.ui.toast(msg);
		} else if (s.dirty()) {
			s.revert();
			c.ui.toast("REVERTED");
		} else {
			c.ui.toast("NOTHING TO REVERT");
		}
		h.settle(c.tg);
		c.ui.redrawAll();
		return true;
	}
	if (key == K_F) {                                                // SAVE PERFORMANCE
		if (refuseMutedSave(c)) return true;
		if (c.syn.savePerformance()) c.ui.toast("PERFORMANCE SAVED");
		else c.ui.toastError("NO PERFORMANCE FOLDER ON SD");
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
