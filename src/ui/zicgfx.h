//
// zicgfx.h - 320x170 RGB565 framebuffer with anti-aliased primitives, bitmap
// text and dirty-rectangle tracking. No Circle dependency: it builds on a PC
// for the screenshot tests and in the kernel.
//
// Colours are 0xRRGGBB. Every draw call records the pixels it touched; the
// glue then pushes only those rectangles to the panel, a few rows per call.
//
#pragma once
#include "zicfont.h"
#include <stdint.h>

namespace zic {

constexpr int W = 320;
constexpr int H = 170;

typedef uint32_t Color;

inline Color rgb(int r, int g, int b) { return ((Color)r << 16) | ((Color)g << 8) | (Color)b; }
Color mix(Color a, Color b, int t256);        // a at t=0, b at t=256
inline Color dim(Color c, int k256) { return mix(0, c, k256); }

struct Rect { int x, y, w, h; };

enum Align { AL_LEFT, AL_CENTER, AL_RIGHT };

class Gfx {
public:
	Gfx();

	// ---- drawing
	void clear(Color c);
	void fillRect(int x, int y, int w, int h, Color c);
	void hline(int x, int y, int w, Color c) { fillRect(x, y, w, 1, c); }
	void vline(int x, int y, int h, Color c) { fillRect(x, y, 1, h, c); }
	// Anti-aliased rounded rectangle: filled (fill != NoColor) and/or outlined.
	void roundRect(float x, float y, float w, float h, float r, Color fill, Color line = NoColor, float lw = 1.f);
	// Anti-aliased line with round caps, width in px.
	void line(float x0, float y0, float x1, float y1, float width, Color c);
	void disc(float cx, float cy, float r, Color fill, Color line = NoColor, float lw = 1.f);
	void polyline(const float* xy, int n, float width, Color c);

	// ---- text
	// Pen at (x,y) = top of the line box. Returns the advance in px.
	// track = extra letter spacing in px. Lower case is folded to upper if the
	// font has no lower case (Huge).
	float text(int x, int y, const char* s, const Font& f, Color c, float track = 0.f);
	float textWidth(const char* s, const Font& f, float track = 0.f) const;
	void textAligned(int x, int y, int w, const char* s, const Font& f, Color c, Align a, float track = 0.f);

	// Restrict drawing (and dirty tracking) to a rectangle, e.g. to repaint a
	// strip of a bar without touching the rest of it.
	void setClip(int x, int y, int w, int h);
	void clearClip() { cx0_ = 0; cy0_ = 0; cx1_ = W; cy1_ = H; }

	// ---- dirty rectangles
	void invalidate(int x, int y, int w, int h) { touch(x, y, x + w, y + h); }
	void invalidateAll() { touch(0, 0, W, H); }
	bool dirty() const { return nDirty_ > 0; }
	void clearDirty() { nDirty_ = 0; }
	int dirtyPixels() const;

	// Take the next piece of dirty area: at most `maxPixels` pixels, whole
	// rows of one rectangle, converted to the panel's byte order into `out`
	// (needs maxPixels + W entries). Returns false when nothing is pending.
	bool takeChunk(Rect& r, uint16_t* out, int maxPixels);

	// Direct access for tests and screenshots.
	const uint16_t* pixels() const { return fb_; }
	Color pixel(int x, int y) const;

	static const Color NoColor = 0xFF000000u;

private:
	void touch(int x0, int y0, int x1, int y1);
	void blend(int x, int y, Color c, int a256);
	void put(int x, int y, Color c) { fb_[y * W + x] = pack(c); }
	static uint16_t pack(Color c) { return (uint16_t)(((c >> 8) & 0xF800) | ((c >> 5) & 0x07E0) | ((c >> 3) & 0x1F)); }
	static Color unpack(uint16_t v);
	void rrCoverage(float x, float y, float w, float h, float r, Color fill, Color line, float lw);

	uint16_t fb_[W * H];
	static const int MaxDirty = 16;
	Rect dirty_[MaxDirty];
	int nDirty_;
	int cx0_, cy0_, cx1_, cy1_;   // clip rectangle
};

} // namespace zic
