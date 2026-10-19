//
// VOICE page (key 1). Three sub-pages of four lanes each:
//   tone: Cutoff, Reso, Detune, Volume
//   play: Mono, Porta, PB, Transpose
//   mix:  Pan, Reverb, Channel, Program
// The picture is the voice name in large type, bank and program, play flags,
// the TG activity dots and a live scope of the current TG.
//
#include "zicpages.h"
#include <string.h>

namespace zic {

namespace {

enum LaneKind { LK_PLAIN, LK_BIPOLAR, LK_PAN, LK_CHANNEL, LK_PROGRAM, LK_MONO, LK_PORTA_MODE };

struct LaneDef {
	const char* label;
	TGParam param;
	LaneKind kind;
};

const LaneDef kLanes[3][4] = {
	{ { "Cutoff", P_CUTOFF, LK_PLAIN }, { "Reso", P_RESO, LK_PLAIN },
	  { "Detune", P_DETUNE, LK_BIPOLAR }, { "Volume", P_VOLUME, LK_PLAIN } },
	{ { "Mode", P_MONO, LK_MONO }, { "Porta", P_PORTA_TIME, LK_PLAIN },
	  { "PB", P_PB_RANGE, LK_PLAIN }, { "Transpose", P_TRANSPOSE, LK_BIPOLAR } },
	{ { "Pan", P_PAN, LK_PAN }, { "Reverb", P_REVERB, LK_PLAIN },
	  { "Channel", P_CHANNEL, LK_CHANNEL }, { "Program", P_PROGRAM, LK_PROGRAM } },
};

const int kScopeX = 150, kScopeY = 94, kScopeW = 164, kScopeH = 44;
const int kDotsX = 232, kDotsY = 60;

class VoicePage : public Page {
public:
	const char* name() const override { return "VOICE"; }
	int subCount() const override { return 3; }

	void lane(Ctx& c, int sub, int i, LaneView& out) override
	{
		const LaneDef& d = kLanes[sub][i];
		copyStr(out.label, d.label, sizeof out.label);
		int v = c.syn.get(d.param, c.tg);
		const ParamRange& r = paramRange(d.param);
		out.frac = frac(v, r.min, r.max);
		out.bipolar = false;
		switch (d.kind) {
		case LK_PLAIN: fmtInt(out.text, v); break;
		case LK_BIPOLAR: fmtInt(out.text, v, true); out.bipolar = true; break;
		case LK_PAN: fmtPan(out.text, v); out.bipolar = true; break;
		case LK_CHANNEL: fmtChannel(out.text, v); break;
		case LK_PROGRAM: fmtInt(out.text, v + 1); if (v < 9) { out.text[2] = 0; out.text[1] = out.text[0]; out.text[0] = '0'; } break;
		case LK_MONO: copyStr(out.text, v ? "MONO" : "POLY", sizeof out.text); out.frac = v ? 1.f : 0.f; break;
		case LK_PORTA_MODE: copyStr(out.text, v ? "FOLLOW" : "RETAIN", sizeof out.text); break;
		}
	}

	bool turn(Ctx& c, int sub, int i, int steps, bool /*fine*/) override
	{
		const LaneDef& d = kLanes[sub][i];
		int v = c.syn.get(d.param, c.tg);
		if (d.kind == LK_MONO || d.kind == LK_PORTA_MODE) v = steps > 0 ? 1 : 0;
		else if (d.kind == LK_PROGRAM) v += steps > 0 ? 1 : -1;      // one voice per click
		else v += steps;
		const ParamRange& r = paramRange(d.param);
		if (v < r.min) v = r.min;
		if (v > r.max) v = r.max;
		c.syn.set(d.param, c.tg, v);
		return d.kind == LK_PROGRAM || d.kind == LK_MONO || d.param == P_PORTA_TIME || d.param == P_PB_RANGE;
	}

	void enter(Ctx&, int /*sub*/) override { sig_[0] = 0; dots_ = 0xFFFFFFFFu; agc_ = 0; }

	void draw(Ctx& c, int /*sub*/) override
	{
		drawInfo(c);
		drawDots(c, true);
		drawScope(c, true);
	}

	void tick(Ctx& c, int /*sub*/) override
	{
		char sig[64];
		signature(c, sig);
		if (strcmp(sig, sig_)) {
			c.g.fillRect(0, kPicTop, kDotsX - 2, kScopeY - kPicTop + 30, BG);
			drawInfo(c);
		}
		drawDots(c, false);
		drawScope(c, false);
	}

private:
	char sig_[64] = {0};
	uint32_t dots_ = 0xFFFFFFFFu;
	float agc_ = 0;
	bool scopeLive_ = false;

