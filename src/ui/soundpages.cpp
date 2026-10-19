//
// FM (w), OPS/ENV (e), MOD (r), BROWSE (d) and KEYS (Shift+x) pages.
// The lane tables and edit logic live in src/logic (namespace zd); this file
// maps them onto the framework and draws each page's picture.
//
#include "soundhub.h"
#include "zicpages.h"
#include "alg_layout.h"
#include <math.h>
#include <string.h>

namespace zic {

namespace {

SoundHub& hub() { return SoundHub::get(); }

// One logic page per sub-page; the framework page just forwards.
void copyLane(const zd::LaneView& in, LaneView& out)
{
	copyStr(out.label, in.label, sizeof out.label);
	copyStr(out.text, in.text, sizeof out.text);
	out.frac = in.frac;
	out.bipolar = in.bipolar;
}

class LogicPage : public Page {
public:
	LogicPage(const char* nm, const zd::PageId* subs, int n) : name_(nm), subs_(subs), n_(n) {}
	const char* name() const override { return name_; }
	int subCount() const override { return n_; }

	void lane(Ctx& c, int sub, int i, LaneView& out) override
	{
		prepare(c, sub);
		zd::LaneView lv;
		zd::ReadLane(zd::GetPage(subs_[sub]), i, hub().ui(), lv);
		copyLane(lv, out);
	}

	bool turn(Ctx& c, int sub, int i, int steps, bool fine) override
	{
		prepare(c, sub);
		hub().ui().shift = fine;
		bool changed = zd::AdjustLane(zd::GetPage(subs_[sub]), i, hub().ui(), steps);
		hub().settle(c.tg);
		return changed;
	}

	void enter(Ctx& c, int sub) override { prepare(c, sub); }

protected:
	virtual void prepare(Ctx& c, int /*sub*/) { hub().bind(c); }
	zd::PageId logicPage(int sub) const { return subs_[sub]; }

	const char* name_;
	const zd::PageId* subs_;
	int n_;
};

// ---------------------------------------------------------------- helpers

Color opColor(Ctx& c, bool carrier) { return dim(c.accent, carrier ? 140 : 77); }

void opLevelBox(Ctx& c, float x, float y, float w, float h, int op, bool carrier, bool selected, bool muted, int level)
{
	Gfx& g = c.g;
	if (muted) {
		g.roundRect(x, y, w, h, 4, Gfx::NoColor, DIM);
		g.textAligned((int)x, (int)(y + h / 2 - 6), (int)w, "-", fBody, DIM, AL_CENTER);
		return;
	}
	g.roundRect(x, y, w, h, 4, PANEL, selected ? c.accent : (carrier ? dim(c.accent, 153) : LINE), selected ? 2.f : 1.f);
	float fh = (h - 4) * (float)level / 99.f;
	if (fh > 0.5f) g.roundRect(x + 2, y + h - 2 - fh, w - 4, fh, 2, opColor(c, carrier));
	char t[4];
	fmtInt(t, op + 1);
	g.textAligned((int)x, (int)(y + h / 2 - 6), (int)w, t, fBody, WHITE, AL_CENTER);
}

// Line from (x0,y0) to (x1,y1) as thin grey wire
void wire(Gfx& g, float x0, float y0, float x1, float y1) { g.line(x0, y0, x1, y1, 1.2f, GREY); }

// ---------------------------------------------------------------- FM

const zd::PageId kFmSubs[2] = { zd::PG_FM, zd::PG_FM_MACROS2 };

class FmPage : public LogicPage {
public:
	FmPage() : LogicPage("FM", kFmSubs, 2) {}

	void draw(Ctx& c, int) override
	{
		hub().bind(c);
		const zd::Voice& v = hub().session().edit();
		const int alg = v.algorithm();
		drawAlgorithm(c, hub().session(), alg, 6, 50, 172, 98);

		Gfx& g = c.g;
		char t[24], b[12];
		g.text(186, 50, "ALGORITHM", fLbl, GREY, 1.f);
		fmtInt(b, alg + 1);
		g.text(186 + 70, 48, b, fTitle, c.accent);
		int n = zd::CarrierCount(alg);
		copyStr(t, n == 1 ? "1 stack" : "", sizeof t);
		if (n > 1) { fmtInt(t, n); strcat(t, n == zd::kOps ? " carriers" : " stacks"); }
		g.text(186, 64, t, fBody, WHITE);

		g.text(186, 86, "FEEDBACK", fLbl, GREY, 1.f);
		int fb = v.feedback();
		for (int i = 0; i < 7; i++)
			g.roundRect((float)(186 + i * 18), 100, 14, 10, 2, i < fb ? c.accent : LINE);
		static const char* const note[8] = { "off", "a little warmth", "warm", "edge", "buzz", "grit", "harsh", "max: noise + grit" };
		g.text(186, 116, note[fb], fSmall, dim(c.accent, 218));
		if (hub().session().dirty()) g.text(186, 134, "edited", fSmall, GREY);
	}

