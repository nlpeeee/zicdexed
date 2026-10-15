// Host tests for src/logic. Build and run with: make -C tests
#include <stdio.h>
#include <string.h>
#include "../src/logic/page_model.h"
#include "../src/logic/shift_map.h"
#include "../src/logic/alg_layout.h"

using namespace zd;

static int g_fail = 0, g_checks = 0;
#define CHECK(c) do { g_checks++; if (!(c)) { g_fail++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

struct RecordSink : VoiceSink {
	int applied = 0, ons = 0, offs = 0, lastNote = -1;
	Voice last;
	void applyVoice(const Voice& v) override { applied++; last = v; }
	void noteOn(int n, int) override { ons++; lastNote = n; }
	void noteOff(int n) override { offs++; lastNote = n; }
};

struct FakeBus : ParamBus {
	int v[kBusParamCount] = {};
	int get(BusParam p, int) override { return v[p]; }
	void set(BusParam p, int, int x) override { v[p] = x; }
	int max(BusParam p) override { return p == BP_COMPRESSOR ? 1 : 99; }
};

struct FakeBank : BankSource {
	int bankCount() override { return 3; }
	void bankName(int b, char* o) override { strcpy(o, b == 0 ? "ROM1A" : (b == 1 ? "NASTY" : "ROM2")); }
	bool voiceName(int b, int i, char* o) override { sprintf(o, "V%d-%02d", b, i); return true; }
	bool readVoice(int b, int i, Voice& out) override { out = Voice::Init(); out.d[G_FB] = (uint8_t)(b * 2 + (i & 1)); sprintf((char*)&out.d[G_NAME], "V%d-%02d    ", b, i); return true; }
};

static void test_voice()
{
	static const int carriers[32] = { 2,2,2,2,3,3,2,2,2,2,2,2,2,2,2,1,1,1,3,3,4,4,4,5,5,3,3,3,4,4,5,6 };
	for (int a = 0; a < 32; a++) CHECK(CarrierCount(a) == carriers[a]);
	for (int a = 0; a < 32; a++) CHECK(Algorithm(a).feedbackOp < 6);
	for (int a = 0; a < 32; a++) CHECK(IsCarrier(a, 0)); // OP1 is always a carrier

	// topology spot checks (0-based ops: OP3 = 2)
	CHECK(Algorithm(6).mods[2] == ((1 << 3) | (1 << 4)) && Algorithm(6).mods[4] == (1 << 5) && Algorithm(6).mods[0] == (1 << 1));
	CHECK(Algorithm(15).mods[0] == ((1 << 1) | (1 << 2) | (1 << 4)) && Algorithm(15).mods[2] == (1 << 3));
	CHECK(Algorithm(17).mods[0] == ((1 << 1) | (1 << 2) | (1 << 3)) && Algorithm(17).feedbackOp == 2);
	CHECK(Algorithm(3).feedbackOp == 5 && Algorithm(3).feedbackOut == 3 && Algorithm(0).feedbackOut == 5);
	for (int a = 0; a < 32; a++) { // carriers modulate nobody, every other op feeds something
		for (int o = 0; o < 6; o++) {
			int users = 0;
			for (int t = 0; t < 6; t++) if (Algorithm(a).mods[t] & (1 << o)) users++;
			if (IsCarrier(a, o)) CHECK(users == 0);
			else CHECK(users >= 1);
		}
	}
	Voice v = Voice::Init();
	CHECK(OpOff(5, OP_R1) == 0);          // OP6 first in the dump
	CHECK(OpOff(0, OP_PD) == 5 * 21 + 20);
	CHECK(v.op(0, OP_OL) == 99 && v.op(1, OP_OL) == 0);
	CHECK(!v.set(G_ALG, 31 + 0) == false); // 0 -> 31 changes
	CHECK(v.d[G_ALG] == 31);
	CHECK(v.set(G_ALG, 999) == false && v.d[G_ALG] == 31); // clamped to the same value
	v.set(OpOff(2, OP_PC), 5);
	CHECK(RatioHundredths(v, 2) == 500);
	v.set(OpOff(2, OP_PF), 50);
	CHECK(RatioHundredths(v, 2) == 750);
	v.set(OpOff(2, OP_PC), 0);
	CHECK(RatioHundredths(v, 2) == 75);
	char n[11]; Voice::Init().name(n);
	CHECK(strcmp(n, "INIT VOICE") == 0);
	for (int off = 0; off < kVoiceSize; off++) { Voice t = Voice::Init(); t.set(off, 100000); CHECK(t.d[off] <= ParamMax(off) || off >= G_NAME); }
}

