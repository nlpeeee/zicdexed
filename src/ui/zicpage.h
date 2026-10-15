//
// zicpage.h - what a screen page is, and the context the framework hands it.
//
// A page is four lanes (one per encoder, along the top edge), a picture in the
// middle and four key labels along the bottom edge. The framework owns the
// chrome (lanes, key band, Shift map) and the input; a page supplies the lane
// values, what an encoder turn does, and the picture.
//
#pragma once
#include "zicgfx.h"
#include "zicsynth.h"

namespace zic {

constexpr int kLaneH = 44;           // lanes: y 0..43
constexpr int kPicTop = 44;          // picture: y 44..151
constexpr int kPicBottom = 152;
constexpr int kBandTop = 152;        // key labels: y 152..169

// Same shape as the logic layer's LaneView so adapters just copy.
struct LaneView {
	char label[12];
	char text[12];
	float frac;        // 0..1 bar fill; for bipolar lanes 0.5 = centre
	bool bipolar;
};

// Page slots: the key that jumps to a page.
enum Slot {
	SLOT_VOICE,   // q
	SLOT_FM,      // w
	SLOT_OPS,     // e
	SLOT_MOD,     // r
	SLOT_BROWSE,  // d
	SLOT_MIX,     // f
	SLOT_KEYS,    // Shift+x
	kSlotCount
};

// Physical keys, row-major as on the case: q w e r / a s d f / z x c v
enum Key {
	K_Q, K_W, K_E, K_R, K_A, K_S, K_D, K_F, K_Z, K_X, K_C, K_V, kKeyCount
};

class Ui;

struct Ctx {
	Gfx& g;
	ISynth& syn;
	Ui& ui;
	int tg;               // current TG
	Color accent;         // current TG colour
	bool shift;
	uint32_t nowMs;
};

class Page {
public:
	virtual ~Page() {}
	virtual const char* name() const = 0;
	virtual int subCount() const { return 1; }

	virtual void lane(Ctx& c, int sub, int i, LaneView& out) = 0;
	// `steps` is the accelerated turn (negative = counter-clockwise). With Shift
	// held the framework passes the raw, unaccelerated step and sets fine.
	// Return true when the picture needs a full redraw.
	virtual bool turn(Ctx& c, int sub, int i, int steps, bool fine) = 0;

	// Four labels for q w e r, or nullptr for the standard page-key labels.
	virtual const char* const* keyLabels(Ctx&, int /*sub*/) { return nullptr; }
	// Page-specific keys (not Shift combinations). Return true if consumed.
	virtual bool key(Ctx&, int /*sub*/, int /*key*/, bool /*down*/) { return false; }

	virtual void enter(Ctx&, int /*sub*/) {}
	virtual void draw(Ctx& c, int sub) = 0;      // full redraw of the picture area
	virtual void tick(Ctx&, int /*sub*/) {}      // ~25 fps: update the live parts only
};

} // namespace zic
