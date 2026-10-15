//
// dx7_voice.h - DX7 voice (VCED, 155 bytes) layout, ranges and algorithm tables.
//
// Pure logic: no Circle/MiniDexed dependencies, so it builds and tests on a PC.
//
#pragma once
#include <stdint.h>
#include <string.h>

namespace zd {

constexpr int kVoiceSize = 155;
constexpr int kOps = 6;
constexpr int kOpSize = 21;
constexpr int kNameLen = 10;

// Offsets inside one operator block (21 bytes)
enum OpParam {
	OP_R1 = 0, OP_R2, OP_R3, OP_R4,    // EG rates
	OP_L1, OP_L2, OP_L3, OP_L4,        // EG levels
	OP_BP, OP_LD, OP_RD, OP_LC, OP_RC, // breakpoint, left/right depth, left/right curve
	OP_RS, OP_AMS, OP_KVS,             // rate scaling, amp mod sens, key velocity sens
	OP_OL,                             // output level
	OP_PM, OP_PC, OP_PF, OP_PD         // mode (0 ratio, 1 fixed), coarse, fine, detune (7 = centre)
};

// Global voice offsets (from the start of the 155 bytes)
enum GlobalParam {
	G_PR1 = 126, G_PR2, G_PR3, G_PR4,  // pitch EG rates
	G_PL1, G_PL2, G_PL3, G_PL4,        // pitch EG levels (50 = centre)
	G_ALG = 134, G_FB, G_OKS,
	G_LFS, G_LFD, G_LPMD, G_LAMD, G_LFKS, G_LFW, G_LPMS,
	G_TRNSP = 144,
	G_NAME = 145
};

// Op 0 = OP1 ... op 5 = OP6. The VCED stores OP6 first.
inline int OpBase(int op) { return (kOps - 1 - op) * kOpSize; }
inline int OpOff(int op, OpParam p) { return OpBase(op) + p; }

// Maximum value of a voice parameter (minimum is always 0).
int ParamMax(int offset);

struct Voice {
	uint8_t d[kVoiceSize];

	uint8_t& op(int o, OpParam p) { return d[OpOff(o, p)]; }
	uint8_t op(int o, OpParam p) const { return d[OpOff(o, p)]; }
	int algorithm() const { return d[G_ALG]; }
	int feedback() const { return d[G_FB]; }
	void name(char out[kNameLen + 1]) const;
	void setName(const char* s);
	// Parameter write with clamping to the DX7 range. Returns true if the value changed.
	bool set(int offset, int value);
	bool operator==(const Voice& o) const { return memcmp(d, o.d, sizeof d) == 0; }
	bool operator!=(const Voice& o) const { return !(*this == o); }

	static Voice Init(); // DX7 "INIT VOICE"
};

// Algorithm facts (algorithm index 0..31 = DX7 algorithm 1..32)
struct AlgInfo {
	uint8_t carrierMask; // bit n set = op n (0 = OP1) is a carrier
	uint8_t feedbackOp;  // op index (0..5) where the feedback loop is read (FB_IN)
	uint8_t feedbackOut; // op index (0..5) whose output feeds the loop; same as feedbackOp except algorithms 4 and 6
	uint8_t mods[kOps];  // mods[n] = bit mask of the ops that modulate op n
};
const AlgInfo& Algorithm(int alg);
inline bool IsCarrier(int alg, int op) { return (Algorithm(alg).carrierMask >> op) & 1; }
int CarrierCount(int alg);

// Operator frequency, integer maths so it works without float formatting.
// Ratio mode: hundredths of the ratio (50 = x0.50, 350 = x3.50).
int RatioHundredths(const Voice& v, int op);
// Fixed mode: frequency in Hz.
int FixedHz(const Voice& v, int op);

} // namespace zd
