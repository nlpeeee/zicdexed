#include "dx7_voice.h"
#include <math.h>

namespace zd {

static const uint8_t kOpMax[kOpSize] = {
	99, 99, 99, 99, 99, 99, 99, 99, // rates, levels
	99, 99, 99, 3, 3,               // BP, LD, RD, LC, RC
	7, 3, 7,                        // RS, AMS, KVS
	99,                             // OL
	1, 31, 99, 14                   // PM, PC, PF, PD
};

int ParamMax(int offset)
{
	if (offset < 0 || offset >= kVoiceSize) return 0;
	if (offset < kOps * kOpSize) return kOpMax[offset % kOpSize];
	switch (offset) {
	case G_PR1: case G_PR2: case G_PR3: case G_PR4:
	case G_PL1: case G_PL2: case G_PL3: case G_PL4: return 99;
	case G_ALG: return 31;
	case G_FB: return 7;
	case G_OKS: return 1;
	case G_LFS: case G_LFD: case G_LPMD: case G_LAMD: return 99;
	case G_LFKS: return 1;
	case G_LFW: return 5;
	case G_LPMS: return 7;
	case G_TRNSP: return 48;
	default: return 127; // name characters
	}
}

bool Voice::set(int offset, int value)
{
	if (offset < 0 || offset >= kVoiceSize) return false;
	int mx = ParamMax(offset);
	if (value < 0) value = 0;
	if (value > mx) value = mx;
	if (d[offset] == value) return false;
	d[offset] = (uint8_t)value;
	return true;
}

void Voice::name(char out[kNameLen + 1]) const
{
	for (int i = 0; i < kNameLen; i++) {
		uint8_t c = d[G_NAME + i];
		out[i] = (c >= 32 && c < 127) ? (char)c : ' ';
	}
	out[kNameLen] = 0;
}

void Voice::setName(const char* s)
{
	int i = 0;
	for (; i < kNameLen && s[i]; i++) d[G_NAME + i] = (uint8_t)s[i];
	for (; i < kNameLen; i++) d[G_NAME + i] = ' ';
}

Voice Voice::Init()
{
	Voice v;
	memset(v.d, 0, sizeof v.d);
	for (int o = 0; o < kOps; o++) {
		v.op(o, OP_R1) = v.op(o, OP_R2) = v.op(o, OP_R3) = v.op(o, OP_R4) = 99;
		v.op(o, OP_L1) = v.op(o, OP_L2) = v.op(o, OP_L3) = 99;
		v.op(o, OP_L4) = 0;
		v.op(o, OP_BP) = 39;
		v.op(o, OP_OL) = (o == 0) ? 99 : 0;
		v.op(o, OP_PC) = 1;
		v.op(o, OP_PD) = 7;
	}
	v.d[G_PR1] = v.d[G_PR2] = v.d[G_PR3] = v.d[G_PR4] = 99;
	v.d[G_PL1] = v.d[G_PL2] = v.d[G_PL3] = v.d[G_PL4] = 50;
	v.d[G_OKS] = 1;
	v.d[G_LFS] = 35;
	v.d[G_LFKS] = 1;
	v.d[G_LPMS] = 3;
	v.d[G_TRNSP] = 24;
	v.setName("INIT VOICE");
	return v;
}

// Carrier sets per DX7 algorithm (OP1 = bit 0) and the operator with the
// feedback loop. Carrier counts per algorithm are checked in the tests:
// 2 2 2 2 3 3 2 2 2 2 2 2 2 2 2 1 1 1 3 3 4 4 4 5 5 3 3 3 4 4 5 6.
static constexpr uint8_t B(int op) { return (uint8_t)(1u << (op - 1)); }
static const AlgInfo kAlgs[32] = {
	{ uint8_t(B(1) | B(3)), 5 },                              //  1
	{ uint8_t(B(1) | B(3)), 1 },                              //  2
	{ uint8_t(B(1) | B(4)), 5 },                              //  3
	{ uint8_t(B(1) | B(4)), 5 },                              //  4 (feedback crosses 4-5-6)
	{ uint8_t(B(1) | B(3) | B(5)), 5 },                       //  5
	{ uint8_t(B(1) | B(3) | B(5)), 5 },                       //  6 (feedback crosses 5-6)
	{ uint8_t(B(1) | B(3)), 5 },                              //  7
	{ uint8_t(B(1) | B(3)), 3 },                              //  8
	{ uint8_t(B(1) | B(3)), 1 },                              //  9
	{ uint8_t(B(1) | B(4)), 2 },                              // 10
	{ uint8_t(B(1) | B(4)), 5 },                              // 11
	{ uint8_t(B(1) | B(3)), 1 },                              // 12
	{ uint8_t(B(1) | B(3)), 5 },                              // 13
	{ uint8_t(B(1) | B(3)), 5 },                              // 14
	{ uint8_t(B(1) | B(3)), 1 },                              // 15
	{ uint8_t(B(1)), 5 },                                     // 16
	{ uint8_t(B(1)), 1 },                                     // 17
	{ uint8_t(B(1)), 2 },                                     // 18
	{ uint8_t(B(1) | B(4) | B(5)), 5 },                       // 19
	{ uint8_t(B(1) | B(2) | B(4)), 2 },                       // 20
	{ uint8_t(B(1) | B(2) | B(4) | B(5)), 5 },                // 21
	{ uint8_t(B(1) | B(3) | B(4) | B(5)), 5 },                // 22
	{ uint8_t(B(1) | B(2) | B(4) | B(5)), 5 },                // 23
	{ uint8_t(B(1) | B(2) | B(3) | B(4) | B(5)), 5 },         // 24
	{ uint8_t(B(1) | B(2) | B(3) | B(4) | B(5)), 5 },         // 25
	{ uint8_t(B(1) | B(2) | B(4)), 5 },                       // 26
	{ uint8_t(B(1) | B(2) | B(4)), 2 },                       // 27
	{ uint8_t(B(1) | B(3) | B(6)), 4 },                       // 28
	{ uint8_t(B(1) | B(2) | B(3) | B(5)), 5 },                // 29
	{ uint8_t(B(1) | B(2) | B(3) | B(6)), 4 },                // 30
	{ uint8_t(B(1) | B(2) | B(3) | B(4) | B(5)), 5 },         // 31
	{ uint8_t(B(1) | B(2) | B(3) | B(4) | B(5) | B(6)), 5 },  // 32
};

const AlgInfo& Algorithm(int alg)
{
	if (alg < 0) alg = 0;
	if (alg > 31) alg = 31;
	return kAlgs[alg];
}

int CarrierCount(int alg)
{
	int n = 0;
	for (int o = 0; o < kOps; o++) n += IsCarrier(alg, o) ? 1 : 0;
	return n;
}

int RatioHundredths(const Voice& v, int op)
{
	int c = v.op(op, OP_PC), f = v.op(op, OP_PF);
	int base = (c == 0) ? 50 : c * 100;
	return base * (100 + f) / 100;
}

int FixedHz(const Voice& v, int op)
{
	float hz = powf(10.0f, (float)(v.op(op, OP_PC) & 3)) * powf(1.023293f, (float)v.op(op, OP_PF));
	return (int)(hz + 0.5f);
}

} // namespace zd