	static void drawAlgorithm(Ctx& c, zd::EditSession& s, int alg, int x0, int y0, int w, int h)
	{
		Gfx& g = c.g;
		zd::AlgLayout lay;
		zd::LayoutAlgorithm(alg, lay);
		const float pitch = fminf(38.f, (float)w / lay.cols);
		const float bw = pitch - 4.f;
		const float rowPitch = fminf(40.f, (float)(h - 10) / (float)lay.rows);
		const float bh = fminf(26.f, rowPitch - 8.f);
		const float ox = x0 + (w - pitch * lay.cols) / 2.f;
		const zd::AlgInfo& a = zd::Algorithm(alg);
		const float outY = y0 + lay.rows * rowPitch + 2.f;

		float cx[zd::kOps], top[zd::kOps], bot[zd::kOps];
		for (int o = 0; o < zd::kOps; o++) {
			cx[o] = ox + lay.node[o].x * pitch + bw / 2.f + 2.f;
			top[o] = y0 + lay.node[o].row * rowPitch;
			bot[o] = top[o] + bh;
		}
		// wires first so the boxes cover their ends
		float lo = 1e9f, hi = -1e9f;
		for (int t = 0; t < zd::kOps; t++) {
			for (int o = 0; o < zd::kOps; o++)
				if ((a.mods[t] >> o) & 1) wire(g, cx[o], bot[o], cx[t], top[t]);
			if (zd::IsCarrier(alg, t)) {
				wire(g, cx[t], bot[t], cx[t], outY);
				if (cx[t] < lo) lo = cx[t];
				if (cx[t] > hi) hi = cx[t];
			}
		}
		g.line(lo - 4, outY, hi + 4, outY, 1.2f, GREY);
		// feedback loop: out of the feedback-out op, over the top, into the feedback-in op
		{
			const int fo = a.feedbackOut, fi = a.feedbackOp;
			float rx = cx[fo] + bw / 2.f, ry = top[fo] + bh / 2.f;
			float ty = fminf(top[fo], top[fi]) - 5.f;
			float pts[10] = { rx, ry, rx + 5.f, ry, rx + 5.f, ty, cx[fi], ty, cx[fi], top[fi] };
			g.polyline(pts, 5, 5.f, dim(c.accent, 77));
			g.polyline(pts, 5, 1.6f, c.accent);
		}
		const zd::Voice& v = s.edit();
		for (int o = 0; o < zd::kOps; o++)
			opLevelBox(c, cx[o] - bw / 2.f, top[o], bw, bh, o, zd::IsCarrier(alg, o), o == hub().ui().op, s.opMuted(o), v.op(o, zd::OP_OL));
	}
};

// ---------------------------------------------------------------- ENV drawing (operator and pitch)

void drawEnvelope(Ctx& c, const int rate[4], const int level[4], bool pitch, const char* title)
{
	Gfx& g = c.g;
	g.text(6, 47, title, fBody, c.accent);
	g.text(8 + (int)g.textWidth(title, fBody) + 10, 48, pitch ? "pitch envelope  \xB7  Shift: levels" : "envelope  \xB7  Shift: levels", fSmall, GREY);

	const int x0 = 10, y0 = 76, w = 300, h = 70;
	for (int gx = 0; gx <= 6; gx++) g.vline(x0 + gx * 50, y0, h, 0x12131A);
	if (pitch) g.hline(x0, y0 + h / 2, w, LINE);

	auto ly = [&](int l) { return y0 + h - (float)l / 99.f * h; };
	float seg[4];
	for (int i = 0; i < 4; i++) seg[i] = (float)(100 - rate[i]) / 100.f * 70.f + 8.f;
	float pts[14];
	int n = 0;
	float x = (float)x0;
	pts[n++] = x; pts[n++] = ly(level[3]);
	for (int i = 0; i < 3; i++) { x += seg[i]; pts[n++] = x; pts[n++] = ly(level[i]); }
	x += 70.f;                                   // key held
	pts[n++] = x; pts[n++] = ly(level[2]);
	x += seg[3];
	pts[n++] = x; pts[n++] = ly(level[3]);
	g.polyline(pts, n / 2, 5.f, dim(c.accent, 77));
	g.polyline(pts, n / 2, 1.8f, c.accent);
	for (int i = 0; i < 3; i++) {
		float px = pts[2 + i * 2], py = pts[3 + i * 2];
		g.disc(px, py, 3.5f, BG, WHITE, 1.6f);
		char t[4] = { 'L', (char)('1' + i), 0, 0 };
		g.text((int)px + 6, (int)py + 2, t, fLbl, GREY, 1.f);
	}
}

const zd::PageId kOpsSubs[2] = { zd::PG_OPS, zd::PG_ENV };

class OpsPage : public LogicPage {
public:
	OpsPage() : LogicPage("OPS", kOpsSubs, 2) {}

