//
// mutate.h - MUTATE: random nudge of the current voice (Shift+w).
//
// Touches modulator ratios and levels, feedback and detune. Carrier level and
// ratio are kept so a mutated voice never goes silent or out of tune by accident.
// Every step is small; press again to keep going, COMPARE/REVERT to undo.
//
#pragma once
#include "dx7_voice.h"
#include "rng.h"

namespace zd {

// strength 1..100 (default 40): how many parameters change per press.
// Returns the number of parameters that actually changed.
int Mutate(Voice& v, Rng& rng, int strength = 40);

} // namespace zd
