#include "mutate.h"

namespace zd {

static const uint8_t kRatios[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 13 }; // coarse values worth landing on

static bool IsLive(const Voice& v, int o) { return v.op(o, OP_OL) > 0; }

static bool NudgeRatio(Voice& v, Rng& rng, int o)
{
	if (v.op(o, OP_PM) != 0) return false; // fixed-frequency ops are left alone
	if (rng.chance(60)) { // jump to a nearby musical ratio
		int c = v.op(o, OP_PC);
		int pick = kRatios[rng.range(0, (int)(sizeof kRatios) - 1)];
		if (pick > c + 4) pick = c + 4;
		if (pick < c - 4 && c > 4) pick = c - 4;
		bool a = v.set(OpOff(o, OP_PC), pick);
		bool b = v.set(OpOff(o, OP_PF), 0); // keep it clean after a coarse jump
		return a || b;
	}
	return v.set(OpOff(o, OP_PF), v.op(o, OP_PF) + rng.sign() * rng.range(1, 12)); // inharmonic fine offset
}

int Mutate(Voice& v, Rng& rng, int strength)
{
	if (strength < 1) strength = 1;
	if (strength > 100) strength = 100;
	const int alg = v.algorithm();
	const int picks = 1 + strength / 20; // 1..6 changes per press
	int changed = 0;

	for (int n = 0; n < picks; n++) {
		int o = rng.range(0, kOps - 1);
		if (!IsLive(v, o)) continue;
		const bool carrier = IsCarrier(alg, o);
		switch (rng.range(0, 5)) {
		case 0: case 1: // ratio (modulators only)
			if (!carrier) changed += NudgeRatio(v, rng, o);
			break;
		case 2: // level
			if (carrier) { // small trim, never below a floor so the note stays audible
				int nv = v.op(o, OP_OL) + rng.range(-3, 3);
				if (nv < 85) nv = 85;
				changed += v.set(OpOff(o, OP_OL), nv);
			} else {
				changed += v.set(OpOff(o, OP_OL), v.op(o, OP_OL) + rng.sign() * rng.range(4, 14));
			}
			break;
		case 3: // feedback
			changed += v.set(G_FB, v.feedback() + rng.sign() * rng.range(1, 2));
			break;
		case 4: // detune (any op)
			changed += v.set(OpOff(o, OP_PD), v.op(o, OP_PD) + rng.sign() * rng.range(1, 3));
			break;
		case 5: // modulator envelope shape
			if (!carrier) {
				OpParam p = rng.chance(50) ? OP_R2 : OP_L3;
				changed += v.set(OpOff(o, p), v.op(o, p) + rng.sign() * rng.range(5, 20));
			}
			break;
		}
	}
	return changed;
}

} // namespace zd