	void draw(Ctx& c, int sub) override
	{
		hub().bind(c);
		zd::EditSession& s = hub().session();
		const zd::Voice& v = s.edit();
		const int op = hub().ui().op;
		if (sub == 1) {
			int r[4], l[4];
			for (int i = 0; i < 4; i++) { r[i] = v.op(op, (zd::OpParam)(zd::OP_R1 + i)); l[i] = v.op(op, (zd::OpParam)(zd::OP_L1 + i)); }
			char t[8] = "OP";
			fmtInt(t + 2, op + 1);
			drawEnvelope(c, r, l, false, t);
			return;
		}
		Gfx& g = c.g;
		const int cw = 52, alg = v.algorithm();
		for (int i = 0; i < zd::kOps; i++) {
			const int x = 4 + i * cw;
			const bool sel = i == op, muted = s.opMuted(i);
			g.roundRect((float)x, 50, (float)(cw - 6), 98, 5, sel ? dim(c.accent, 41) : PANEL, sel ? c.accent : Gfx::NoColor, 1.f);
			char t[8] = "OP";
			fmtInt(t + 2, i + 1);
			g.text(x + 6, 53, t, fLbl, sel ? c.accent : GREY, 1.f);
			char r[12];
			if (v.op(i, zd::OP_PM)) { fmtInt(r, zd::FixedHz(v, i)); strcat(r, "Hz"); }
			else zd::FormatRatio(r, zd::RatioHundredths(v, i));
			g.text(x + 6, 64, r, fBody, muted ? DIM : WHITE);
			const int lvl = v.op(i, zd::OP_OL);
			const float bh = 54.f * (float)lvl / 99.f;
			g.roundRect((float)(x + 6), 138 - 54, 6, 54, 2, LINE);
			if (bh > 0.5f) g.roundRect((float)(x + 6), 138 - bh, 6, bh, 2, sel ? c.accent : dim(c.accent, 128));
			char n[4];
			fmtInt(n, lvl);
			g.text(x + 16, 128, n, fSmall, sel ? WHITE : GREY);
			if (zd::IsCarrier(alg, i)) g.text(x + 16, 84, "OUT", fLbl, dim(c.accent, 204), 1.f);
			if (muted) g.text(x + 16, 98, "MUTE", fLbl, GREY, 1.f);
		}
	}
};

// ---------------------------------------------------------------- MOD

const zd::PageId kModSubs[3] = { zd::PG_LFO, zd::PG_PEG, zd::PG_FX };

class ModPage : public LogicPage {
public:
	ModPage() : LogicPage("MOD", kModSubs, 3) {}

	void enter(Ctx& c, int sub) override { LogicPage::enter(c, sub); phaseLine_ = -1; }

	void draw(Ctx& c, int sub) override
	{
		hub().bind(c);
		const zd::Voice& v = hub().session().edit();
		Gfx& g = c.g;
		if (sub == 0) {
			char nm[16];
			lfoName(v, nm);
			g.text(8, 47, "LFO", fBody, c.accent);
			g.text(40, 48, nm, fSmall, GREY);
			phaseLine_ = -1;
			drawWave(c, v, kWX, kWX + kWW + 1);
			drawPhase(c, v, true);
		} else if (sub == 1) {
			int r[4], l[4];
			for (int i = 0; i < 4; i++) { r[i] = v.d[zd::G_PR1 + i]; l[i] = v.d[zd::G_PL1 + i]; }
			drawEnvelope(c, r, l, true, "PITCH EG");
		} else {
			drawFx(c);
		}
	}

