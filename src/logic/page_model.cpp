#include "page_model.h"

namespace zd {

// ---- text helpers --------------------------------------------------------

void FormatInt(char* out, int v, bool showPlus)
{
	char tmp[12];
	int n = 0;
	bool neg = v < 0;
	unsigned u = neg ? (unsigned)(-v) : (unsigned)v;
	do { tmp[n++] = (char)('0' + u % 10); u /= 10; } while (u);
	int i = 0;
	if (neg) out[i++] = '-';
	else if (showPlus && v > 0) out[i++] = '+';
	while (n) out[i++] = tmp[--n];
	out[i] = 0;
}

void FormatRatio(char* out, int h)
{
	char a[12];
	FormatInt(a, h / 100);
	int i = 0;
	for (; a[i]; i++) out[i] = a[i];
	out[i++] = '.';
	out[i++] = (char)('0' + (h / 10) % 10);
	out[i++] = (char)('0' + h % 10);
	out[i] = 0;
}

static void SetLabel(LaneView& v, const char* a, int n)
{
	int i = 0;
	for (; a[i] && i < 8; i++) v.label[i] = a[i];
	v.label[i++] = ' ';
	FormatInt(v.label + i, n);
}

static void SetText(LaneView& v, const char* s)
{
	int i = 0;
	for (; s[i] && i < 11; i++) v.text[i] = s[i];
	v.text[i] = 0;
}

static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

// ---- generic lane implementations ----------------------------------------

// Plain voice parameter (arg = absolute voice offset); `bake` decides whether macros are baked.
static void ReadVoiceParam(const UiCtx& c, int off, LaneView& v)
{
	int mx = ParamMax(off), val = c.session->param(off);
	FormatInt(v.text, val);
	v.frac = mx ? (float)val / mx : 0;
}
static bool AdjustVoiceParam(UiCtx& c, int off, int d)
{
	return c.session->setParam(off, c.session->param(off) + d);
}

// Parameter of the selected operator (arg = OpParam)
static void ReadOpParam(const UiCtx& c, int p, LaneView& v)
{
	ReadVoiceParam(c, OpOff(c.op, (OpParam)p), v);
}
static bool AdjustOpParam(UiCtx& c, int p, int d)
{
	return AdjustVoiceParam(c, OpOff(c.op, (OpParam)p), d);
}

// Macro lane (arg = Macro)
static void ReadMacro(const UiCtx& c, int m, LaneView& v)
{
	int val = c.session->macro(m);
	FormatInt(v.text, val, MacroBipolar(m));
	v.bipolar = MacroBipolar(m);
	int lo = kMacroMin[m], hi = kMacroMax[m];
	v.frac = (float)(val - lo) / (hi - lo);
}
static bool AdjustMacro(UiCtx& c, int m, int d)
{
	c.session->setMacro(m, c.session->macro(m) + d);
	return true;
}

// ---- FM ------------------------------------------------------------------

static void ReadAlgo(const UiCtx& c, int, LaneView& v)
{
	int a = c.session->param(G_ALG);
	FormatInt(v.text, a + 1);
	v.frac = a / 31.0f;
}
static bool AdjustAlgo(UiCtx& c, int, int d) { return c.session->setParam(G_ALG, c.session->param(G_ALG) + d); }

static void ReadSync(const UiCtx& c, int, LaneView& v)
{
	bool on = c.session->param(G_OKS) != 0;
	SetText(v, on ? "ON" : "OFF");
	v.frac = on ? 1.0f : 0.0f;
}
static bool AdjustSync(UiCtx& c, int, int d)
{
	if (d == 0) return false;
	// macros do not touch this parameter, so they stay live
	return c.session->setParam(G_OKS, d > 0 ? 1 : 0, false);
}

// ---- OPS -----------------------------------------------------------------

static void ReadOpSel(const UiCtx& c, int, LaneView& v)
{
	FormatInt(v.text, c.op + 1);
	v.frac = c.op / 5.0f;
}
static bool AdjustOpSel(UiCtx& c, int, int d)
{
	int n = Clamp(c.op + d, 0, kOps - 1);
	bool ch = n != c.op;
	c.op = n;
	return ch;
}

static void ReadRatio(const UiCtx& c, int, LaneView& v)
{
	const Voice& vo = c.session->edit();
	if (vo.op(c.op, OP_PM)) {
		FormatInt(v.text, FixedHz(vo, c.op));
		int i = 0;
		while (v.text[i]) i++;
		v.text[i++] = 'H'; v.text[i++] = 'z'; v.text[i] = 0;
		v.frac = vo.op(c.op, OP_PC) / 31.0f;
	} else {
		FormatRatio(v.text, RatioHundredths(vo, c.op));
		v.frac = vo.op(c.op, OP_PC) / 31.0f;
	}
}
// coarse by default, fine (PF) with Shift
static bool AdjustRatio(UiCtx& c, int, int d)
{
	OpParam p = c.shift ? OP_PF : OP_PC;
	return c.session->setParam(OpOff(c.op, p), c.session->param(OpOff(c.op, p)) + d);
}

static void ReadDetune(const UiCtx& c, int, LaneView& v)
{
	int pd = c.session->param(OpOff(c.op, OP_PD)) - 7;
	FormatInt(v.text, pd, true);
	v.bipolar = true;
	v.frac = (pd + 7) / 14.0f;
}
static bool AdjustDetune(UiCtx& c, int, int d) { return AdjustOpParam(c, OP_PD, d); }

// ---- Envelopes (arg = 0..3 for stage 1..4) -------------------------------

// Operator EG: rate, or level while Shift is held
static void ReadEgStage(const UiCtx& c, int st, LaneView& v)
{
	SetLabel(v, c.shift ? "LEVEL" : "RATE", st + 1);
	ReadVoiceParam(c, OpOff(c.op, (OpParam)((c.shift ? OP_L1 : OP_R1) + st)), v);
}
static bool AdjustEgStage(UiCtx& c, int st, int d)
{
	return AdjustVoiceParam(c, OpOff(c.op, (OpParam)((c.shift ? OP_L1 : OP_R1) + st)), d);
}

// Pitch EG: rate, or level while Shift is held (level 50 = no change)
static void ReadPegStage(const UiCtx& c, int st, LaneView& v)
{
	SetLabel(v, c.shift ? "LEVEL" : "RATE", st + 1);
	int off = (c.shift ? G_PL1 : G_PR1) + st;
	int val = c.session->param(off);
	if (c.shift) {
		FormatInt(v.text, val - 50, true);
		v.bipolar = true;
	} else {
		FormatInt(v.text, val);
	}
	v.frac = val / 99.0f;
}
static bool AdjustPegStage(UiCtx& c, int st, int d)
{
	int off = (c.shift ? G_PL1 : G_PR1) + st;
	return c.session->setParam(off, c.session->param(off) + d);
}

// ---- LFO -----------------------------------------------------------------

static void ReadLfoWave(const UiCtx& c, int, LaneView& v)
{
	static const char* const names[6] = { "TRI", "SAW DN", "SAW UP", "SQUARE", "SINE", "S&H" };
	int w = Clamp(c.session->param(G_LFW), 0, 5);
	SetText(v, names[w]);
	v.frac = w / 5.0f;
}
static bool AdjustLfoWave(UiCtx& c, int, int d)
{
	// one wave per detent no matter how fast the knob turns
	return c.session->setParam(G_LFW, c.session->param(G_LFW) + (d > 0 ? 1 : (d < 0 ? -1 : 0)));
}

// ---- FX (MiniDexed globals through the bus) ------------------------------

static void ReadBus(const UiCtx& c, int p, LaneView& v)
{
	if (!c.bus) { SetText(v, "-"); return; }
	int val = c.bus->get((BusParam)p, c.tg), mx = c.bus->max((BusParam)p);
	if (p == BP_COMPRESSOR) {
		SetText(v, val ? "ON" : "OFF");
		v.frac = val ? 1.0f : 0.0f;
		return;
	}
	FormatInt(v.text, val);
	v.frac = mx ? (float)val / mx : 0;
}
static bool AdjustBus(UiCtx& c, int p, int d)
{
	if (!c.bus) return false;
	int mx = c.bus->max((BusParam)p), cur = c.bus->get((BusParam)p, c.tg);
	int nv = (p == BP_COMPRESSOR) ? (d > 0 ? 1 : (d < 0 ? 0 : cur)) : Clamp(cur + d, 0, mx);
	if (nv == cur) return false;
	c.bus->set((BusParam)p, c.tg, nv);
	return true;
}

// ---- BROWSE ----------------------------------------------------------------

static void ReadBrVoice(const UiCtx& c, int, LaneView& v)
{
	int i = c.browse ? c.browse->index() : 0;
	FormatInt(v.text, i + 1);
	v.frac = i / (float)(kBankVoices - 1);
}
static bool AdjustBrVoice(UiCtx& c, int, int d)
{
	return c.browse && c.browse->moveVoice(d);
}
static void ReadBrBank(const UiCtx& c, int, LaneView& v)
{
	if (!c.browse) { SetText(v, "-"); return; }
	char n[kNameLen + 1];
	c.browse->bankLabel(n);
	SetText(v, n);
	v.frac = 0;
}
static bool AdjustBrBank(UiCtx& c, int, int d) { return c.browse && c.browse->moveBank(d); }
static void ReadBrTarget(const UiCtx& c, int, LaneView& v)
{
	int t = c.browse ? c.browse->target() : c.tg;
	v.text[0] = 'T'; v.text[1] = 'G'; FormatInt(v.text + 2, t + 1);
	v.frac = t / 7.0f;
}
static bool AdjustBrTarget(UiCtx& c, int, int d)
{
	if (!c.browse) return false;
	int before = c.browse->target();
	c.browse->setTarget(before + d);
	return c.browse->target() != before;
}

// ---- KEYS ------------------------------------------------------------------

static void ReadKeys(const UiCtx& c, int which, LaneView& v)
{
	const KeysModel* k = c.keys;
	if (!k) { SetText(v, "-"); return; }
	switch (which) {
	case 0: FormatInt(v.text, k->octave); v.frac = k->octave / 6.0f; break;
	case 1: SetText(v, KeysModel::ScaleName(k->scale)); v.frac = k->scale / (float)(KeysModel::ScaleCount() - 1); break;
	case 2: SetText(v, KeysModel::NoteName(k->root)); v.frac = k->root / 11.0f; break;
	case 3: FormatInt(v.text, k->velocity); v.frac = k->velocity / 127.0f; break;
	}
}
static bool AdjustKeys(UiCtx& c, int which, int d)
{
	KeysModel* k = c.keys;
	if (!k) return false;
	switch (which) {
	case 0: k->setOctave(k->octave + d); break;
	case 1: k->setScale(k->scale + (d > 0 ? 1 : (d < 0 ? -1 : 0))); break;
	case 2: k->setRoot(k->root + (d > 0 ? 1 : (d < 0 ? -1 : 0))); break;
	case 3: k->setVelocity(k->velocity + d); break;
	}
	return true;
}

// ---- tables -------------------------------------------------------------------

#define MACRO(label, m)   { label, m, ReadMacro, AdjustMacro }
#define EG(st)            { "", st, ReadEgStage, AdjustEgStage }
#define PEG(st)           { "", st, ReadPegStage, AdjustPegStage }

static const Page kPages[kPageCount] = {
	// PG_FM
	{ "FM", {
		{ "Algo", 0, ReadAlgo, AdjustAlgo },
		{ "Feedback", G_FB, ReadVoiceParam, AdjustVoiceParam },
		MACRO("Bright", M_BRIGHT),
		MACRO("Snarl", M_SNARL) }, { "", "", "", "" } },
	// PG_FM_MACROS2
	{ "FM 2", {
		MACRO("Bite", M_BITE),
		MACRO("Tail", M_TAIL),
		MACRO("Vel", M_VEL),
		{ "Sync", 0, ReadSync, AdjustSync } }, { "", "", "", "" } },
	// PG_OPS
	{ "OPS", {
		{ "Op", 0, ReadOpSel, AdjustOpSel },
		{ "Level", OP_OL, ReadOpParam, AdjustOpParam },
		{ "Ratio", 0, ReadRatio, AdjustRatio },
		{ "Detune", 0, ReadDetune, AdjustDetune } }, { "", "", "", "" } },
	// PG_ENV
	{ "ENV", { EG(0), EG(1), EG(2), EG(3) }, { "", "", "", "" } },
	// PG_LFO
	{ "LFO", {
		{ "Wave", 0, ReadLfoWave, AdjustLfoWave },
		{ "Speed", G_LFS, ReadVoiceParam, AdjustVoiceParam },
		{ "Pitch", G_LPMD, ReadVoiceParam, AdjustVoiceParam },
		{ "Amp", G_LAMD, ReadVoiceParam, AdjustVoiceParam } }, { "", "", "", "" } },
	// PG_PEG
	{ "PITCH EG", { PEG(0), PEG(1), PEG(2), PEG(3) }, { "", "", "", "" } },
	// PG_FX
	{ "FX", {
		{ "Size", BP_REVERB_SIZE, ReadBus, AdjustBus },
		{ "Damp", BP_REVERB_DAMP, ReadBus, AdjustBus },
		{ "Reverb", BP_REVERB_LEVEL, ReadBus, AdjustBus },
		{ "Comp", BP_COMPRESSOR, ReadBus, AdjustBus } }, { "", "", "", "" } },
	// PG_BROWSE
	{ "BROWSE", {
		{ "Voice", 0, ReadBrVoice, AdjustBrVoice },
		{ "Bank", 0, ReadBrBank, AdjustBrBank },
		{ "To", 0, ReadBrTarget, AdjustBrTarget },
		{ "Volume", BP_TG_VOLUME, ReadBus, AdjustBus } }, { "VOICES", "PERFS", "", "LOAD (c)" } },
	// PG_KEYS
	{ "KEYS", {
		{ "Octave", 0, ReadKeys, AdjustKeys },
		{ "Scale", 1, ReadKeys, AdjustKeys },
		{ "Root", 2, ReadKeys, AdjustKeys },
		{ "Velocity", 3, ReadKeys, AdjustKeys } }, { "", "", "", "EXIT (v)" } },
};

const Page& GetPage(PageId id) { return kPages[id < kPageCount ? id : PG_FM]; }

PageId FirstPageOfKey(char key)
{
	switch (key) {
	case 'w': return PG_FM;
	case 'e': return PG_OPS;
	case 'r': return PG_LFO;
	default: return kPageCount;
	}
}

static int GroupStart(PageId id)
{
	switch (id) {
	case PG_FM: case PG_FM_MACROS2: return PG_FM;
	case PG_OPS: case PG_ENV: return PG_OPS;
	case PG_LFO: case PG_PEG: case PG_FX: return PG_LFO;
	default: return id;
	}
}
int SubPageCount(PageId id)
{
	switch (GroupStart(id)) {
	case PG_FM: case PG_OPS: return 2;
	case PG_LFO: return 3;
	default: return 1;
	}
}
int SubPageIndex(PageId id) { return id - GroupStart(id); }
PageId NextSubPage(PageId id)
{
	int start = GroupStart(id), n = SubPageCount(id);
	return (PageId)(start + (SubPageIndex(id) + 1) % n);
}

} // namespace zd
