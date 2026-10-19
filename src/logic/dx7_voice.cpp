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

// The 32 DX7 algorithms as the Dexed/MSFA engine encodes them (Synth_Dexed
// fm_core.cpp, Apache-2.0, originally Google music-synthesizer-for-android).
// Entry i is processed in order, so entry 0 is OP6 and entry 5 is OP1.
// Bits: 0..1 output bus (0 = main output), 2 add (to the bus, or to the output
// when the bus is 0), 4..5 input bus, 6 feedback in, 7 feedback out.
static const uint8_t kEngine[32][6] = {
	{ 0xc1, 0x11, 0x11, 0x14, 0x01, 0x14 }, { 0x01, 0x11, 0x11, 0x14, 0xc1, 0x14 },
	{ 0xc1, 0x11, 0x14, 0x01, 0x11, 0x14 }, { 0xc1, 0x11, 0x94, 0x01, 0x11, 0x14 },
	{ 0xc1, 0x14, 0x01, 0x14, 0x01, 0x14 }, { 0xc1, 0x94, 0x01, 0x14, 0x01, 0x14 },
	{ 0xc1, 0x11, 0x05, 0x14, 0x01, 0x14 }, { 0x01, 0x11, 0xc5, 0x14, 0x01, 0x14 },
	{ 0x01, 0x11, 0x05, 0x14, 0xc1, 0x14 }, { 0x01, 0x05, 0x14, 0xc1, 0x11, 0x14 },
	{ 0xc1, 0x05, 0x14, 0x01, 0x11, 0x14 }, { 0x01, 0x05, 0x05, 0x14, 0xc1, 0x14 },
	{ 0xc1, 0x05, 0x05, 0x14, 0x01, 0x14 }, { 0xc1, 0x05, 0x11, 0x14, 0x01, 0x14 },
	{ 0x01, 0x05, 0x11, 0x14, 0xc1, 0x14 }, { 0xc1, 0x11, 0x02, 0x25, 0x05, 0x14 },
	{ 0x01, 0x11, 0x02, 0x25, 0xc5, 0x14 }, { 0x01, 0x11, 0x11, 0xc5, 0x05, 0x14 },
	{ 0xc1, 0x14, 0x14, 0x01, 0x11, 0x14 }, { 0x01, 0x05, 0x14, 0xc1, 0x14, 0x14 },
	{ 0x01, 0x14, 0x14, 0xc1, 0x14, 0x14 }, { 0xc1, 0x14, 0x14, 0x14, 0x01, 0x14 },
	{ 0xc1, 0x14, 0x14, 0x01, 0x14, 0x04 }, { 0xc1, 0x14, 0x14, 0x14, 0x04, 0x04 },
	{ 0xc1, 0x14, 0x14, 0x04, 0x04, 0x04 }, { 0xc1, 0x05, 0x14, 0x01, 0x14, 0x04 },
	{ 0x01, 0x05, 0x14, 0xc1, 0x14, 0x04 }, { 0x04, 0xc1, 0x11, 0x14, 0x01, 0x14 },
	{ 0xc1, 0x14, 0x01, 0x14, 0x04, 0x04 }, { 0x04, 0xc1, 0x11, 0x14, 0x04, 0x04 },
	{ 0xc1, 0x14, 0x04, 0x04, 0x04, 0x04 }, { 0xc4, 0x04, 0x04, 0x04, 0x04, 0x04 },
};

static AlgInfo gAlgs[32];
static bool gAlgsBuilt = false;

static void BuildAlgorithms()
{
	for (int a = 0; a < 32; a++) {
		AlgInfo& r = gAlgs[a];
		memset(&r, 0, sizeof r);
		uint8_t contrib[3] = { 0, 0, 0 }; // ops currently writing each bus
		for (int i = 0; i < kOps; i++) {
			const int op = kOps - 1 - i; // 0-based op number (OP1 = 0)
			const uint8_t f = kEngine[a][i];
			const int in = (f >> 4) & 3, out = f & 3;
			if (in) r.mods[op] = contrib[in];
			if (out == 0) {
				if (f & 4) r.carrierMask |= (uint8_t)(1u << op);
			} else if (f & 4) {
				contrib[out] |= (uint8_t)(1u << op); // adds to what is already on the bus
			} else {
				contrib[out] = (uint8_t)(1u << op);  // replaces it
			}
			if (f & 0x40) r.feedbackOp = (uint8_t)op;
			if (f & 0x80) r.feedbackOut = (uint8_t)op;
		}
	}
	gAlgsBuilt = true;
}

const AlgInfo& Algorithm(int alg)
{
	if (alg < 0) alg = 0;
	if (alg > 31) alg = 31;
	if (!gAlgsBuilt) BuildAlgorithms();
	return gAlgs[alg];
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