	void tick(Ctx& c, int sub) override
	{
		if (sub != 0) return;
		hub().bind(c);
		drawPhase(c, hub().session().edit(), false);
	}

private:
	static const int kWX = 8, kWY = 66, kWW = 304, kWH = 80;
	int phaseLine_ = -1;

	static void lfoName(const zd::Voice& v, char* out)
	{
		static const char* const w[6] = { "triangle", "saw down", "saw up", "square", "sine", "sample & hold" };
		copyStr(out, w[v.d[zd::G_LFW] > 5 ? 5 : v.d[zd::G_LFW]], 20);
	}

	// value of waveform `wave` at phase p in [0,1): -1..1
	static float wave(int wf, float p, int cycle)
	{
		switch (wf) {
		case 0: return p < 0.5f ? 4.f * p - 1.f : 3.f - 4.f * p;
		case 1: return 1.f - 2.f * p;
		case 2: return 2.f * p - 1.f;
		case 3: return p < 0.5f ? 1.f : -1.f;
		case 4: return sinf(p * 6.2831853f);
		default: {                     // sample & hold: fixed pseudo-random steps per quarter cycle
			unsigned k = (unsigned)(cycle * 4 + (int)(p * 4.f)) * 2654435761u;
			return ((k >> 12) & 255) / 127.5f - 1.f;
		}
		}
	}

	// Redraw the waveform inside the column range [x0, x1)
	void drawWave(Ctx& c, const zd::Voice& v, int x0, int x1)
	{
		Gfx& g = c.g;
		g.setClip(x0, kWY - 2, x1 - x0, kWH + 4);
		g.fillRect(x0, kWY - 2, x1 - x0, kWH + 4, BG);
		const float cy = kWY + kWH / 2.f;
		g.hline(kWX, (int)cy, kWW, LINE);
		const int wf = v.d[zd::G_LFW] > 5 ? 5 : v.d[zd::G_LFW];
		static float pts[2 * (kWW + 1)];
		for (int i = 0; i <= kWW; i++) {
			float t = (float)i / (float)kWW * 4.f;          // four cycles across
			int cyc = (int)t;
			float val = wave(wf, t - (float)cyc, cyc);
			pts[2 * i] = (float)(kWX + i);
			pts[2 * i + 1] = cy - val * kWH * 0.42f;
		}
		g.polyline(pts, kWW + 1, 5.f, dim(c.accent, 77));
		g.polyline(pts, kWW + 1, 1.8f, c.accent);
		g.clearClip();
	}

	void drawPhase(Ctx& c, const zd::Voice& v, bool force)
	{
		// The sweep speed follows the Speed lane, compressed to something readable.
		const float s = (float)v.d[zd::G_LFS] / 99.f;
		const float hz = 0.15f + 3.85f * s * sqrtf(s);
		float ph = fmodf((float)c.nowMs * 0.001f * hz, 1.f);
		int px = kWX + (int)(ph * kWW);
		if (!force && px == phaseLine_) return;
		Gfx& g = c.g;
		if (phaseLine_ >= 0 && phaseLine_ != px) drawWave(c, v, phaseLine_ - 2, phaseLine_ + 3);
		phaseLine_ = px;
		g.vline(px, kWY - 2, kWH + 4, WHITE);
	}

