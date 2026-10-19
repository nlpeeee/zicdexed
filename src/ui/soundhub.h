//
// soundhub.h - glue between the sound-design logic (src/logic, namespace zd) and
// the screen framework: one EditSession per TG, kept in step with the voice the
// synth is really playing, plus the shared state of the FM/OPS/MOD/BROWSE/KEYS pages.
//
#pragma once
#include "zicpage.h"
#include "zicui.h"
#include "page_model.h"

namespace zic {

class SoundHub {
public:
	static SoundHub& get();

	// Call at the start of every page entry point. Points the logic at this
	// Ctx's synth/TG and adopts the synth's voice if it changed behind our back
	// (COMPARE, revert, program change, SysEx, a browse preview).
	void bind(Ctx& c);
	zd::UiCtx& ui() { return ctx_; }
	zd::EditSession& session() { return *ctx_.session; }
	zd::EditSession& session(int tg) { return sess_[tg]; }
	zd::KeysModel& keys() { return keys_; }
	zd::BrowseModel& browse() { return browse_; }
	ISynth* synth() { return syn_; }

	// After anything that pushed a voice to the synth: remember what the synth
	// now reports, so the next bind() does not mistake our own edit for an outside one.
	void settle(int tg);
	// Forget what we know about a TG (its voice was replaced by someone else).
	void forget(int tg) { known_[tg] = false; }

private:
	SoundHub();
	void sync(int tg);

	class Sink;
	class Bus;
	class Bank;
	struct Impl;
	Impl* d_;
	ISynth* syn_ = nullptr;
	zd::UiCtx ctx_;
	zd::EditSession sess_[kMaxTG];
	zd::KeysModel keys_;
	zd::BrowseModel browse_;
	uint8_t echo_[kMaxTG][kVoiceSize];
	bool known_[kMaxTG];
};

// SoundDesignShiftKey (Shift+q INIT, Shift+w MUTATE, Shift+e OP MUTE) is declared in zicpages.h.

Page* CreateFmPage();       // w: algorithm, feedback, macros
Page* CreateOpsPage();      // e: operators, envelope
Page* CreateModPage();      // r: LFO, pitch EG, FX
Page* CreateBrowsePage();   // d
Page* CreateKeysPage();     // Shift+x

} // namespace zic
