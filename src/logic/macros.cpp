#include "macros.h"

namespace zd {

static int clampi(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
// a + (b - a) * pct / 100, rounded toward b
static int lerp100(int a, int b, int pct) { return a + ((b - a) * pct + (b >= a ? 50 : -50)) / 100; }

void MacroState::rebase(const Voice& v)
{
	base_ = v;
	for (int m = 0; m < kMacroCount; m++) amt_[m] = 0;
}

void MacroState::set(int m, int amount)
{
	if (m < 0 || m >= kMacroCount) return;
	amt_[m] = (int8_t)clampi(amount, kMacroMin[m], kMacroMax[m]);
}

bool MacroState::active() const
{
	for (int m = 0; m < kMacroCount; m++) if (amt_[m]) return true;
	return false;
}

Voice MacroState::render() const
{
	Voice v = base_;
	const int alg = v.algorithm();

	// BRIGHT: modulator output levels up/down (+-20 steps at the ends). Operators
	// that are switched off (level 0) stay off. Algorithm 32 has no modulators.
	if (amt_[M_BRIGHT]) {
		for (int o = 0; o < kOps; o++) {
			if (IsCarrier(alg, o) || v.op(o, OP_OL) == 0) continue;
			v.set(OpOff(o, OP_OL), v.op(o, OP_OL) + amt_[M_BRIGHT] * 2 / 5);
		}
	}

	// SNARL: feedback toward max, and the modulators drift apart (detune spread
	// and a slight inharmonic ratio fine offset on every other modulator).
	if (amt_[M_SNARL]) {
		const int s = amt_[M_SNARL];
		int fb = lerp100(v.feedback(), 7, s);
		v.set(G_FB, fb);
		int k = 0;
		for (int o = 0; o < kOps; o++) {
			if (IsCarrier(alg, o) || v.op(o, OP_OL) == 0) continue;
			int dir = (k++ & 1) ? 1 : -1;
			v.set(OpOff(o, OP_PD), v.op(o, OP_PD) + dir * s * 4 / 99);
			if (v.op(o, OP_PM) == 0 && (k & 1)) v.set(OpOff(o, OP_PF), v.op(o, OP_PF) + s * 15 / 99);
		}
	}

	// BITE: modulators get a fast, hard attack that settles to a lower sustain,
	// so the start of the note is brighter than the body.
	if (amt_[M_BITE]) {
		const int b = amt_[M_BITE];
		for (int o = 0; o < kOps; o++) {
			if (IsCarrier(alg, o) || v.op(o, OP_OL) == 0) continue;
			v.set(OpOff(o, OP_R1), lerp100(v.op(o, OP_R1), 99, b));
			v.set(OpOff(o, OP_R2), lerp100(v.op(o, OP_R2), 80, b * 7 / 10));
			int sustain = v.op(o, OP_L2) * 55 / 100;
			if (sustain < v.op(o, OP_L3)) v.set(OpOff(o, OP_L3), lerp100(v.op(o, OP_L3), sustain, b));
		}
	}

	// TAIL: release time. Positive = longer. Carriers get the full amount,
	// modulators half so the tail does not turn into a noisy smear.
	if (amt_[M_TAIL]) {
		for (int o = 0; o < kOps; o++) {
			if (v.op(o, OP_OL) == 0) continue;
			int w = IsCarrier(alg, o) ? 90 : 45;
			v.set(OpOff(o, OP_R4), v.op(o, OP_R4) - amt_[M_TAIL] * w / 100);
		}
	}

	// VEL: key velocity sensitivity, modulators first so harder hits get brighter.
	if (amt_[M_VEL]) {
		for (int o = 0; o < kOps; o++) {
			if (v.op(o, OP_OL) == 0) continue;
			int pct = IsCarrier(alg, o) ? amt_[M_VEL] / 2 : amt_[M_VEL];
			v.set(OpOff(o, OP_KVS), lerp100(v.op(o, OP_KVS), 7, pct));
		}
	}
	return v;
}

} // namespace zd