	void drawFx(Ctx& c)
	{
		Gfx& g = c.g;
		static const struct { const char* label; zd::BusParam p; } rows[4] = {
			{ "ROOM SIZE", zd::BP_REVERB_SIZE }, { "DAMPING", zd::BP_REVERB_DAMP },
			{ "REVERB LEVEL", zd::BP_REVERB_LEVEL }, { "COMPRESSOR", zd::BP_COMPRESSOR } };
		g.text(8, 47, "EFFECTS", fBody, c.accent);
		g.text(70, 48, "shared by all tone generators", fSmall, GREY);
		for (int i = 0; i < 4; i++) {
			const int y = 66 + i * 21;
			const int val = hub().ui().bus->get(rows[i].p, c.tg), mx = hub().ui().bus->max(rows[i].p);
			g.text(8, y + 1, rows[i].label, fLbl, GREY, 1.f);
			g.roundRect(110, (float)(y + 2), 160, 8, 3, LINE);
			float f = mx ? (float)val / mx : 0.f;
			if (f > 0.f) g.roundRect(110, (float)(y + 2), fmaxf(6.f, 160.f * f), 8, 3, c.accent);
			char t[8];
			if (rows[i].p == zd::BP_COMPRESSOR) copyStr(t, val ? "ON" : "OFF", sizeof t);
			else fmtInt(t, val);
			g.textAligned(276, y, 40, t, fBody, WHITE, AL_RIGHT);
		}
	}
};

// ---------------------------------------------------------------- BROWSE

const zd::PageId kBrowseSubs[1] = { zd::PG_BROWSE };
const int kRows = 7, kRowH = 15, kListTop = 46;

class BrowsePage : public LogicPage {
public:
	BrowsePage() : LogicPage("BROWSE", kBrowseSubs, 1) {}

	const char* const* keyLabels(Ctx&, int) override
	{
		static const char* const l[4] = { "VOICES", "", "", "LOAD (11)" };
		return l;
	}

	void enter(Ctx& c, int) override
	{
		hub().bind(c);
		savedMask_ = 0;
		hub().browse().begin(hub().session().edit(), c.syn.get(P_BANK, c.tg), c.syn.get(P_PROGRAM, c.tg), c.tg);
		hub().ui().tg = c.tg;
	}

	void leave(Ctx& c, int) override { cancel(c); }

	// Lanes of this page always address the browse target TG.
	void prepare(Ctx& c, int sub) override
	{
		LogicPage::prepare(c, sub);
		if (hub().browse().active()) hub().ui().tg = hub().browse().target();
	}

	bool turn(Ctx& c, int sub, int i, int steps, bool fine) override
	{
		prepare(c, sub);
		zd::BrowseModel& b = hub().browse();
		if (!b.active()) return false;
		const int before = b.target();
		hub().ui().shift = fine;
		bool preview = zd::AdjustLane(zd::GetPage(zd::PG_BROWSE), i, hub().ui(), steps);
		if (i == 2 && b.target() != before) {          // moved to another TG: put the old one back, audition here
			restore(c, before);
			preview = true;
		}
		if (preview && i != 3) audition(c);
		return true;
	}

	bool key(Ctx& c, int, int k, bool down) override
	{
		if (!down) return false;
		if (k == K_C) {                                 // LOAD
			zd::BrowseModel& b = hub().browse();
			if (!b.active()) return true;
			const int t = b.target();
			if (savedMask_ & (1u << t)) {               // keep the previewed voice as the TG's program
				c.syn.set(P_BANK, t, b.bank());
				c.syn.set(P_PROGRAM, t, b.index());
				savedMask_ &= ~(1u << t);
				hub().forget(t);
			}
			c.ui.toast("VOICE LOADED");
			c.ui.gotoSlot(SLOT_VOICE);
			return true;
		}
		return false;                                   // v (exit) and the rest go to the framework; leave() undoes the preview
	}

	void draw(Ctx& c, int) override
	{
		hub().bind(c);
		zd::BrowseModel& b = hub().browse();
		Gfx& g = c.g;
		const int first = b.firstVisible(kRows);
		for (int r = 0; r < kRows; r++) {
			const int idx = first + r, y = kListTop + r * kRowH;
			const bool sel = idx == b.index();
			if (sel) g.roundRect(2, (float)y, 316, (float)kRowH, 3, c.accent);
			char num[4];
			fmtInt(num, idx + 1);
			if (num[1] == 0) { num[2] = 0; num[1] = num[0]; num[0] = '0'; }
			g.text(8, y + 2, num, fSmall, sel ? INK : DIM);
			char nm[zd::kNameLen + 1];
			if (!b.rowName(idx, nm)) nm[0] = 0;
			const int dist = idx > b.index() ? idx - b.index() : b.index() - idx;
			g.text(30, y + 1, nm, fBody, sel ? INK : (dist == 1 ? WHITE : GREY));
			if (sel && (savedMask_ & (1u << b.target()))) g.textAligned(200, y + 2, 110, "previewing", fSmall, INK, AL_RIGHT);
		}
	}

private:
	uint32_t savedMask_ = 0;
	uint8_t saved_[kMaxTG][kVoiceSize];