	void signature(Ctx& c, char* sig)
	{
		char nm[kNameLen + 1];
		c.syn.voiceName(c.tg, nm);
		char t[kNameLen + 1];
		trimName(t, nm);
		char b[12];
		strcpy(sig, t);
		strcat(sig, "|");
		fmtInt(b, c.syn.get(P_BANK, c.tg)); strcat(sig, b); strcat(sig, "|");
		fmtInt(b, c.syn.get(P_PROGRAM, c.tg)); strcat(sig, b); strcat(sig, "|");
		fmtInt(b, c.syn.get(P_MONO, c.tg)); strcat(sig, b); strcat(sig, "|");
		fmtInt(b, c.syn.get(P_PORTA_TIME, c.tg)); strcat(sig, b); strcat(sig, "|");
		fmtInt(b, c.syn.get(P_PB_RANGE, c.tg)); strcat(sig, b);
	}

	void drawInfo(Ctx& c)
	{
		Gfx& g = c.g;
		signature(c, sig_);
		char nm[kNameLen + 1], t[kNameLen + 1];
		c.syn.voiceName(c.tg, nm);
		trimName(t, nm);
		if (!t[0]) copyStr(t, "-", sizeof t);
		const Font& f = g.textWidth(t, fHuge) <= kDotsX - 8 ? fHuge : fBig;
		g.text(4, f.lineHeight == fHuge.lineHeight ? 44 : 52, t, f, WHITE);

		char line[48], b[12];
		c.syn.bankName(c.syn.get(P_BANK, c.tg), line);
		if (strlen(line) > 14) line[14] = 0;
		strcat(line, "  \xB7  ");
		fmtInt(b, c.syn.get(P_PROGRAM, c.tg) + 1);
		if (b[1] == 0) { b[2] = 0; b[1] = b[0]; b[0] = '0'; }
		strcat(line, b);
		strcat(line, " / 32");
		g.text(6, 98, line, fSmall, GREY);

		strcpy(line, c.syn.get(P_MONO, c.tg) ? "MONO" : "POLY");
		int porta = c.syn.get(P_PORTA_TIME, c.tg);
		if (porta) { strcat(line, "   PORTA "); fmtInt(b, porta); strcat(line, b); }
		strcat(line, "   PB "); fmtInt(b, c.syn.get(P_PB_RANGE, c.tg)); strcat(line, b);
		g.text(6, 112, line, fSmall, dim(c.accent, 218));
	}

	void drawDots(Ctx& c, bool force)
	{
		uint32_t sig = (uint32_t)c.tg << 24;
		int n = c.syn.tgCount();
		for (int i = 0; i < kMaxTG; i++) {
			uint32_t st = i >= n ? 3 : (c.ui.muted(i) ? 2 : (c.ui.playing(i) ? 1 : 0));
			sig |= st << (i * 2);
		}
		if (!force && sig == dots_) return;
		dots_ = sig;
		Gfx& g = c.g;
		g.fillRect(kDotsX - 2, kDotsY - 2, 84, 11, BG);
		for (int i = 0; i < n; i++) {
			int x = kDotsX + i * 10;
			Color col = kTGColor[i];
			if (c.ui.muted(i)) g.roundRect((float)x, (float)kDotsY, 7, 7, 2, NoColorBg(), DIM);
			else if (i == c.tg) g.roundRect((float)x - 1, (float)kDotsY - 1, 9, 9, 2, col);
			else g.roundRect((float)x, (float)kDotsY, 7, 7, 2, dim(col, c.ui.playing(i) ? 230 : 77));
		}
	}

	static Color NoColorBg() { return Gfx::NoColor; }

	void drawScope(Ctx& c, bool force)
	{
		Gfx& g = c.g;
		static int16_t buf[420];
		int n = c.syn.scope(buf, 420);
		const int w = kScopeW, h = kScopeH;

		int start = 0, peak = 0;
		if (n >= w + 8) {
			int lim = n - w - 1 < 255 ? n - w - 1 : 255;
			for (int i = 0; i < lim; i++) if (buf[i] < 0 && buf[i + 1] >= 0) { start = i; break; }
			for (int i = 0; i < w; i++) { int a = buf[start + i]; if (a < 0) a = -a; if (a > peak) peak = a; }
		}
		agc_ = peak > agc_ * 0.97f ? (float)peak : agc_ * 0.97f;
		bool live = agc_ > 120.f && n >= w + 8;
		if (!live && !scopeLive_ && !force) return;      // idle line is already on screen
		scopeLive_ = live;

		g.setClip(kScopeX, kScopeY, w + 2, h);
		g.fillRect(kScopeX, kScopeY, w + 2, h, BG);
		float cy = kScopeY + h / 2.f;
		g.hline(kScopeX, (int)cy, w, LINE);
		if (live) {
			float gain = (h / 2.f * 0.92f) / (agc_ < 2500.f ? 2500.f : agc_);
			static float pts[2 * 165];
			for (int i = 0; i <= w; i++) {
				float v = buf[start + (i < w ? i : w - 1)] * gain;
				pts[2 * i] = (float)(kScopeX + i);
				pts[2 * i + 1] = cy - v;
			}
			g.polyline(pts, w + 1, 5.f, dim(c.accent, 64));
			g.polyline(pts, w + 1, 1.6f, c.accent);
		} else {
			g.hline(kScopeX, (int)cy, w, dim(c.accent, 110));
		}
		g.clearClip();
	}
};

} // namespace

Page* CreateVoicePage() { static VoicePage p; return &p; }

} // namespace zic
