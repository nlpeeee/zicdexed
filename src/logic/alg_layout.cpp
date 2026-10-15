#include "alg_layout.h"

namespace zd {

void LayoutAlgorithm(int alg, AlgLayout& out)
{
	const AlgInfo& a = Algorithm(alg);

	// rank 0 = carrier; others one above their highest target
	int rank[kOps];
	for (int o = 0; o < kOps; o++) rank[o] = IsCarrier(alg, o) ? 0 : 1;
	for (int pass = 0; pass < kOps; pass++)
		for (int t = 0; t < kOps; t++)
			for (int o = 0; o < kOps; o++)
				if ((a.mods[t] >> o) & 1 && rank[o] < rank[t] + 1) rank[o] = rank[t] + 1;
	int maxRank = 0;
	for (int o = 0; o < kOps; o++) if (rank[o] > maxRank) maxRank = rank[o];

	float x[kOps] = {};
	int nc = 0;
	for (int o = 0; o < kOps; o++) if (IsCarrier(alg, o)) x[o] = (float)nc++;

	for (int r = 1; r <= maxRank; r++) {
		for (int o = 0; o < kOps; o++) {
			if (rank[o] != r) continue;
			float sum = 0;
			int n = 0;
			for (int t = 0; t < kOps; t++)
				if ((a.mods[t] >> o) & 1) { sum += x[t]; n++; }
			x[o] = n ? sum / n : 0;
		}
	}
	// keep boxes in a row apart, left to right
	for (int r = 0; r <= maxRank; r++) {
		int idx[kOps], n = 0;
		for (int o = 0; o < kOps; o++) if (rank[o] == r) idx[n++] = o;
		for (int i = 1; i < n; i++) // insertion sort by x, then by op number
			for (int j = i; j > 0 && x[idx[j]] < x[idx[j - 1]]; j--) { int t = idx[j]; idx[j] = idx[j - 1]; idx[j - 1] = t; }
		for (int i = 1; i < n; i++)
			if (x[idx[i]] < x[idx[i - 1]] + 1.0f) x[idx[i]] = x[idx[i - 1]] + 1.0f;
	}
	float maxx = 0;
	for (int o = 0; o < kOps; o++) if (x[o] > maxx) maxx = x[o];

	out.rows = maxRank + 1;
	out.cols = maxx + 1.0f;
	for (int o = 0; o < kOps; o++) {
		out.node[o].x = x[o];
		out.node[o].row = maxRank - rank[o];
	}
}

} // namespace zd