static void test_layout()
{
	for (int a = 0; a < 32; a++) {
		AlgLayout l; LayoutAlgorithm(a, l);
		CHECK(l.rows >= 1 && l.rows <= 4 && l.cols >= 1 && l.cols <= 6.01f);
		for (int o = 0; o < 6; o++) {
			CHECK(l.node[o].x >= 0 && l.node[o].x + 1 <= l.cols + 0.001f);
			CHECK(l.node[o].row >= 0 && l.node[o].row < l.rows);
			if (IsCarrier(a, o)) CHECK(l.node[o].row == l.rows - 1);
			for (int t = 0; t < 6; t++) // a modulator sits above what it modulates
				if (Algorithm(a).mods[t] & (1 << o)) CHECK(l.node[o].row < l.node[t].row);
			for (int p = o + 1; p < 6; p++) // no two boxes overlap
				if (l.node[o].row == l.node[p].row) CHECK(l.node[o].x + 0.999f <= l.node[p].x || l.node[p].x + 0.999f <= l.node[o].x);
		}
	}
	AlgLayout l; LayoutAlgorithm(0, l);   // alg 1: chain of four over one carrier plus 2 over 1
	CHECK(l.rows == 4);
	LayoutAlgorithm(31, l);               // alg 32: six carriers
	CHECK(l.rows == 1 && l.cols > 5.99f);
}

static void test_macros()
{
	Voice v = Voice::Init();
	v.set(G_ALG, 4); // algorithm 5: carriers 1,3,5
	for (int o = 0; o < 6; o++) { v.set(OpOff(o, OP_OL), 80); v.set(OpOff(o, OP_L2), 90); v.set(OpOff(o, OP_L3), 85); v.set(OpOff(o, OP_R4), 50); }
	MacroState m; m.rebase(v);
	CHECK(m.render() == v);               // all zero = identity
	m.set(M_BRIGHT, 50);
	Voice b = m.render();
	for (int o = 0; o < 6; o++) {
		if (IsCarrier(4, o)) CHECK(b.op(o, OP_OL) == 80);
		else CHECK(b.op(o, OP_OL) == 100 - 1 || b.op(o, OP_OL) == 99); // 80 + 20 clamps to 99
	}
	m.set(M_BRIGHT, 0);
	CHECK(m.render() == v);               // returning to 0 restores exactly
	m.set(M_BRIGHT, -50);
	CHECK(m.render().op(1, OP_OL) == 60 && m.render().op(0, OP_OL) == 80);

	// off operators stay off
	Voice w = v; w.set(OpOff(1, OP_OL), 0);
	MacroState m2; m2.rebase(w); m2.set(M_BRIGHT, 50); m2.set(M_SNARL, 99);
	CHECK(m2.render().op(1, OP_OL) == 0);

	// every macro at every extreme stays in range and never touches carriers' level
	for (int mm = 0; mm < kMacroCount; mm++)
		for (int a = 0; a < 32; a++) {
			Voice t = Voice::Init(); t.set(G_ALG, a);
			for (int o = 0; o < 6; o++) t.set(OpOff(o, OP_OL), 70);
			MacroState ms; ms.rebase(t);
			ms.set(mm, kMacroMin[mm]); Voice lo = ms.render();
			ms.set(mm, kMacroMax[mm]); Voice hi = ms.render();
			for (int off = 0; off < G_NAME; off++) { CHECK(lo.d[off] <= ParamMax(off)); CHECK(hi.d[off] <= ParamMax(off)); }
			for (int o = 0; o < 6; o++) if (IsCarrier(a, o) && mm != M_BRIGHT) CHECK(hi.op(o, OP_OL) == 70);
		}

	MacroState s; Voice t = Voice::Init(); t.set(G_ALG, 4); t.set(OpOff(1, OP_OL), 80);
	s.rebase(t); s.set(M_SNARL, 99);
	CHECK(s.render().feedback() == 7);
	s.set(M_TAIL, 50); // release faster/longer moves R4 only
	CHECK(s.render().op(0, OP_R4) < 99);
}

