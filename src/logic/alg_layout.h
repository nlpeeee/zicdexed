//
// alg_layout.h - box positions for drawing a DX7 algorithm, derived from the
// engine's routing (Algorithm().mods), so all 32 algorithms are drawn correctly.
//
// Carriers sit on the bottom row, ordered by operator number. Every other
// operator sits one row above the highest of its targets, centred over them.
//
#pragma once
#include "dx7_voice.h"

namespace zd {

struct AlgLayout {
	int rows;               // 1..4
	float cols;             // width in box units (a box is 1.0 wide)
	struct Node { float x; int row; } node[kOps]; // x = left edge in box units, row 0 = top
};

void LayoutAlgorithm(int alg, AlgLayout& out);

} // namespace zd
