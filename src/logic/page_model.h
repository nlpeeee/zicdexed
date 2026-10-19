//
// page_model.h - the lane tables for FM, OPS, ENV, MOD (LFO / pitch EG / FX), BROWSE and KEYS.
//
// A page is four lanes, one per encoder. A lane knows how to turn the current
// state into text and a bar fraction (read) and how to apply an encoder turn
// (adjust). The framework draws the lane from LaneView and calls adjust() with
// the already-accelerated delta; Shift = fine step is carried in UiCtx::shift.
//
#pragma once
#include "browse_model.h"
#include "edit_session.h"
#include "keys_model.h"

namespace zd {

// Parameters outside the voice (MiniDexed globals), implemented by the framework.
enum BusParam {
	BP_REVERB_SIZE, BP_REVERB_DAMP, BP_REVERB_LEVEL, BP_COMPRESSOR, // MOD > FX
	BP_TG_VOLUME,                                                   // BROWSE volume lane (target TG)
	kBusParamCount
};

class ParamBus {
public:
	virtual ~ParamBus() {}
	virtual int get(BusParam p, int tg) = 0;
	virtual void set(BusParam p, int tg, int value) = 0;
	virtual int max(BusParam p) = 0;
};

struct UiCtx {
	EditSession* session = nullptr;
	ParamBus* bus = nullptr;
	KeysModel* keys = nullptr;
	BrowseModel* browse = nullptr;
	int tg = 0;      // current TG 0..7
	int op = 0;      // selected operator 0..5, shared by OPS and ENV
	bool shift = false;
};

struct LaneView {
	char label[12];
	char text[12];
	float frac;      // 0..1 bar fill; for bipolar lanes 0.5 = centre
	bool bipolar;
};

struct Lane {
	const char* label;
	int arg;
	void (*read)(const UiCtx&, int arg, LaneView&);
	// Returns true when something audible or visible besides the lane itself changed
	// (voice edit, selected op, preview...). The lane re-reads either way.
	bool (*adjust)(UiCtx&, int arg, int delta);
};

enum PageId {
	PG_FM, PG_FM_MACROS2,            // key w
	PG_OPS, PG_ENV,                  // key e
	PG_LFO, PG_PEG, PG_FX,           // key r
	PG_BROWSE, PG_KEYS,
	kPageCount
};

struct Page {
	const char* name;
	Lane lanes[4];
	const char* keyLabels[4];        // labels for q w e r on this page
};

const Page& GetPage(PageId id);

// Key-page groups: pressing the same page key again steps through the sub-pages.
PageId FirstPageOfKey(char key);     // 'w' -> PG_FM, 'e' -> PG_OPS, 'r' -> PG_LFO, else kPageCount
PageId NextSubPage(PageId id);       // PG_OPS -> PG_ENV -> PG_OPS ...
int SubPageIndex(PageId id);
int SubPageCount(PageId id);

inline void ReadLane(const Page& p, int lane, const UiCtx& c, LaneView& out)
{
	out.label[0] = out.text[0] = 0;
	out.frac = 0;
	out.bipolar = false;
	p.lanes[lane].read(c, p.lanes[lane].arg, out);
	if (!out.label[0]) {
		const char* l = p.lanes[lane].label;
		int i = 0;
		for (; l[i] && i < 11; i++) out.label[i] = l[i];
		out.label[i] = 0;
	}
}

inline bool AdjustLane(const Page& p, int lane, UiCtx& c, int delta)
{
	return p.lanes[lane].adjust(c, p.lanes[lane].arg, delta);
}

// Text helpers (no printf, so this builds the same on bare metal)
void FormatInt(char* out, int v, bool showPlus = false);
void FormatRatio(char* out, int hundredths);

} // namespace zd