static void test_mutate()
{
	// deterministic for a seed
	Voice a = Voice::Init(), b = Voice::Init();
	a.set(G_ALG, 4); b.set(G_ALG, 4);
	for (int o = 0; o < 6; o++) { a.set(OpOff(o, OP_OL), 80); b.set(OpOff(o, OP_OL), 80); }
	Rng r1(7), r2(7);
	for (int i = 0; i < 20; i++) { Mutate(a, r1, 60); Mutate(b, r2, 60); }
	CHECK(a == b);
	CHECK(a != Voice::Init());

	// after many mutations: parameters valid, carriers audible, off ops stay off, ratios still valid
	for (int alg = 0; alg < 32; alg++) {
		Voice v = Voice::Init(); v.set(G_ALG, alg);
		for (int o = 0; o < 6; o++) v.set(OpOff(o, OP_OL), 85);
		v.set(OpOff(5, OP_OL), 0);
		Rng rng(100 + alg);
		for (int i = 0; i < 300; i++) Mutate(v, rng, 100);
		for (int off = 0; off < G_NAME; off++) CHECK(v.d[off] <= ParamMax(off));
		CHECK(v.op(5, OP_OL) == 0);
		bool loud = false;
		for (int o = 0; o < 6; o++) if (IsCarrier(alg, o) && v.op(o, OP_OL) >= 85) loud = true;
		CHECK(loud || CarrierCount(alg) == 0 || v.op(5, OP_OL) == 0);
		CHECK(v.algorithm() == alg);     // algorithm itself never changes
		for (int o = 0; o < 6; o++) if (IsCarrier(alg, o)) CHECK(v.op(o, OP_PC) == 1 && v.op(o, OP_PF) == 0);
	}
	// strength 1 changes few params, 100 changes more on average
	int lo = 0, hi = 0;
	for (int i = 0; i < 200; i++) {
		Voice t = Voice::Init(); t.set(G_ALG, 4); for (int o = 0; o < 6; o++) t.set(OpOff(o, OP_OL), 80);
		Voice t2 = t; Rng ra(i + 1), rb(i + 1);
		lo += Mutate(t, ra, 1); hi += Mutate(t2, rb, 100);
	}
	CHECK(hi > lo);
}

