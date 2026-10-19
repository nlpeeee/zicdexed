#include "edit_session.h"

namespace zd {

void EditSession::push()
{
	if (sink_) sink_->applyVoice(effective());
}

Voice EditSession::effective() const
{
	Voice v = comparing_ ? stored_ : edit_;
	for (int o = 0; o < kOps; o++)
		if ((opMute_ >> o) & 1) v.op(o, OP_OL) = 0;
	return v;
}

void EditSession::remember(UndoKind k)
{
	undoStored_ = stored_;
	undoEdit_ = edit_;
	undoKind_ = k;
}

EditSession::UndoKind EditSession::undo()
{
	const UndoKind k = undoKind_;
	if (k == UNDO_NONE || locked()) return UNDO_NONE;
	stored_ = undoStored_;
	edit_ = undoEdit_;
	undoKind_ = UNDO_NONE;
	macros_.rebase(edit_);
	push();
	return k;
}

void EditSession::load(const Voice& v)
{
	if (playing_) playUp();
	comparing_ = false;
	if (v != edit_ || v != stored_) remember(UNDO_LOAD);
	stored_ = edit_ = v;
	macros_.rebase(v);
	push();
}

void EditSession::markSaved()
{
	undoKind_ = UNDO_NONE;
	stored_ = edit_;
}

bool EditSession::setParam(int offset, int value, bool bakeMacros)
{
	if (locked()) return false;
	if (bakeMacros && macros_.active()) macros_.rebase(edit_);
	bool changed = edit_.set(offset, value);
	if (changed) {
		if (undoKind_ == UNDO_LOAD) undoKind_ = UNDO_NONE;   // tweaks after a load: undo would jump back a voice
		// A direct edit must not be undone by the macros, so the baseline follows it.
		if (bakeMacros) macros_.rebase(edit_);
		else macros_.setBase(offset, value);
		push();
	}
	return changed;
}

void EditSession::setMacro(int m, int amount)
{
	if (locked()) return;
	macros_.set(m, amount);
	Voice v = macros_.render();
	if (v != edit_) {
		if (undoKind_ == UNDO_LOAD) undoKind_ = UNDO_NONE;
		edit_ = v;
		push();
	}
}

void EditSession::init()
{
	if (locked()) return;
	if (edit_ != Voice::Init()) remember(UNDO_INIT);
	edit_ = Voice::Init();
	macros_.rebase(edit_);
	push();
}

void EditSession::mutate(int strength)
{
	if (locked()) return;
	Voice v = edit_;
	if (Mutate(v, rng_, strength) == 0) return;
	remember(UNDO_MUTATE);
	edit_ = v;
	macros_.rebase(edit_);
	push();
}

void EditSession::revert()
{
	if (locked()) return;
	edit_ = stored_;
	macros_.rebase(edit_);
	push();
}

void EditSession::toggleOpMute(int op)
{
	if (op < 0 || op >= kOps) return;
	opMute_ ^= (uint8_t)(1u << op);
	push();
}

void EditSession::compareBegin()
{
	if (comparing_) return;
	comparing_ = true;
	push();
}

void EditSession::compareEnd()
{
	if (!comparing_) return;
	comparing_ = false;
	push();
}

void EditSession::playDown(int note, int velocity)
{
	if (playing_) return;
	playing_ = true;
	playNote_ = note;
	if (sink_) sink_->noteOn(note, velocity);
}

void EditSession::playUp()
{
	if (!playing_) return;
	playing_ = false;
	if (sink_) sink_->noteOff(playNote_);
}

} // namespace zd
