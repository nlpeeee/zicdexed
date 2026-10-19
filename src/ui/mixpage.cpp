//
// MIX page (key 8): the eight channel strips. Encoders: TG, Volume, Pan, Reverb.
// Pressing key 8 again shows the MASTER lane (TG, Master volume) over the same strips.
// Keys 1 / 2 mute and solo the current TG. Strips are drawn in their TG colour with
// a live level tick; the current TG's strip is outlined.
//
#include "zicpages.h"
#include <string.h>

namespace zic {

namespace {

const int kSW = 40;                  // strip width
const int kBarTop = 76, kBarH = 42;
const int kStripTop = 58, kStripH = 80;

class MixPage : public Page {
public:
	const char* name() const override { return "MIX"; }
	int subCount() const override { return 2; }

	void lane(Ctx& c, int sub, int i, LaneView& out) override
	{
		out.bipolar = false;
		if (sub == 1 && i > 0) {
			if (i == 1) {
				copyStr(out.label, "Master", sizeof out.label);
				int v = c.syn.masterVolume();
				fmtInt(out.text, v);
				out.frac = frac(v, 0, 127);
			} else {
				out.label[0] = 0;
				out.text[0] = 0;
				out.frac = 0.f;
			}
			return;
		}
		switch (i) {
		case 0:
			copyStr(out.label, "TG", sizeof out.label);
			fmtInt(out.text, c.tg + 1);
			out.frac = c.syn.tgCount() > 1 ? (float)c.tg / (float)(c.syn.tgCount() - 1) : 0.f;
			break;
		case 1: {
			copyStr(out.label, "Volume", sizeof out.label);
			int v = c.syn.get(P_VOLUME, c.tg);
			fmtInt(out.text, v);
			out.frac = frac(v, 0, 127);
			break;
		}
		case 2: {
			copyStr(out.label, "Pan", sizeof out.label);
			int v = c.syn.get(P_PAN, c.tg);
			fmtPan(out.text, v);
			out.frac = frac(v, 0, 127);
			out.bipolar = true;
			break;
		}
		default: {
			copyStr(out.label, "Reverb", sizeof out.label);
			int v = c.syn.get(P_REVERB, c.tg);
			fmtInt(out.text, v);
			out.frac = frac(v, 0, 99);
			break;
		}
		}
	}

	bool turn(Ctx& c, int sub, int i, int steps, bool) override
	{
		if (i == 0) { c.ui.setTG(c.tg + (steps > 0 ? 1 : -1)); return false; }
		if (sub == 1) {
			if (i == 1) {
				int v = c.syn.masterVolume() + volumeSteps(steps);
				c.syn.setMasterVolume(v < 0 ? 0 : (v > 127 ? 127 : v));
			}
			return false;
		}
		if (i == 1) steps = volumeSteps(steps);
		TGParam p = i == 1 ? P_VOLUME : (i == 2 ? P_PAN : P_REVERB);
		int v = c.syn.get(p, c.tg) + steps;
		const ParamRange& r = paramRange(p);
		if (v < r.min) v = r.min;
		if (v > r.max) v = r.max;
		c.syn.set(p, c.tg, v);
		return false;
	}

	const char* const* keyLabels(Ctx& c, int) override
	{
		static const char* labs[4];
		labs[0] = c.ui.muted(c.tg) ? "UNMUTE" : "MUTE";
		labs[1] = c.ui.soloed(c.tg) ? "UNSOLO" : "SOLO";
		labs[2] = "";
		labs[3] = "";
		return labs;
	}

	bool key(Ctx& c, int, int k, bool down) override
	{
		if (k > K_R) return false;
		if (down) {
			if (k == K_Q) c.ui.toggleMute(c.tg);
			else if (k == K_W) c.ui.toggleSolo(c.tg);
			c.ui.redrawFooter();
			redrawStrips(c);
		}
		return true;
	}

	void enter(Ctx&, int) override { for (int i = 0; i < kMaxTG; i++) { sig_[i] = 0xFFFFFFFFu; tick_[i] = -1; } }

	void draw(Ctx& c, int) override { redrawStrips(c); }

	void tick(Ctx& c, int) override
	{
		for (int i = 0; i < c.syn.tgCount() && i < kMaxTG; i++) {
			uint32_t s = signature(c, i);
			if (s != sig_[i]) { drawStrip(c, i); continue; }
			int ty = tickY(c, i);
			if (ty != tick_[i]) {
				int old = tick_[i] > 0 ? tick_[i] : ty, cur = ty > 0 ? ty : old;
				int lo = (old < cur ? old : cur) - 3, hi = (old > cur ? old : cur) + 4;
				drawBar(c, i, lo, hi);
			}
		}
	}

private:
	uint32_t sig_[kMaxTG];
	int tick_[kMaxTG];
	char names_[kMaxTG][kNameLen + 1];

