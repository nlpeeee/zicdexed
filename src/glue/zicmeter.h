//
// zicmeter.h - live levels and a scope tap, written by the audio core and read
// by the UI. Lock-free: single writer (core 1), single reader (UI), plain floats.
//
#ifndef _zicmeter_h
#define _zicmeter_h

#include <stdint.h>

struct ZicMeter
{
	static const unsigned Taps = 1024;		// scope ring, decimated by 4

	volatile float peak[8];				// peak since the UI last read it
	volatile int16_t ring[Taps];
	volatile unsigned head;				// next write position
	volatile int scopeTG;				// whose output feeds the ring
};

extern ZicMeter g_ZicMeter;

// Called once per audio chunk from CMiniDexed::ProcessSound with the per-TG
// output buffers (nTG rows of `stride` floats, `nFrames` valid).
void ZicMeterFeed (const float *pLevels, unsigned nStride, unsigned nTGs, unsigned nFrames);

#endif
