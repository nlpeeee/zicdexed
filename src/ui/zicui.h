//
// zicui.h - the ZicDexed screen framework: chrome, input routing, page switching.
//
// Layout (320x170), following the hardware: the four encoders sit above the
// screen so the top 44 px is one lane per encoder; the keys sit below so the
// bottom 18 px labels the top key row. The middle is the current page's picture.
//
// The glue feeds it key and encoder events and calls tick() from the main loop.
// Nothing here knows about Circle or MiniDexed; the synth is an ISynth.
//
#pragma once
#include "zicpage.h"

namespace zic {

// Palette. One hot colour per tone generator; the whole screen takes the
// current TG's colour.
constexpr Color BG = 0x060608, PANEL = 0x16171B, LINE = 0x282A30, GREY = 0x787A82,
                DIM = 0x3E4046, WHITE = 0xEEEEF0, INK = 0x060608;
extern const Color kTGColor[kMaxTG];

class Ui {
public:
	Ui(Gfx& g, ISynth& syn);

	// Pages are owned by the caller. A null slot is "not in this build".
	void addPage(Slot slot, Page* page) { pages_[slot] = page; }
	void start(uint32_t nowMs);          // draws the first screen

	// ---- input (from the glue)
	void key(int key, bool down, uint32_t nowMs);
	// steps: accelerated detents (negative = counter-clockwise); raw: unaccelerated
	void encoder(int enc, int steps, int raw, uint32_t nowMs);
	void tick(uint32_t nowMs);           // every main-loop pass; cheap when idle

	// ---- stock menu hand-over (Shift+v). While suspended the UI draws and
	// handles nothing; resume() repaints everything.
	bool menuRequested() { bool r = menuReq_; menuReq_ = false; return r; }
	bool suspended() const { return suspended_; }
	void resume(uint32_t nowMs);

	// ---- state for pages
	int tg() const { return tg_; }
	int tgCount() { return syn_.tgCount(); }
	Color accent() const { return kTGColor[tg_]; }
	void setTG(int tg);
	bool muted(int tg) const { return muted_[tg]; }
	bool soloed(int tg) const { return solo_[tg]; }
	bool anySolo() const;
	void toggleMute(int tg);
	void toggleSolo(int tg);
	// Smoothed output level of a TG for meters, 0..1 on a dB-ish scale.
	float meter(int tg) const { return meter_[tg]; }
	bool playing(int tg) const { return meter_[tg] > 0.02f; }
	void toast(const char* text, uint32_t ms = 1400);

	Ctx ctx() { return Ctx{g_, syn_, *this, tg_, accent(), shift_, now_}; }
	Slot slot() const { return slot_; }
	Slot prevSlot() const { return prev_; }   // the page we came from
	int sub() const { return sub_[slot_]; }
	void gotoSlot(Slot s);
	void gotoSub(Slot s, int sub);       // jump straight to a sub-page
	void redrawPicture();
	void redrawFooter() { if (!suspended_ && !mapShown_) drawFooter(); }
	void redrawAll();
	bool shiftHeld() const { return shift_; }

private:
	Page* page() const { return pages_[slot_]; }
	void drawLane(int i);
	void drawLanes();
	void drawFooter();
	void drawTGTag();
	void drawMap();
	void refreshLanes(bool force);
	void normalKey(int key);
	void shiftKey(int key);
	void applyMutes();
	void showToast();
	void clearToast();

	Gfx& g_;
	ISynth& syn_;
	Page* pages_[kSlotCount];
	Slot slot_, prev_;
	int sub_[kSlotCount];
	int tg_;
	uint32_t now_;

	bool shift_, mapShown_, mapHidden_;
	uint32_t shiftDownMs_;
	bool suspended_, menuReq_;

	uint32_t touchedMs_[4];
	LaneView shown_[4];
	bool shownHot_[4];
	uint32_t lastLaneMs_, lastTickMs_;

	char toast_[32];
	uint32_t toastUntil_;
	bool toastOn_;

	bool muted_[kMaxTG], solo_[kMaxTG];
	float level_[kMaxTG], meter_[kMaxTG];

	bool playing_, comparing_;
	int playTG_, playNote_, compareTG_;
	uint32_t vDownMs_;
	bool vDown_, vFired_;
};

const char* slotName(Slot s);

} // namespace zic
