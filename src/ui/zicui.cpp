#include "zicui.h"
#include "zicpages.h"
#include <math.h>
#include <string.h>

namespace zic {

const Color kTGColor[kMaxTG] = {
	0xBEFF28, 0xFF6E1E, 0x28D2FF, 0xFF3C96, 0xFFDC28, 0x966EFF, 0x3CF0AA, 0xFF4646,
};

static const Color WARN = 0xFFB020;   // the dirty dot
static const char* const kSlotNames[kSlotCount] = { "VOICE", "FM", "OPS", "MOD", "BROWSE", "MIX", "KEYS", "SAVE" };
// The key label of the active page names the sub-page it is on, so MOD reads FX when you are on FX.
static const char* subLabel(Slot s, int sub, int count)
{
	if (s == SLOT_OPS && count == 2) return sub ? "ENVELOPE" : "OPERATORS";
	if (s == SLOT_MOD && count == 3) return sub == 0 ? "LFO" : (sub == 1 ? "PITCH EG" : "FX");
	return nullptr;
}
const char* slotName(Slot s) { return kSlotNames[s]; }

// Keys 5-12 on the pages that do not say otherwise.
static const char* const kLowerKeys[8] = { "TG-", "TG+", "BRWS", "MIX", "SHFT", "PLAY", "COMP", "EXIT" };

static const char* const kPageKeys[4] = { "VOICE", "FM", "OPS", "MOD" };

// Shift map (mockup 09): what each key does with and without Shift.
struct KeyDef { const char* shifted; const char* normal; };
static const KeyDef kKeyDefs[kKeyCount] = {
	{ "INIT", "voice" },     { "MUTATE", "fm" },       { "OP MUTE", "ops" },    { "FX", "mod" },
	{ "TG MUTE", "< tg" },   { "TG SOLO", "tg >" },    { "SAVE VCE", "browse" }, { "SAVE PRF", "mix" },
	{ "SHIFT", "shift" },    { "KEYS", "play" },       { "UNDO", "compare" }, { "MENU", "exit" },
};

static void upper(char* dst, const char* src, int n)
{
	int i = 0;
	for (; src[i] && i < n - 1; i++) dst[i] = (src[i] >= 'a' && src[i] <= 'z') ? src[i] - 32 : src[i];
	dst[i] = 0;
}

Ui::Ui(Gfx& g, ISynth& syn)
	: g_(g), syn_(syn), slot_(SLOT_VOICE), prev_(SLOT_VOICE), tg_(0), now_(0),
	  shift_(false), mapShown_(false), mapHidden_(false), shiftDownMs_(0),
	  suspended_(false), menuReq_(false),
	  lastLaneMs_(0), lastTickMs_(0), toastUntil_(0), toastOn_(false),
	  playing_(false), comparing_(false), playTG_(0), playNote_(60), compareTG_(0),
	  vDownMs_(0), vDown_(false), vFired_(false)
{
	for (int i = 0; i < kSlotCount; i++) { pages_[i] = nullptr; sub_[i] = 0; }
	for (int i = 0; i < 4; i++) { touchedMs_[i] = 0; shown_[i] = LaneView{}; shownHot_[i] = false; }
	for (int i = 0; i < kMaxTG; i++) { muted_[i] = solo_[i] = false; level_[i] = meter_[i] = 0; }
	toast_[0] = 0;
}

void Ui::start(uint32_t nowMs)
{
	now_ = nowMs;
	syn_.setScopeTG(tg_);
	redrawAll();
}

void Ui::resume(uint32_t nowMs)
{
	now_ = nowMs;
	suspended_ = false;
	shift_ = false;
	mapShown_ = false;
	redrawAll();
}

bool Ui::anySolo() const
{
	for (int i = 0; i < kMaxTG; i++) if (solo_[i]) return true;
	return false;
}

void Ui::applyMutes()
{
	bool solo = anySolo();
	for (int i = 0; i < syn_.tgCount() && i < kMaxTG; i++) syn_.setMute(i, muted_[i] || (solo && !solo_[i]));
}

void Ui::toggleMute(int tg) { muted_[tg] = !muted_[tg]; applyMutes(); }
void Ui::toggleSolo(int tg) { solo_[tg] = !solo_[tg]; applyMutes(); }

void Ui::setTG(int tg)
{
	int n = syn_.tgCount();
	if (tg < 0) tg = n - 1;
	if (tg >= n) tg = 0;
	if (tg == tg_) return;
	tg_ = tg;
	syn_.setScopeTG(tg_);
	Ctx c = ctx();
	if (page()) page()->enter(c, sub_[slot_]);
	redrawAll();
}

void Ui::gotoSlot(Slot s)
{
	if (!pages_[s]) {
		char buf[32];
		strcpy(buf, kSlotNames[s]);
		strcat(buf, " - NOT IN THIS BUILD");
		toast(buf);
		return;
	}
	if (s == slot_) {
		int n = pages_[s]->subCount();
		if (n > 1) sub_[s] = (sub_[s] + 1) % n;
		else return;
	} else {
		if (page()) { Ctx lc = ctx(); page()->leave(lc, sub_[slot_]); }
		prev_ = slot_;
		slot_ = s;
		sub_[s] = 0;
	}
	Ctx c = ctx();
	pages_[s]->enter(c, sub_[s]);
	redrawAll();
}

void Ui::gotoSub(Slot s, int sub)
{
	if (s != slot_ || !pages_[s]) gotoSlot(s);
	if (!pages_[s] || s != slot_ || sub < 0 || sub >= pages_[s]->subCount() || sub_[s] == sub) return;
	sub_[s] = sub;
	Ctx c = ctx();
	pages_[s]->enter(c, sub);
	redrawAll();
}

// ---------------------------------------------------------------- drawing
void Ui::drawLane(int i)
{
	Ctx c = ctx();
	if (i == 3) dirty_ = SoundDesignDirty(c);
	LaneView v{};
	if (page()) page()->lane(c, sub_[slot_], i, v);
	shown_[i] = v;
	bool hot = touchedMs_[i] != 0 && now_ - touchedMs_[i] < 600;
	shownHot_[i] = hot;

	const Color a = accent();
	int x = i * 80;
	g_.fillRect(x, 0, 80, kLaneH, BG);
	if (hot) g_.roundRect((float)x + 1, 0, 78, 44, 4, a);
	Color fg = hot ? INK : WHITE, lab = hot ? INK : GREY;
	char l[12];
	upper(l, v.label, sizeof l);
	g_.text(x + 7, 4, l, fLbl, lab, 1.f);
	const Font& vf = g_.textWidth(v.text, fVal) > 68 ? fBody : fVal;
	g_.text(x + 6, vf.lineHeight == fVal.lineHeight ? 13 : 18, v.text, vf, fg);

	// value track
	const int bx = x + 7, by = 38, bw = 66;
	float frac = v.frac < 0 ? 0 : (v.frac > 1 ? 1 : v.frac);
	g_.roundRect((float)bx, (float)by, (float)bw, 3, 1, hot ? dim(a, 150) : LINE);
	if (v.bipolar) {
		float mid = bx + bw / 2.f, pos = bx + bw * frac;
		float lo = mid < pos ? mid : pos, hi = mid < pos ? pos : mid;
		if (hi - lo < 2) hi = lo + 2;
		g_.roundRect(lo, (float)by, hi - lo, 3, 1, hot ? INK : a);
		g_.fillRect((int)mid, by - 2, 1, 7, hot ? INK : GREY);
	} else {
		float w = bw * frac;
		if (w < 2) w = 2;
		g_.roundRect((float)bx, (float)by, w, 3, 1, hot ? INK : a);
	}
	if (i == 3 && dirty_) g_.roundRect((float)(W - 10), 4, 6, 6, 3, hot ? INK : WARN);
	if (i && !hot && !(touchedMs_[i - 1] != 0 && now_ - touchedMs_[i - 1] < 600)) g_.vline(x, 6, 32, LINE);
}

void Ui::drawLanes() { for (int i = 0; i < 4; i++) drawLane(i); }

void Ui::refreshLanes(bool force)
{
	if (!page()) return;
	Ctx c = ctx();
	const bool dirtyNow = SoundDesignDirty(c);
	for (int i = 0; i < 4; i++) {
		LaneView v{};
		page()->lane(c, sub_[slot_], i, v);
		bool hot = touchedMs_[i] != 0 && now_ - touchedMs_[i] < 600;
		if (!hot && touchedMs_[i]) touchedMs_[i] = 0;
		bool same = !force && hot == shownHot_[i] && !strcmp(v.label, shown_[i].label) && !strcmp(v.text, shown_[i].text)
		            && (int)(v.frac * 66) == (int)(shown_[i].frac * 66) && v.bipolar == shown_[i].bipolar
		            && (i != 3 || dirtyNow == dirty_);
		if (!same) drawLane(i);
	}
}

void Ui::drawFooter()
{
	const int y = kBandTop + 1;
	g_.fillRect(0, kBandTop, W, H - kBandTop, BG);
	g_.hline(0, kBandTop, W, LINE);
	Ctx c = ctx();
	const char* const* labs = page() ? page()->keyLabels(c, sub_[slot_]) : nullptr;
	bool pageKeys = labs == nullptr;
	if (pageKeys) labs = kPageKeys;
	for (int i = 0; i < 4; i++) {
		int x = i * 80;
		bool on = pageKeys && (int)slot_ == i;
		if (on) g_.roundRect((float)x + 2, (float)y - 1, 76, 12, 3, accent());
		Color col = on ? INK : (!labs[i] || !labs[i][0] ? DIM : (pageKeys && !pages_[i] ? DIM : GREY));
		const char* lab = labs[i];
		if (on && page()) { const char* sl = subLabel(slot_, sub_[slot_], page()->subCount()); if (sl) lab = sl; }
		if (lab) g_.textAligned(x, y, 80, lab, fLbl, col, AL_CENTER, 1.f);
		if (on && page() && page()->subCount() > 1) {
			int n = page()->subCount();
			for (int k = 0; k < n; k++)
				g_.roundRect((float)(x + 70 - (n - 1 - k) * 5), (float)y + 4, 3, 3, 1, k == sub_[slot_] ? INK : mix(accent(), INK, 150));
		}
	}
	// Second row: keys 5-12 as one strip, each led by its key number.
	const char* const* low = page() ? page()->lowerLabels(c, sub_[slot_]) : nullptr;
	if (!low) low = kLowerKeys;
	for (int idx = 0; idx < 8; idx++) {
		if (!low[idx] || !low[idx][0]) continue;
		char t[16];
		const int key = idx + 5;
		int n = 0;
		if (key >= 10) t[n++] = '1';
		t[n++] = (char)('0' + (key >= 10 ? key - 10 : key));
		t[n++] = ' ';
		strcpy(t + n, low[idx]);
		const bool here = (idx == 2 && slot_ == SLOT_BROWSE) || (idx == 3 && slot_ == SLOT_MIX);
		g_.textAligned(idx * 40, kBandTop + 14, 40, t, fTiny, here ? accent() : GREY, AL_CENTER, 0.f);
	}
	if (toastOn_) showToast();
}

void Ui::drawTGTag()
{
	char t[8] = { 'T', 'G', ' ', (char)('1' + tg_), 0 };
	g_.roundRect((float)(W - 36), (float)kPicTop + 1, 32, 13, 3, BG, dim(accent(), 150), 1.f);
	g_.textAligned(W - 36, kPicTop + 2, 32, t, fLbl, accent(), AL_CENTER, 0.f);
}

void Ui::showToast()
{
	g_.fillRect(0, kBandTop + 1, W, H - kBandTop - 1, BG);
	char t[32];
	upper(t, toast_, sizeof t);
	float tw = g_.textWidth(t, fLbl, 1.f);
	int w = (int)tw + 20, x = (W - w) / 2;
	g_.roundRect((float)x, (float)kBandTop + 3, (float)w, 14, 3, accent());
	g_.textAligned(x, kBandTop + 5, w, t, fLbl, INK, AL_CENTER, 1.f);
}

void Ui::clearToast() { toastOn_ = false; toastUntil_ = 0; if (!mapShown_) drawFooter(); else g_.fillRect(0, kBandTop, W, H - kBandTop, BG); }

void Ui::toast(const char* text, uint32_t ms)
{
	strncpy(toast_, text, sizeof toast_ - 1);
	toast_[sizeof toast_ - 1] = 0;
	toastOn_ = true;
	toastUntil_ = ms ? now_ + ms : 0;
	if (!suspended_) showToast();
}

void Ui::drawMap()
{
	g_.clear(BG);
	g_.text(6, 2, "SHIFT", fHead, accent());
	g_.text(70, 7, "encoders: fine steps", fSmall, GREY);
	char net[32];
	syn_.netStatus(net);
	if (net[0]) g_.textAligned(0, 7, W - 6, net, fSmall, dim(accent(), 200), AL_RIGHT);
	const int kw = 76, kh = 36;
	for (int r = 0; r < 3; r++) {
		for (int c = 0; c < 4; c++) {
			int k = r * 4 + c, x = 4 + c * (kw + 2), y = 30 + r * 44;
			bool held = k == K_Z;
			g_.roundRect((float)x, (float)y, (float)kw, (float)kh, 5, held ? accent() : PANEL);
			char cap[3] = { 0, 0, 0 };                       // keys are numbered 1..12, top left to bottom right
			if (k + 1 >= 10) { cap[0] = '1'; cap[1] = (char)('0' + (k + 1 - 10)); } else cap[0] = (char)('0' + k + 1);
			g_.text(x + 6, y + 3, cap, fLbl, held ? INK : DIM);
			g_.textAligned(x, y + 11, kw, kKeyDefs[k].shifted, fBody, held ? INK : WHITE, AL_CENTER);
			g_.textAligned(x, y + 25, kw, kKeyDefs[k].normal, fTiny, held ? INK : GREY, AL_CENTER);
		}
	}
	if (toastOn_) showToast();
}

void Ui::redrawPicture()
{
	if (suspended_ || mapShown_) return;
	g_.fillRect(0, kPicTop, W, kPicBottom - kPicTop, BG);
	if (!page()) return;
	Ctx c = ctx();
	page()->draw(c, sub_[slot_]);
	drawTGTag();
}

void Ui::redrawAll()
{
	if (suspended_) return;
	if (mapShown_) { drawMap(); return; }
	g_.clear(BG);
	drawLanes();
	if (page()) { Ctx c = ctx(); page()->draw(c, sub_[slot_]); drawTGTag(); }
	else g_.textAligned(0, 90, W, "NO PAGE", fBody, DIM, AL_CENTER, 1.f);
	drawFooter();
}

// ------------------------------------------------------------------ input
void Ui::encoder(int enc, int steps, int raw, uint32_t nowMs)
{
	now_ = nowMs;
	if (suspended_ || !page() || enc < 0 || enc > 3) return;
	if (shift_ && mapShown_) { mapShown_ = false; redrawAll(); }
	if (shift_) mapHidden_ = true;
	Ctx c = ctx();
	bool fine = shift_;
	bool redraw = page()->turn(c, sub_[slot_], enc, fine ? raw : steps, fine);
	touchedMs_[enc] = now_ ? now_ : 1;
	if (redraw) redrawPicture();
	drawLane(enc);
}

void Ui::key(int k, bool down, uint32_t nowMs)
{
	now_ = nowMs;
	if (suspended_ || k < 0 || k >= kKeyCount) return;

	if (k == K_Z) {
		shift_ = down;
		if (down) { shiftDownMs_ = nowMs; mapHidden_ = false; }
		else if (mapShown_) { mapShown_ = false; redrawAll(); }
		return;
	}

	if (!down) {
		Ctx c = ctx();
		if (page()) page()->key(c, sub_[slot_], k, false);
		if (k == K_X && playing_) { syn_.noteOff(playTG_, playNote_); playing_ = false; }
		if (k == K_C && comparing_) {
			Ctx cc = ctx();
			if (!SoundDesignCompare(cc, compareTG_, false)) syn_.compareEnd(compareTG_);
			comparing_ = false;
			clearToast();
			redrawAll();
		}
		if (k == K_V && vDown_) {
			vDown_ = false;
			if (!vFired_ && pages_[SLOT_VOICE]) { if (slot_ != SLOT_VOICE || sub_[SLOT_VOICE]) { if (slot_ != SLOT_VOICE && page()) { Ctx lc = ctx(); page()->leave(lc, sub_[slot_]); } sub_[SLOT_VOICE] = 0; slot_ = SLOT_VOICE; Ctx c2 = ctx(); pages_[SLOT_VOICE]->enter(c2, 0); redrawAll(); } }
		}
		return;
	}

	if (shift_) shiftKey(k); else normalKey(k);
}

void Ui::normalKey(int k)
{
	Ctx c = ctx();
	if (page() && page()->key(c, sub_[slot_], k, true)) return;
	switch (k) {
	case K_Q: case K_W: case K_E: case K_R: gotoSlot((Slot)k); break;
	case K_D: gotoSlot(SLOT_BROWSE); break;
	case K_F: gotoSlot(SLOT_MIX); break;
	case K_A: setTG(tg_ - 1); break;
	case K_S: setTG(tg_ + 1); break;
	case K_X:
		if (!playing_) { playTG_ = tg_; playNote_ = 60; syn_.noteOn(tg_, playNote_, 100); playing_ = true; }
		break;
	case K_C:
		if (!comparing_) {
			comparing_ = true;
			compareTG_ = tg_;
			Ctx cc = ctx();
			if (!SoundDesignCompare(cc, tg_, true)) syn_.compareBegin(tg_);
			toast("COMPARE - STORED VOICE", 0);
			redrawAll();
		}
		break;
	case K_V: vDown_ = true; vFired_ = false; vDownMs_ = now_; break;
	default: break;
	}
}

void Ui::shiftKey(int k)
{
	switch (k) {
	case K_Q: case K_W: case K_E: {
		Ctx c = ctx();
		if (SoundDesignShiftKey(c, k)) break;
		toast(k == K_Q ? "INIT - NOT IN THIS BUILD" : (k == K_W ? "MUTATE - NOT IN THIS BUILD" : "OP MUTE - NOT IN THIS BUILD"));
		break;
	}
	case K_R: { Ctx c = ctx(); if (!SoundDesignShiftKey(c, k)) toast("FX - NOT IN THIS BUILD"); break; }
	case K_A: toggleMute(tg_); toast(muted_[tg_] ? "TG MUTED" : "TG UNMUTED"); redrawPicture(); break;
	case K_S: toggleSolo(tg_); toast(solo_[tg_] ? "TG SOLO ON" : "TG SOLO OFF"); redrawPicture(); break;
	case K_D: { Ctx c = ctx(); if (!SoundDesignShiftKey(c, k)) toast("SAVE VOICE - NOT IN THIS BUILD"); break; }
	case K_F: { Ctx c = ctx(); if (!SoundDesignShiftKey(c, k)) toast("SAVE PERFORMANCE - NOT IN THIS BUILD"); break; }
	case K_X: gotoSlot(SLOT_KEYS); break;
	case K_C: { Ctx c = ctx(); if (!SoundDesignShiftKey(c, k)) { syn_.revert(tg_); toast("REVERTED"); redrawAll(); } break; }
	case K_V: menuReq_ = true; suspended_ = true; break;
	default: break;
	}
}

void Ui::tick(uint32_t nowMs)
{
	now_ = nowMs;
	if (suspended_) return;

	if (shift_ && !mapShown_ && !mapHidden_ && now_ - shiftDownMs_ >= 250) { mapShown_ = true; drawMap(); }
	if (vDown_ && !vFired_ && now_ - vDownMs_ >= 700) {
		vFired_ = true;
		syn_.panic();
		toast("PANIC - ALL NOTES OFF");
	}
	if (toastOn_ && toastUntil_ && (int32_t)(now_ - toastUntil_) >= 0) clearToast();
	if (mapShown_) return;

	if (now_ - lastLaneMs_ >= 100) {
		lastLaneMs_ = now_;
		refreshLanes(false);
	}
	if (now_ - lastTickMs_ >= 40) {
		lastTickMs_ = now_;
		int n = syn_.tgCount();
		for (int i = 0; i < n && i < kMaxTG; i++) {
			float raw = syn_.level(i);
			level_[i] = raw > level_[i] * 0.82f ? raw : level_[i] * 0.82f;
			float m = level_[i] > 0.0005f ? (20.f * log10f(level_[i]) + 54.f) / 54.f : 0.f;
			meter_[i] = m < 0 ? 0 : (m > 1 ? 1 : m);
		}
		if (page()) { Ctx c = ctx(); page()->tick(c, sub_[slot_]); }
	}
}

} // namespace zic
