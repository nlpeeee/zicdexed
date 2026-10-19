#pragma once
#include <stdint.h>

namespace zd {

// Small deterministic PRNG (xorshift32) so MUTATE is reproducible in tests.
class Rng {
public:
	explicit Rng(uint32_t seed = 0x2545F491u) : s_(seed ? seed : 1u) {}
	uint32_t next() { s_ ^= s_ << 13; s_ ^= s_ >> 17; s_ ^= s_ << 5; return s_; }
	// Uniform integer in [lo, hi]
	int range(int lo, int hi) { return lo + (int)(next() % (uint32_t)(hi - lo + 1)); }
	// +1 or -1
	int sign() { return (next() & 0x100) ? 1 : -1; }
	bool chance(int percent) { return range(0, 99) < percent; }
private:
	uint32_t s_;
};

} // namespace zd
