//
// edit_session.h - the voice being edited on one TG, and the sound-design actions
// around it: PLAY, COMPARE, REVERT, INIT, MUTATE, OP MUTE.
//
// The session owns three things: the stored voice (what is on the TG's bank slot),
// the edit voice (stored + changes, with macros on top) and the op-mute mask.
// Whatever the synth should be playing is effective(); every change pushes it
// through the VoiceSink. The framework implements VoiceSink on top of MiniDexed.
//
#pragma once
#include "dx7_voice.h"
#include "macros.h"
#include "mutate.h"

namespace zd {

class VoiceSink {
public:
	virtual ~VoiceSink() {}
	virtual void applyVoice(const Voice& v) = 0;        // load into the TG's engine
	virtual void noteOn(int note, int velocity) = 0;    // on the TG's MIDI channel
	virtual void noteOff(int note) = 0;
};

class EditSession {
public:
	explicit EditSession(VoiceSink* sink = nullptr, uint32_t seed = 1) : sink_(sink), rng_(seed) { load(Voice::Init()); undoKind_ = UNDO_NONE; }
	void setSink(VoiceSink* s) { sink_ = s; }

	// New voice from the bank/browser/SysEx: becomes both stored and edit.
	void load(const Voice& v);
	// The edit voice was saved to the bank: it is the new stored voice.
	void markSaved();

	const Voice& edit() const { return edit_; }
	const Voice& stored() const { return stored_; }
	bool dirty() const { return edit_ != stored_; }
	bool comparing() const { return comparing_; }
	// What the synth should be playing right now.
	Voice effective() const;

	// Parameter access for the page models. Non-macro edits bake macros first.
	int param(int offset) const { return edit_.d[offset]; }
	bool setParam(int offset, int value, bool bakeMacros = true);
	int macro(int m) const { return macros_.get(m); }
	void setMacro(int m, int amount);

	// Sound-design actions. All are ignored while COMPARE is held (like the DX7).
	void init();
	void mutate(int strength = 40);
	void revert();
	// One undo slot: the voice (stored + edit) before the last INIT, MUTATE or load.
	// Saving the voice clears it.
	enum UndoKind { UNDO_NONE, UNDO_INIT, UNDO_MUTATE, UNDO_LOAD };
	UndoKind undoKind() const { return undoKind_; }
	UndoKind undo();                      // returns what was undone, UNDO_NONE if nothing
	void clearUndo() { undoKind_ = UNDO_NONE; }
	void toggleOpMute(int op);
	bool opMuted(int op) const { return (opMute_ >> op) & 1; }
	void clearOpMute() { opMute_ = 0; push(); }
	uint8_t opMuteMask() const { return opMute_; }

	// COMPARE: hold to hear the stored voice, release to get the edit back.
	void compareBegin();
	void compareEnd();

	// PLAY: hold to sound a note on the current TG.
	void playDown(int note = 60, int velocity = 100);
	void playUp();
	bool playing() const { return playing_; }

private:
	void push();
	void remember(UndoKind k);
	bool locked() const { return comparing_; }

	VoiceSink* sink_;
	Rng rng_;
	Voice stored_, edit_;
	Voice undoStored_, undoEdit_;
	UndoKind undoKind_ = UNDO_NONE;
	MacroState macros_;
	uint8_t opMute_ = 0;
	bool comparing_ = false;
	bool playing_ = false;
	int playNote_ = 60;
};

} // namespace zd
