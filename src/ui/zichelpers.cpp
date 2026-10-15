#include "zicpages.h"
#include "zicsynth.h"

namespace zic {

static const ParamRange kRanges[kTGParamCount] = {
	{ 0, 127 },    // P_VOLUME
	{ 0, 127 },    // P_PAN
	{ 0, 99 },     // P_REVERB
	{ 0, 99 },     // P_CUTOFF
	{ 0, 99 },     // P_RESO
	{ -99, 99 },   // P_DETUNE
	{ 0, 17 },     // P_CHANNEL
	{ 0, 31 },     // P_PROGRAM
	{ 0, 16383 },  // P_BANK
	{ 0, 1 },      // P_MONO
	{ 0, 99 },     // P_PORTA_TIME
	{ 0, 1 },      // P_PORTA_MODE
	{ 0, 12 },     // P_PB_RANGE
	{ -24, 24 },   // P_TRANSPOSE
};

const ParamRange& paramRange(TGParam p) { return kRanges[p]; }

void copyStr(char* dst, const char* src, int cap)
{
	int i = 0;
	for (; src[i] && i < cap - 1; i++) dst[i] = src[i];
	dst[i] = 0;
}

void fmtInt(char* out, int v, bool plus)
{
	char tmp[12];
	int n = 0, neg = v < 0;
	unsigned u = neg ? (unsigned)-v : (unsigned)v;
	do { tmp[n++] = (char)('0' + u % 10); u /= 10; } while (u);
	int o = 0;
	if (neg) out[o++] = '-'; else if (plus && v > 0) out[o++] = '+';
	while (n) out[o++] = tmp[--n];
	out[o] = 0;
}

void fmtPan(char* out, int pan)
{
	int d = pan - 64;
	if (d > -2 && d < 2) { copyStr(out, "C", 12); return; }
	out[0] = d < 0 ? 'L' : 'R';
	fmtInt(out + 1, d < 0 ? -d : d);
}

void fmtChannel(char* out, int ch)
{
	if (ch == 16) copyStr(out, "OMNI", 12);
	else if (ch >= 17) copyStr(out, "OFF", 12);
	else fmtInt(out, ch + 1);
}

void trimName(char* dst, const char* src)
{
	int n = 0;
	while (src[n] && n < kNameLen) { dst[n] = src[n]; n++; }
	while (n > 0 && (dst[n - 1] == ' ' || (unsigned char)dst[n - 1] < 32)) n--;
	dst[n] = 0;
}

} // namespace zic
