//
// zicfont.h - pre-rendered Inter bitmaps (4-bit alpha, two pixels per byte).
// Generated data lives in zicfont_data.cpp (tools/fontgen.py).
//
#pragma once
#include <stdint.h>

namespace zic {

struct Glyph {
	uint32_t off;      // byte offset into Font::bits
	uint8_t w, h;      // bitmap size (0 for blank glyphs such as space)
	int8_t x, y;       // bitmap origin relative to the pen / top of the line
	uint16_t adv16;    // advance in 1/16 px
};

struct Font {
	const Glyph* glyphs;
	const uint8_t* bits;
	uint8_t first, last;
	uint8_t lineHeight;
};

extern const Font fLbl;    // Inter SemiBold 9   - small caps labels
extern const Font fSmall;  // Inter Medium 10
extern const Font fTiny;   // Inter Medium 9
extern const Font fBody;   // Inter SemiBold 12
extern const Font fTitle;  // Inter Display Bold 15
extern const Font fVal;    // Inter Display Bold 21 - lane values
extern const Font fHead;   // Inter Display Black 18
extern const Font fBig;    // Inter Display Black 30
extern const Font fHuge;   // Inter Display Black 44 - voice name (upper case only)

} // namespace zic
