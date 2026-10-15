#include "zicgfx.h"
#include <math.h>
#include <string.h>

namespace zic {

static inline int imin(int a, int b) { return a < b ? a : b; }
static inline int imax(int a, int b) { return a > b ? a : b; }
static inline float fclamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

Color mix(Color a, Color b, int t)
{
	if (t <= 0) return a & 0xFFFFFF;
	if (t >= 256) return b & 0xFFFFFF;
	int ar = (a >> 16) & 255, ag = (a >> 8) & 255, ab = a & 255;
	int br = (b >> 16) & 255, bg = (b >> 8) & 255, bb = b & 255;
	return rgb(ar + (((br - ar) * t) >> 8), ag + (((bg - ag) * t) >> 8), ab + (((bb - ab) * t) >> 8));
}

Color Gfx::unpack(uint16_t v)
{
	int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
	return rgb((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2));
}

Gfx::Gfx() : nDirty_(0), cx0_(0), cy0_(0), cx1_(W), cy1_(H)
{
	memset(fb_, 0, sizeof fb_);
}

Color Gfx::pixel(int x, int y) const { return unpack(fb_[y * W + x]); }

// ------------------------------------------------------------ dirty rectangles
static int area(int x0, int y0, int x1, int y1) { return (x1 - x0) * (y1 - y0); }

void Gfx::setClip(int x, int y, int w, int h)
{
	cx0_ = imax(x, 0); cy0_ = imax(y, 0); cx1_ = imin(x + w, W); cy1_ = imin(y + h, H);
}

void Gfx::touch(int x0, int y0, int x1, int y1)
{
	x0 = imax(x0, cx0_); y0 = imax(y0, cy0_); x1 = imin(x1, cx1_); y1 = imin(y1, cy1_);
	if (x1 <= x0 || y1 <= y0) return;

	// merge into an existing rectangle when that wastes little area
	int best = -1, bestGrow = 0x7FFFFFFF;
	for (int i = 0; i < nDirty_; i++) {
		const Rect& d = dirty_[i];
		int ux0 = imin(x0, d.x), uy0 = imin(y0, d.y);
		int ux1 = imax(x1, d.x + d.w), uy1 = imax(y1, d.y + d.h);
		int grow = area(ux0, uy0, ux1, uy1) - area(d.x, d.y, d.x + d.w, d.y + d.h) - area(x0, y0, x1, y1);
		bool overlap = x0 < d.x + d.w && x1 > d.x && y0 < d.y + d.h && y1 > d.y;
		if (overlap && grow < 0) grow = 0;
		if (grow < bestGrow) { bestGrow = grow; best = i; }
	}
	int newArea = area(x0, y0, x1, y1);
	bool full = nDirty_ == MaxDirty;
	if (best >= 0 && (bestGrow <= newArea / 2 + 64 || full)) {
		Rect& d = dirty_[best];
		int ux0 = imin(x0, d.x), uy0 = imin(y0, d.y);
		int ux1 = imax(x1, d.x + d.w), uy1 = imax(y1, d.y + d.h);
		d = Rect{ux0, uy0, ux1 - ux0, uy1 - uy0};
		return;
	}
	dirty_[nDirty_++] = Rect{x0, y0, x1 - x0, y1 - y0};
}

int Gfx::dirtyPixels() const
{
	int n = 0;
	for (int i = 0; i < nDirty_; i++) n += dirty_[i].w * dirty_[i].h;
	return n;
}

bool Gfx::takeChunk(Rect& r, uint16_t* out, int maxPixels)
{
	if (!nDirty_) return false;
	Rect& d = dirty_[0];
	int rows = imax(1, maxPixels / d.w);
	if (rows > d.h) rows = d.h;
	r = Rect{d.x, d.y, d.w, rows};
	for (int y = 0; y < rows; y++) {
		const uint16_t* src = fb_ + (d.y + y) * W + d.x;
		for (int x = 0; x < d.w; x++) {
			uint16_t v = src[x];
			*out++ = (uint16_t)((v << 8) | (v >> 8));   // panel wants big-endian RGB565
		}
	}
	d.y += rows;
	d.h -= rows;
	if (d.h <= 0) {
		for (int i = 1; i < nDirty_; i++) dirty_[i - 1] = dirty_[i];
		nDirty_--;
	}
	return true;
}

// ------------------------------------------------------------------ pixels
void Gfx::blend(int x, int y, Color c, int a)
{
	if (x < cx0_ || x >= cx1_ || y < cy0_ || y >= cy1_ || a <= 0) return;
	if (a >= 255) { put(x, y, c); return; }
	Color o = unpack(fb_[y * W + x]);
	put(x, y, mix(o, c, a + (a >> 7)));
}

void Gfx::clear(Color c) { fillRect(0, 0, W, H, c); }

void Gfx::fillRect(int x, int y, int w, int h, Color c)
{
	int x0 = imax(x, cx0_), y0 = imax(y, cy0_), x1 = imin(x + w, cx1_), y1 = imin(y + h, cy1_);
	if (x1 <= x0 || y1 <= y0) return;
	uint16_t p = pack(c);
	for (int yy = y0; yy < y1; yy++) {
		uint16_t* row = fb_ + yy * W;
		for (int xx = x0; xx < x1; xx++) row[xx] = p;
	}
	touch(x0, y0, x1, y1);
}

void Gfx::rrCoverage(float x, float y, float w, float h, float r, Color fill, Color line, float lw)
{
	if (r > w / 2) r = w / 2;
	if (r > h / 2) r = h / 2;
	float cx = x + w / 2, cy = y + h / 2, hx = w / 2 - r, hy = h / 2 - r;
	int x0 = (int)floorf(x) - 1, y0 = (int)floorf(y) - 1, x1 = (int)ceilf(x + w) + 1, y1 = (int)ceilf(y + h) + 1;
	int cx0 = imax(x0, cx0_), cy0 = imax(y0, cy0_), cx1 = imin(x1, cx1_), cy1 = imin(y1, cy1_);
	for (int py = cy0; py < cy1; py++) {
		for (int px = cx0; px < cx1; px++) {
			float qx = fabsf(px + 0.5f - cx) - hx, qy = fabsf(py + 0.5f - cy) - hy;
			float d;
			if (qx > 0 && qy > 0) d = sqrtf(qx * qx + qy * qy) - r;
			else d = (qx > qy ? qx : qy) - r;
			float cov = fclamp(0.5f - d, 0.f, 1.f);
			if (cov <= 0) continue;
			if (fill != NoColor) blend(px, py, fill, (int)(cov * 255.f + 0.5f));
			if (line != NoColor) {
				float ring = cov * fclamp(d + lw + 0.5f, 0.f, 1.f);
				blend(px, py, line, (int)(ring * 255.f + 0.5f));
			}
		}
	}
	touch(cx0, cy0, cx1, cy1);
}

void Gfx::roundRect(float x, float y, float w, float h, float r, Color fill, Color line, float lw)
{
	if (r <= 0.01f && line == NoColor && x == floorf(x) && y == floorf(y) && w == floorf(w) && h == floorf(h)) {
		if (fill != NoColor) fillRect((int)x, (int)y, (int)w, (int)h, fill);
		return;
	}
	rrCoverage(x, y, w, h, r, fill, line, lw);
}

void Gfx::disc(float cx, float cy, float r, Color fill, Color line, float lw)
{
	rrCoverage(cx - r, cy - r, 2 * r, 2 * r, r, fill, line, lw);
}

void Gfx::line(float x0, float y0, float x1, float y1, float width, Color c)
{
	float hw = width / 2;
	int bx0 = (int)floorf(fminf(x0, x1) - hw - 1), by0 = (int)floorf(fminf(y0, y1) - hw - 1);
	int bx1 = (int)ceilf(fmaxf(x0, x1) + hw + 1), by1 = (int)ceilf(fmaxf(y0, y1) + hw + 1);
	bx0 = imax(bx0, cx0_); by0 = imax(by0, cy0_); bx1 = imin(bx1, cx1_); by1 = imin(by1, cy1_);
	float dx = x1 - x0, dy = y1 - y0, len2 = dx * dx + dy * dy;
	for (int py = by0; py < by1; py++) {
		for (int px = bx0; px < bx1; px++) {
			float qx = px + 0.5f - x0, qy = py + 0.5f - y0;
			float t = len2 > 0 ? fclamp((qx * dx + qy * dy) / len2, 0.f, 1.f) : 0.f;
			float ex = qx - t * dx, ey = qy - t * dy;
			float dist = sqrtf(ex * ex + ey * ey);
			float cov = fclamp(hw + 0.5f - dist, 0.f, 1.f);
			if (cov > 0) blend(px, py, c, (int)(cov * 255.f + 0.5f));
		}
	}
	touch(bx0, by0, bx1, by1);
}

void Gfx::polyline(const float* xy, int n, float width, Color c)
{
	for (int i = 0; i + 1 < n; i++) line(xy[2 * i], xy[2 * i + 1], xy[2 * i + 2], xy[2 * i + 3], width, c);
}

// -------------------------------------------------------------------- text
static const Glyph* findGlyph(const Font& f, unsigned char ch)
{
	if (ch == 0xB7) return &f.glyphs[f.last - f.first + 1];     // middle dot lives after the last glyph
	if (ch >= 'a' && ch <= 'z' && ch > f.last) ch -= 32;       // Huge has no lower case
	if (ch < f.first || ch > f.last) ch = '?';
	if (ch < f.first || ch > f.last) return &f.glyphs[0];
	return &f.glyphs[ch - f.first];
}

float Gfx::textWidth(const char* s, const Font& f, float track) const
{
	int adv = 0, n = 0;
	for (; *s; s++, n++) adv += findGlyph(f, (unsigned char)*s)->adv16;
	return adv / 16.f + (n > 1 ? track * (n - 1) : 0.f);
}

float Gfx::text(int x, int y, const char* s, const Font& f, Color c, float track)
{
	int pen = x * 16, tr = (int)(track * 16.f + (track < 0 ? -0.5f : 0.5f));
	int minX = W, maxX = 0;
	for (; *s; s++) {
		const Glyph* g = findGlyph(f, (unsigned char)*s);
		int gx = ((pen + 8) >> 4) + g->x, gy = y + g->y;
		if (g->w) {
			const uint8_t* bits = f.bits + g->off;
			for (int row = 0; row < g->h; row++) {
				int py = gy + row;
				if (py < cy0_ || py >= cy1_) continue;
				for (int col = 0; col < g->w; col++) {
					int idx = row * g->w + col;
					int a = (bits[idx >> 1] >> ((idx & 1) ? 0 : 4)) & 15;
					if (a) blend(gx + col, py, c, a * 17);
				}
			}
			if (gx < minX) minX = gx;
			if (gx + g->w > maxX) maxX = gx + g->w;
		}
		pen += g->adv16 + tr;
	}
	if (maxX > minX) touch(minX, y - 1, maxX, y + f.lineHeight + 1);
	return (pen - x * 16) / 16.f;
}

void Gfx::textAligned(int x, int y, int w, const char* s, const Font& f, Color c, Align a, float track)
{
	float tw = textWidth(s, f, track);
	if (a == AL_CENTER) x += (int)((w - tw) / 2 + 0.5f);
	else if (a == AL_RIGHT) x += (int)(w - tw + 0.5f);
	text(x, y, s, f, c, track);
}

} // namespace zic