	void redrawStrips(Ctx& c)
	{
		for (int i = 0; i < kMaxTG; i++) { sig_[i] = 0xFFFFFFFFu; tick_[i] = -1; }
		for (int i = 0; i < kMaxTG; i++) drawStrip(c, i);
	}

	uint32_t signature(Ctx& c, int i)
	{
		char nm[kNameLen + 1];
		c.syn.voiceName(i, nm);
		uint32_t h = 2166136261u;
		for (int k = 0; nm[k] && k < 5; k++) h = (h ^ (uint8_t)nm[k]) * 16777619u;
		h = (h ^ (uint32_t)c.syn.get(P_VOLUME, i)) * 16777619u;
		h = (h ^ (uint32_t)c.syn.get(P_PAN, i)) * 16777619u;
		h = (h ^ (uint32_t)((c.ui.muted(i) ? 1 : 0) | (c.ui.soloed(i) ? 2 : 0) | (c.ui.anySolo() ? 4 : 0) | (i == c.tg ? 8 : 0))) * 16777619u;
		return h;
	}

	bool silenced(Ctx& c, int i) { return c.ui.muted(i) || (c.ui.anySolo() && !c.ui.soloed(i)); }

	int tickY(Ctx& c, int i)
	{
		float m = silenced(c, i) ? 0.f : c.ui.meter(i);
		if (m < 0.03f) return 0;
		return kBarTop + kBarH - 1 - (int)((kBarH - 2) * m);
	}

	void drawBar(Ctx& c, int i, int y0, int y1)
	{
		Gfx& g = c.g;
		const int x = i * kSW;
		const Color col = kTGColor[i];
		const bool cur = i == c.tg, off = silenced(c, i);
		const int lo = y0 < kBarTop - 2 ? kBarTop - 2 : y0, hi = y1 > kBarTop + kBarH + 2 ? kBarTop + kBarH + 2 : y1;
		g.setClip(x + 12, lo, 16, hi - lo + 1);
		g.fillRect(x + 12, kBarTop - 2, 16, kBarH + 4, cur ? dim(col, 38) : BG);
		g.roundRect((float)x + 16, (float)kBarTop, 8, (float)kBarH, 2, PANEL);
		if (!off) {
			float bh = kBarH * c.syn.get(P_VOLUME, i) / 127.f;
			if (bh > 0.5f) g.roundRect((float)x + 16, kBarTop + kBarH - bh, 8, bh, 2, col);
		}
		int ty = tickY(c, i);
		if (ty) g.fillRect(x + 13, ty, 14, 2, WHITE);
		g.clearClip();
		tick_[i] = ty;
	}

	void drawStrip(Ctx& c, int i)
	{
		Gfx& g = c.g;
		const int x = i * kSW;
		const Color col = kTGColor[i];
		const bool cur = i == c.tg;
		const bool active = i < c.syn.tgCount();
		sig_[i] = active ? signature(c, i) : 0;
		g.fillRect(x, kStripTop, kSW, kStripH, BG);
		if (!active) {
			g.textAligned(x, 61, kSW, "-", fBody, DIM, AL_CENTER);
			return;
		}
		if (cur) g.roundRect((float)x + 2, (float)kStripTop, kSW - 4, kStripH, 4, dim(col, 38), col);
		const bool off = silenced(c, i);
		char num[4];
		fmtInt(num, i + 1);
		if (c.ui.soloed(i)) {
			g.roundRect((float)x + 10, 60, 20, 15, 3, col);
			g.textAligned(x, 61, kSW, num, fBody, INK, AL_CENTER);
		} else {
			g.textAligned(x, 61, kSW, num, fBody, off ? DIM : col, AL_CENTER);
		}
		drawBar(c, i, kBarTop - 2, kBarTop + kBarH + 2);
		if (off) g.textAligned(x, kBarTop + 20, kSW, "M", fBody, DIM, AL_CENTER);

		// pan: a dot on a short rail under the bar
		int pan = c.syn.get(P_PAN, i);
		g.hline(x + 6, 123, 28, LINE);
		g.roundRect((float)x + 6 + 26 * pan / 127.f, 121.5f, 3, 3, 1, off ? DIM : col);

		char nm[kNameLen + 1], t[kNameLen + 1], u[8];
		c.syn.voiceName(i, nm);
		trimName(t, nm);
		int k = 0;
		for (; t[k] && k < 5; k++) u[k] = (t[k] >= 'a' && t[k] <= 'z') ? t[k] - 32 : t[k];
		u[k] = 0;
		g.textAligned(x, 126, kSW, u, fLbl, off ? DIM : GREY, AL_CENTER);
	}
};

} // namespace

Page* CreateMixPage() { static MixPage p; return &p; }

} // namespace zic
