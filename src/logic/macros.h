//
// macros.h - FM page macros (Bright, Snarl, Bite, Tail, Vel).
//
// A macro writes ordinary DX7 parameters, so the result saves as a normal voice.
// MacroState keeps a baseline copy of the voice and the five macro amounts;
// render() recomputes the voice from the baseline every time, so turning a
// macro back to 0 restores the original exactly. Editing a non-macro parameter
// "bakes" the current result into a new baseline and resets the amounts to 0.
//
#pragma once
#include "dx7_voice.h"

namespace zd {

enum Macro { M_BRIGHT, M_SNARL, M_BITE, M_TAIL, M_VEL, kMacroCount };

constexpr int kMacroMin[kMacroCount] = { -50, 0, 0, -50, 0 };
constexpr int kMacroMax[kMacroCount] = {  50, 99, 99, 50, 99 };
inline bool MacroBipolar(int m) { return kMacroMin[m] < 0; }

class MacroState {
public:
	MacroState() { rebase(Voice::Init()); }
	void rebase(const Voice& v);           // new baseline, amounts back to 0
	void set(int m, int amount);           // clamped to the macro's range
	int get(int m) const { return amt_[m]; }
	bool active() const;                   // any amount non-zero
	Voice render() const;                  // baseline + macros
	const Voice& base() const { return base_; }
	// Direct write to the baseline for parameters no macro touches (e.g. key sync).
	void setBase(int offset, int value) { base_.set(offset, value); }
private:
	Voice base_;
	int8_t amt_[kMacroCount];
};

} // namespace zd