	void audition(Ctx& c)
	{
		zd::BrowseModel& b = hub().browse();
		const int t = b.target();
		if (!(savedMask_ & (1u << t))) { c.syn.readVoice(t, saved_[t]); savedMask_ |= 1u << t; }
		c.syn.loadVoice(t, b.preview().d);
		hub().forget(t);
		c.ui.redrawPicture();
	}

	void restore(Ctx& c, int tg)
	{
		if (savedMask_ & (1u << tg)) {
			c.syn.loadVoice(tg, saved_[tg]);
			savedMask_ &= ~(1u << tg);
			hub().forget(tg);
		}
	}

	void cancel(Ctx& c)
	{
		for (int t = 0; t < kMaxTG; t++) restore(c, t);
		if (hub().browse().active()) hub().browse().cancel();
	}
};

// ---------------------------------------------------------------- KEYS

const zd::PageId kKeysSubs[1] = { zd::PG_KEYS };

class KeysPage : public LogicPage {
public:
	KeysPage() : LogicPage("KEYS", kKeysSubs, 1) {}

	const char* const* keyLabels(Ctx&, int) override
	{
		static const char* const l[4] = { "", "", "", "EXIT (12)" };
		return l;
	}

	void enter(Ctx& c, int sub) override { LogicPage::enter(c, sub); shown_ = 0xFFFFu; }

	void leave(Ctx& c, int) override
	{
		hub().bind(c);
		hub().keys().allOff(&sink(c));
	}

	bool key(Ctx& c, int, int k, bool down) override
	{
		const int idx = k;                              // K_Q..K_F are keys 0..7, in pad order
		if (idx < 0 || idx >= zd::kKeyCount) return false;
		hub().bind(c);
		if (down) hub().keys().keyDown(idx, &sink(c)); else hub().keys().keyUp(idx, &sink(c));
		return true;
	}

	void draw(Ctx& c, int) override
	{
		hub().bind(c);
		drawPads(c, true);
	}

	void tick(Ctx& c, int) override { drawPads(c, false); }

private:
	uint32_t shown_ = 0xFFFFu;

	// the keys model plays through the current TG
	zd::VoiceSink& sink(Ctx& c)
	{
		struct S : zd::VoiceSink {
			ISynth* syn; int tg;
			void applyVoice(const zd::Voice&) override {}
			void noteOn(int n, int v) override { syn->noteOn(tg, n, v); }
			void noteOff(int n) override { syn->noteOff(tg, n); }
		};
		static S s;
		s.syn = &c.syn;
		s.tg = c.tg;
		return s;
	}

	void drawPads(Ctx& c, bool force)
	{
		const zd::KeysModel& k = hub().keys();
		uint32_t sig = 0;
		for (int i = 0; i < zd::kKeyCount; i++) if (k.held(i)) sig |= 1u << i;
		sig |= (uint32_t)(k.octave * 7 + k.scale * 3 + k.root * 5) << 8;     // changes when the layout changes
		if (!force && sig == shown_) return;
		shown_ = sig;
		Gfx& g = c.g;
		const int kw = 75, kh = 49;
		for (int i = 0; i < zd::kKeyCount; i++) {
			const int r = i / 4, col = i % 4;
			const int x = 4 + col * (kw + 4), y = 48 + r * (kh + 4);
			const bool on = k.held(i), root = k.isRoot(i);
			g.fillRect(x, y, kw, kh, BG);
			g.roundRect((float)x, (float)y, (float)kw, (float)kh, 6, on ? c.accent : PANEL, (root && !on) ? dim(c.accent, 153) : Gfx::NoColor, 1.f);
			g.textAligned(x, y + 8, kw, k.keyLabel(i), fHead, on ? INK : WHITE, AL_CENTER);
			char cap[2] = { (char)('1' + i), 0 };
			g.text(x + 6, y + 4, cap, fLbl, on ? INK : DIM);
		}
	}
};

} // namespace

Page* CreateFmPage() { static FmPage p; return &p; }
Page* CreateOpsPage() { static OpsPage p; return &p; }
Page* CreateModPage() { static ModPage p; return &p; }
Page* CreateBrowsePage() { static BrowsePage p; return &p; }
Page* CreateKeysPage() { static KeysPage p; return &p; }

} // namespace zic