static void test_session()
{
	RecordSink sink;
	EditSession s(&sink, 5);
	Voice v = Voice::Init(); v.set(G_ALG, 4);
	for (int o = 0; o < 6; o++) v.set(OpOff(o, OP_OL), 80);
	s.load(v);
	CHECK(!s.dirty());
	CHECK(s.setParam(G_FB, 5));
	CHECK(s.dirty() && sink.last.feedback() == 5);

	// COMPARE: hold hears stored, release gives the edit back, edits ignored while held
	s.compareBegin();
	CHECK(sink.last.feedback() == 0);
	CHECK(!s.setParam(G_FB, 7));
	s.mutate(); s.init(); s.revert();
	CHECK(s.edit().feedback() == 5);
	s.compareEnd();
	CHECK(sink.last.feedback() == 5);

	// REVERT
	s.revert();
	CHECK(!s.dirty() && sink.last.feedback() == 0);

	// MUTATE then REVERT restores exactly
	s.mutate(100);
	CHECK(s.dirty());
	s.revert();
	CHECK(s.edit() == v);

	// macros: set, bake on direct edit, return-to-zero
	s.setMacro(M_SNARL, 99);
	CHECK(s.edit().feedback() == 7);
	s.setMacro(M_SNARL, 0);
	CHECK(s.edit() == v);
	s.setMacro(M_BRIGHT, 20);
	int bright = s.edit().op(1, OP_OL);
	CHECK(bright > 80);
	s.setParam(G_FB, 3);                     // bakes
	CHECK(s.macro(M_BRIGHT) == 0 && s.edit().op(1, OP_OL) == bright && s.edit().feedback() == 3);
	s.setMacro(M_BRIGHT, 10);                // relative to the new baseline
	s.setParam(G_OKS, 0, false);             // non-baking edit survives later macro moves
	s.setMacro(M_BRIGHT, 20);
	CHECK(s.edit().d[G_OKS] == 0 && s.macro(M_BRIGHT) == 20);

	// OP MUTE affects sound only, never the stored edit
	s.load(v);
	s.toggleOpMute(1);
	CHECK(sink.last.op(1, OP_OL) == 0 && s.edit().op(1, OP_OL) == 80 && !s.dirty());
	s.toggleOpMute(1);
	CHECK(sink.last.op(1, OP_OL) == 80);

	// INIT
	s.init();
	CHECK(s.edit() == Voice::Init());

	// PLAY: one note on / off, loading a new voice releases a held note
	int ons = sink.ons;
	s.playDown(60); s.playDown(60);
	CHECK(sink.ons == ons + 1 && s.playing());
	s.playUp(); s.playUp();
	CHECK(sink.offs == 1 && !s.playing());
	s.playDown(62);
	s.load(v);
	CHECK(sink.offs == 2 && !s.playing());

	s.setParam(G_FB, 2);
	s.markSaved();
	CHECK(!s.dirty());
}

static void test_keys()
{
	KeysModel k;       // octave 3, minor, root C
	CHECK(k.noteForKey(0) == 60 && k.noteForKey(1) == 62 && k.noteForKey(2) == 63);
	CHECK(k.noteForKey(6) == 70 && k.noteForKey(7) == 72);
	CHECK(strcmp(k.keyLabel(2), "Eb") == 0 && k.isRoot(0) && k.isRoot(7) && !k.isRoot(1));
	k.setRoot(2); k.setScale(1);
	CHECK(k.noteForKey(0) == 62 && k.noteForKey(2) == 66);
	k.setScale(8); // pentatonic wraps into the next octave after 5 notes
	CHECK(k.noteForKey(5) == 62 + 12);
	k.setOctave(99); CHECK(k.octave == 6);
	k.setOctave(6); k.setRoot(11); k.setScale(0);
	CHECK(k.noteForKey(7) <= 127);
	CHECK(KeysModel::KeyIndex('q') == 0 && KeysModel::KeyIndex('f') == 7 && KeysModel::KeyIndex('z') == -1);

	// held notes survive a scale change
	RecordSink sink; KeysModel m;
	m.keyDown(1, &sink); m.setScale(1); m.setOctave(4);
	m.keyUp(1, &sink);
	CHECK(sink.ons == 1 && sink.offs == 1 && sink.lastNote == 62);
	m.keyDown(0, &sink); m.keyDown(3, &sink); m.allOff(&sink);
	CHECK(sink.offs == 3 && !m.held(0));
}

static void test_browse()
{
	FakeBank bank; BrowseModel br(&bank);
	Voice cur = Voice::Init(); cur.set(G_FB, 6);
	br.begin(cur, 1, 30, 2);
	CHECK(br.active() && br.target() == 2);
	CHECK(br.moveVoice(3) && br.index() == 1);   // wraps 30 -> 1
	CHECK(br.preview().feedback() == 2 * 1 + 1);
	CHECK(br.moveBank(1) && br.bank() == 2);
	CHECK(!br.moveBank(1) && br.bank() == 2);    // clamped
	CHECK(br.firstVisible(8) == 0);
	br.begin(cur, 0, 31, 0);
	CHECK(br.firstVisible(8) == 24);
	br.begin(cur, 0, 15, 0);
	CHECK(br.firstVisible(8) == 11);
	br.moveVoice(1);
	CHECK(br.cancel() == cur);
	br.begin(cur, 0, 0, 0); br.moveVoice(1);
	Voice p = br.preview();
	CHECK(br.commit() == p && !br.active());
	char nm[11]; CHECK(br.rowName(3, nm) && strcmp(nm, "V0-03") == 0);
}

static void test_pages()
{
	RecordSink sink; EditSession s(&sink);
	FakeBus bus; FakeBank bank; BrowseModel br(&bank); KeysModel keys;
	UiCtx c; c.session = &s; c.bus = &bus; c.keys = &keys; c.browse = &br;
	Voice v = Voice::Init(); v.set(G_ALG, 4);
	for (int o = 0; o < 6; o++) v.set(OpOff(o, OP_OL), 76);
	s.load(v);
	LaneView lv;

	// Every lane of every page reads and adjusts without crashing, with sane output
	for (int p = 0; p < kPageCount; p++) {
		const Page& pg = GetPage((PageId)p);
		br.begin(v, 0, 0, 0);
		for (int l = 0; l < 4; l++)
			for (int shift = 0; shift < 2; shift++) {
				c.shift = shift;
				for (int d = -150; d <= 150; d += 75) {
					AdjustLane(pg, l, c, d);
					ReadLane(pg, l, c, lv);
					CHECK(lv.label[0] && lv.text[0]);
					CHECK(lv.frac >= 0.0f && lv.frac <= 1.0f);
				}
			}
		s.load(v);
	}
	c.shift = false; c.op = 0; s.load(v);

	// FM page
	const Page& fm = GetPage(PG_FM);
	ReadLane(fm, 0, c, lv); CHECK(strcmp(lv.text, "5") == 0 && strcmp(lv.label, "Algo") == 0);
	AdjustLane(fm, 0, c, 2); ReadLane(fm, 0, c, lv); CHECK(strcmp(lv.text, "7") == 0);
	AdjustLane(fm, 2, c, 18); ReadLane(fm, 2, c, lv);
	CHECK(strcmp(lv.text, "+18") == 0 && lv.bipolar && lv.frac > 0.5f);
	const Page& fm2 = GetPage(PG_FM_MACROS2);
	AdjustLane(fm2, 3, c, -1); ReadLane(fm2, 3, c, lv); CHECK(strcmp(lv.text, "OFF") == 0);
	AdjustLane(fm2, 3, c, 1); ReadLane(fm2, 3, c, lv); CHECK(strcmp(lv.text, "ON") == 0);

	// OPS page: op select drives Level/Ratio/Detune
	s.load(v); c.op = 0;
	const Page& ops = GetPage(PG_OPS);
	AdjustLane(ops, 0, c, 5); CHECK(c.op == 5);
	AdjustLane(ops, 0, c, 5); CHECK(c.op == 5);
	AdjustLane(ops, 0, c, -99); CHECK(c.op == 0);
	AdjustLane(ops, 0, c, 2); ReadLane(ops, 0, c, lv); CHECK(strcmp(lv.text, "3") == 0);
	AdjustLane(ops, 1, c, 3); ReadLane(ops, 1, c, lv); CHECK(strcmp(lv.text, "79") == 0);
	AdjustLane(ops, 2, c, 4); ReadLane(ops, 2, c, lv); CHECK(strcmp(lv.text, "5.00") == 0);  // coarse 1 -> 5
	c.shift = true; AdjustLane(ops, 2, c, 50); c.shift = false;
	ReadLane(ops, 2, c, lv); CHECK(strcmp(lv.text, "7.50") == 0);                            // fine +50
	AdjustLane(ops, 3, c, 3); ReadLane(ops, 3, c, lv); CHECK(strcmp(lv.text, "+3") == 0);
	s.setParam(OpOff(2, OP_PM), 1); s.setParam(OpOff(2, OP_PC), 2); s.setParam(OpOff(2, OP_PF), 0);
	ReadLane(ops, 2, c, lv); CHECK(strcmp(lv.text, "100Hz") == 0);

	// ENV page: rates, Shift = levels
	c.op = 1;
	const Page& env = GetPage(PG_ENV);
	ReadLane(env, 0, c, lv); CHECK(strcmp(lv.label, "RATE 1") == 0);
	c.shift = true; ReadLane(env, 2, c, lv); CHECK(strcmp(lv.label, "LEVEL 3") == 0);
	int l3 = s.param(OpOff(1, OP_L3));
	AdjustLane(env, 2, c, -4); CHECK(s.param(OpOff(1, OP_L3)) == l3 - 4);
	c.shift = false;

	// MOD pages
	const Page& lfo = GetPage(PG_LFO);
	AdjustLane(lfo, 0, c, 40); ReadLane(lfo, 0, c, lv); CHECK(strcmp(lv.text, "SAW DN") == 0); // one step per detent
	const Page& peg = GetPage(PG_PEG);
	c.shift = true; AdjustLane(peg, 0, c, 10); ReadLane(peg, 0, c, lv); c.shift = false;
	CHECK(strcmp(lv.text, "+10") == 0 && lv.bipolar);
	const Page& fx = GetPage(PG_FX);
	for (int i = 0; i < kBusParamCount; i++) bus.v[i] = 0;
	AdjustLane(fx, 0, c, 30); CHECK(bus.v[BP_REVERB_SIZE] == 30);
	AdjustLane(fx, 3, c, 5); CHECK(bus.v[BP_COMPRESSOR] == 1);
	AdjustLane(fx, 3, c, -5); CHECK(bus.v[BP_COMPRESSOR] == 0);

	// KEYS page
	const Page& kp = GetPage(PG_KEYS);
	keys = KeysModel();
	ReadLane(kp, 1, c, lv); CHECK(strcmp(lv.text, "MINOR") == 0);
	AdjustLane(kp, 1, c, -5); ReadLane(kp, 1, c, lv); CHECK(strcmp(lv.text, "MAJOR") == 0);

	// sub-page cycling
	CHECK(FirstPageOfKey('w') == PG_FM && FirstPageOfKey('e') == PG_OPS && FirstPageOfKey('r') == PG_LFO);
	CHECK(FirstPageOfKey('q') == kPageCount);
	CHECK(NextSubPage(PG_OPS) == PG_ENV && NextSubPage(PG_ENV) == PG_OPS);
	CHECK(NextSubPage(PG_LFO) == PG_PEG && NextSubPage(PG_PEG) == PG_FX && NextSubPage(PG_FX) == PG_LFO);
	CHECK(SubPageCount(PG_FM) == 2 && SubPageCount(PG_FX) == 3 && SubPageIndex(PG_FX) == 2);

	// Shift map covers all 12 keys with matching actions
	for (const char* k = "qwerasdfzxcv"; *k; k++) CHECK(FindKey(*k) != nullptr);
	CHECK(FindKey('w')->action == SA_MUTATE && FindKey('c')->action == SA_REVERT && FindKey('p') == nullptr);
}

int main()
{
	test_voice(); test_layout(); test_macros(); test_mutate(); test_session(); test_keys(); test_browse(); test_pages();
	printf("%d checks, %d failed\n", g_checks, g_fail);
	return g_fail ? 1 : 0;
}
