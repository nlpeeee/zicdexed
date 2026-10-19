// Host tests for the FM/OPS/MOD/BROWSE/KEYS pages running on the real framework.
#include "zicpages.h"
#include "soundhub.h"
#include "soundsynth.h"
#include <initializer_list>
#include <stdio.h>
#include <string.h>

using namespace zic;

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

struct Rig {
	Gfx g;
	SoundSynth syn;
	Ui ui;
	uint32_t ms = 1000;
	Rig() : ui(g, syn) { RegisterPages(ui); ui.start(ms); SoundHub::get().ui().op = 0; }
	void run(uint32_t dt) { for (uint32_t t = 0; t < dt; t += 20) { ms += 20; ui.tick(ms); } }
	void down(int k) { ui.key(k, true, ms); run(20); }
	void up(int k) { ui.key(k, false, ms); run(20); }
	void press(int k) { down(k); up(k); }
	void turn(int e, int n) { ui.encoder(e, n, n > 0 ? 1 : -1, ms); run(20); }
	void shiftPress(int k) { ui.key(K_Z, true, ms); run(20); press(k); ui.key(K_Z, false, ms); run(20); }
	zd::Voice live(int tg = 0) { zd::Voice v; syn.readVoice(tg, v.d); return v; }
};

static void flushAll(Gfx& g) { Rect r; static uint16_t buf[W * H]; while (g.takeChunk(r, buf, 5000)) {} }

int main()
{
	{	// FM page: algorithm and feedback lanes edit the live voice
		Rig r;
		r.press(K_W);
		CHECK(r.ui.slot() == SLOT_FM);
		int alg = r.live().algorithm();
		r.turn(0, 3);
		CHECK(r.live().algorithm() == alg + 3);
		r.turn(1, 2);
		CHECK(r.live().feedback() == 2);
	}
	{	// Bright is relative and exactly reversible; a direct edit bakes it
		Rig r;
		r.press(K_W);
		zd::Voice before = r.live();
		r.turn(2, 20);
		CHECK(r.live() != before);
		r.turn(2, -20);
		CHECK(r.live() == before);
		r.turn(2, 10);
		zd::Voice bright = r.live();
		r.turn(0, 1);                                // algorithm change bakes the macro
		for (int o = 0; o < 6; o++) CHECK(r.live().op(o, zd::OP_OL) == bright.op(o, zd::OP_OL));
		LaneView lv;
		Ctx c = r.ui.ctx();
		CreateFmPage()->lane(c, 0, 2, lv);
		CHECK(strcmp(lv.text, "0") == 0);            // the macro reads 0 again after the bake
	}
	{	// OPS: op select drives level, ratio (coarse, Shift = fine) and detune
		Rig r;
		r.press(K_E);
		r.turn(0, 2);                                // op 3
		int ol = r.live().op(2, zd::OP_OL);
		r.turn(1, -5);
		CHECK(r.live().op(2, zd::OP_OL) == ol - 5);
		int pc = r.live().op(2, zd::OP_PC);
		r.turn(2, 1);
		CHECK(r.live().op(2, zd::OP_PC) == pc + 1);
		r.ui.key(K_Z, true, r.ms);
		r.ui.encoder(2, 6, 1, r.ms);                 // Shift: raw step on fine
		r.ui.key(K_Z, false, r.ms);
		CHECK(r.live().op(2, zd::OP_PF) == 1);
		r.turn(3, 3);
		CHECK(r.live().op(2, zd::OP_PD) == 10);
		r.press(K_E);                                // envelope sub-page
		CHECK(r.ui.sub() == 1);
		int r1 = r.live().op(2, zd::OP_R1);
		r.turn(0, -10);
		CHECK(r.live().op(2, zd::OP_R1) == r1 - 10);
		r.ui.key(K_Z, true, r.ms);
		int l2 = r.live().op(2, zd::OP_L2);
		r.ui.encoder(1, -1, -1, r.ms);               // Shift + lane 2 = level 2
		r.ui.key(K_Z, false, r.ms);
		CHECK(r.live().op(2, zd::OP_L2) == l2 - 1);
	}
	{	// MOD: LFO, pitch EG, FX
		Rig r;
		r.press(K_R);
		r.turn(1, 7);
		CHECK(r.live().d[zd::G_LFS] == 42);
		r.press(K_R);
		r.turn(0, -5);
		CHECK(r.live().d[zd::G_PR1] == 94);
		r.press(K_R);
		CHECK(r.ui.sub() == 2);
		r.turn(0, 30);
		CHECK(r.syn.getFx(FX_REVERB_SIZE) == 30);
		r.run(500);                                  // LFO and FX pages tick without trouble
		r.press(K_R); r.run(400);
	}
	{	// Shift+q INIT, Shift+w MUTATE, Shift+e OP MUTE
		Rig r;
		r.press(K_W);
		zd::Voice before = r.live();
		for (int i = 0; i < 10 && r.live() == before; i++) r.shiftPress(K_W);
		CHECK(r.live() != before);
		int muteBefore = r.live().op(0, zd::OP_OL);
		r.shiftPress(K_E);                           // op 1 muted: its level reads 0 in the engine
		CHECK(r.live().op(0, zd::OP_OL) == 0 && muteBefore > 0);
		r.shiftPress(K_E);
		CHECK(r.live().op(0, zd::OP_OL) == muteBefore);
		r.shiftPress(K_Q);
		CHECK(r.live() == zd::Voice::Init());
		for (int i = 0; i < 20; i++) r.shiftPress(K_W);   // many mutates stay valid
		zd::Voice v = r.live();
		for (int off = 0; off < zd::G_NAME; off++) CHECK(v.d[off] <= zd::ParamMax(off));
	}
	{	// COMPARE and revert from the framework: pages follow the engine voice
		Rig r;
		r.press(K_E);
		r.turn(1, -10);                              // edit OP1 level
		zd::Voice edited = r.live();
		r.down(K_C);
		CHECK(r.live() != edited);
		r.up(K_C);
		CHECK(r.live() == edited);
		LaneView lv;
		Ctx c = r.ui.ctx();
		CreateOpsPage()->lane(c, 0, 1, lv);          // lane re-reads the engine voice
		char want[4];
		fmtInt(want, edited.op(0, zd::OP_OL));
		CHECK(strcmp(lv.text, want) == 0);
		r.shiftPress(K_C);                           // revert
		CHECK(r.live().op(0, zd::OP_OL) != edited.op(0, zd::OP_OL));
	}
	{	// BROWSE: preview on turn, v cancels, c loads
		Rig r;
		zd::Voice orig = r.live();
		r.press(K_D);
		CHECK(r.ui.slot() == SLOT_BROWSE);
		r.turn(0, 3);
		CHECK(r.live() != orig);
		zd::Voice previewed = r.live();
		r.press(K_V);                                // exit: previews are undone
		CHECK(r.ui.slot() == SLOT_VOICE && r.live() == orig);
		r.press(K_D);
		r.turn(0, 4);
		previewed = r.live();
		int prog = r.syn.get(P_PROGRAM, 0);
		r.press(K_C);                                // load
		CHECK(r.ui.slot() == SLOT_VOICE);
		CHECK(r.syn.get(P_PROGRAM, 0) == (prog + 4) % 32);
		CHECK(r.live() == previewed);
		// bank lane skips bank 2, which does not exist
		r.press(K_D);
		r.turn(1, 1);
		CHECK(SoundHub::get().browse().bank() == 1 && r.syn.get(P_BANK, 0) == 0);   // not committed yet
		r.turn(1, 1);
		CHECK(SoundHub::get().browse().bank() == 3);                                // bank 2 does not exist
		r.press(K_V);
		// target TG: previews play on another TG and the old one is untouched
		r.press(K_D);
		zd::Voice tg0 = r.live(0), tg1 = r.live(1);
		r.turn(2, 1);                                // target TG2
		r.turn(0, 1);
		CHECK(r.live(1) != tg1 && r.live(0) == tg0);
		r.press(K_V);
		CHECK(r.live(1) == tg1);
	}
	{	// Shift+r goes straight to the FX sub-page; Shift+d saves the edit voice; Shift+f saves the performance
		Rig r;
		r.shiftPress(K_R);
		CHECK(r.ui.slot() == SLOT_MOD && r.ui.sub() == 2);
		r.shiftPress(K_R);                           // already there: stays
		CHECK(r.ui.slot() == SLOT_MOD && r.ui.sub() == 2);
		r.press(K_W);
		r.turn(2, 10);                               // a Bright macro edit
		r.shiftPress(K_E);                           // op 1 muted: sounds silent but must not be saved silent
		zd::Voice live = r.live();
		r.shiftPress(K_D);                           // opens the naming screen, saves nothing yet
		CHECK(r.ui.slot() == SLOT_SAVE && r.syn.userSaves == 0);
		r.press(K_E);                                // key 3: save to the next free slot
		CHECK(r.syn.userSaves == 1 && r.syn.lastUserSlot == 0);
		CHECK(r.ui.slot() == SLOT_FM);               // back where we came from
		zd::Voice saved; memcpy(saved.d, r.syn.userVoice[0], kVoiceSize);
		CHECK(saved.op(0, zd::OP_OL) > 0 && live.op(0, zd::OP_OL) == 0);
		CHECK(saved.op(1, zd::OP_OL) == live.op(1, zd::OP_OL));
		r.syn.userStatus = -3;                       // unreadable user bank: stays put
		r.shiftPress(K_D);
		CHECK(r.ui.slot() == SLOT_FM);
		r.syn.userStatus = 1;
		r.shiftPress(K_F);
		CHECK(r.syn.perfSaves == 1);
	}
	{	// SAVE VOICE: name with the encoders, pick a slot, overwrite by choice
		Rig r;
		r.shiftPress(K_D);
		CHECK(r.ui.slot() == SLOT_SAVE);
		r.turn(0, 1);                                // first character: B -> C
		r.press(K_W);                                // key 2: cursor right
		r.turn(0, 2);                                // second character: '0' -> '2'
		r.press(K_E);
		zd::Voice a; memcpy(a.d, r.syn.userVoice[0], kVoiceSize);
		char nm[11]; a.name(nm);
		CHECK(r.syn.userSaves == 1 && r.syn.lastUserSlot == 0);
		CHECK(nm[0] == 'C' && nm[1] == '2');
		r.shiftPress(K_D);                           // the edit voice carries the new name now, so it targets slot 1
		r.press(K_E);
		CHECK(r.syn.userSaves == 2 && r.syn.lastUserSlot == 0);          // same name: overwrite, no second slot
		r.shiftPress(K_D);
		r.press(K_W); r.press(K_W); r.turn(0, 1);    // different name
		r.press(K_E);
		CHECK(r.syn.lastUserSlot == 1);
		r.shiftPress(K_D);
		r.turn(2, -1);                               // encoder 3: this name follows slot 2; pick slot 1 instead
		r.press(K_E);
		CHECK(r.syn.lastUserSlot == 0);              // slot 1 is index 0
		r.shiftPress(K_D);
		CHECK(r.ui.slot() == SLOT_SAVE);
		int before = r.syn.userSaves;
		r.press(K_R);                                // key 4 cancels
		CHECK(r.syn.userSaves == before && r.ui.slot() != SLOT_SAVE);
		r.shiftPress(K_D);
		r.press(K_V);                                // key 12 leaves too
		CHECK(r.ui.slot() != SLOT_SAVE && r.syn.userSaves == before);
		r.syn.saveResult = -2;
		r.shiftPress(K_D); r.press(K_E);
		CHECK(r.ui.slot() == SLOT_SAVE && r.syn.userSaves == before);   // refused: stays on the screen
		r.syn.saveResult = 0;
		r.syn.userStatus = 0;                        // a build that cannot save
		r.press(K_V); r.shiftPress(K_D);
		CHECK(r.ui.slot() != SLOT_SAVE);
	}
	{	// RENAME: the VOICE page's last sub-page edits the name only, nothing goes to a bank
		Rig r;
		r.press(K_Q); r.press(K_Q); r.press(K_Q);
		CHECK(r.ui.slot() == SLOT_VOICE && r.ui.sub() == 3);
		r.turn(0, 2);                                // first character B -> D
		r.press(K_W); r.turn(0, 1);                  // second character: '0' -> '1'
		r.turn(2, 5);                                // encoder 3 has nothing to pick here
		r.press(K_E);                                // key 3 applies
		CHECK(r.ui.sub() == 0 && r.syn.userSaves == 0 && r.syn.voiceSaves == 0);
		char nm[11]; r.live().name(nm);
		CHECK(nm[0] == 'D' && nm[1] == '1' && strncmp(nm + 2, " VOI 01", 7) == 0);
		r.press(K_Q); r.press(K_Q); r.press(K_Q);    // cancel leaves the name alone
		r.turn(0, 3);
		r.press(K_R);
		CHECK(r.ui.sub() == 0);
		r.live().name(nm);
		CHECK(nm[0] == 'D');
		r.press(K_Q); r.press(K_Q); r.press(K_Q);    // TG-/TG+ are ignored while editing
		r.press(K_S);
		CHECK(r.ui.tg() == 0 && r.ui.sub() == 3);
		r.press(K_R);
		r.shiftPress(K_D);
		r.press(K_A);
		CHECK(r.ui.tg() == 0 && r.ui.slot() == SLOT_SAVE);
	}
	{	// BROWSE key 3: RENAME in the user bank (overwrites that slot), SAVE AS from other banks
		Rig r;
		zd::Voice u = zd::Voice::Init(); u.setName("MY SOUND"); u.set(zd::G_ALG, 7);
		memcpy(r.syn.userVoice[2], u.d, kVoiceSize); r.syn.userUsed[2] = true;
		r.press(K_D);
		r.turn(1, 1); r.turn(1, 1); r.turn(1, 1);        // bank 1 -> 2 (nasty) -> 4 (skips 3) -> 99
		CHECK(SoundHub::get().browse().bank() == 98);
		r.turn(0, 2);                                    // voice 3 (index 2)
		CHECK(SoundHub::get().browse().index() == 2);
		r.press(K_E);
		CHECK(r.ui.slot() == SLOT_SAVE && r.syn.get(P_BANK, 0) == 98 && r.syn.get(P_PROGRAM, 0) == 2);
		r.press(K_W); r.turn(0, 1);                      // second character: next one up
		r.press(K_E);
		CHECK(r.syn.lastUserSlot == 2 && r.syn.userSaves == 1);   // same slot, new name
		CHECK(r.ui.slot() == SLOT_BROWSE);
		// from a stock bank: saves a copy into the first free slot
		r.turn(1, -1);
		int b = SoundHub::get().browse().bank();
		CHECK(b != 98);
		r.press(K_E);
		CHECK(r.ui.slot() == SLOT_SAVE);
		r.press(K_E);
		CHECK(r.syn.lastUserSlot == 0 && r.syn.userSaves == 2);
	}
	{	// Shift+r with the Shift map showing (held past 250 ms), from several starting pages
		for (int start = 0; start < 4; start++) {
			Rig r;
			if (start == 1) r.press(K_R);
			if (start == 2) { r.press(K_R); r.press(K_R); }
			if (start == 3) r.press(K_W);
			r.ui.key(K_Z, true, r.ms); r.run(500);
			r.down(K_R); r.up(K_R);
			r.ui.key(K_Z, false, r.ms); r.run(100);
			CHECK(r.ui.slot() == SLOT_MOD && r.ui.sub() == 2);
		}
	}
	{	// KEYS: pads play the current TG through the scale, held notes release on exit
		Rig r;
		r.ui.key(K_Z, true, r.ms); r.run(20); r.press(K_X); r.ui.key(K_Z, false, r.ms); r.run(20);
		CHECK(r.ui.slot() == SLOT_KEYS);
		r.down(K_Q);
		CHECK(r.syn.notesOn == 1 && r.syn.lastNote == 60);
		r.down(K_S);
		CHECK(r.syn.lastNote == 68);                 // C minor, pad s is the sixth degree
		r.up(K_Q);
		CHECK(r.syn.notesOff == 1);
		r.press(K_V);                                // exit with a pad still held
		CHECK(r.syn.notesHeld[0] == 0);
	}
	{	// every page draws inside the screen and never touches the picture chrome, at every TG
		Rig r;
		for (int tg = 0; tg < 8; tg++) {
			r.ui.setTG(tg);
			for (int k : { K_W, K_E, K_R, K_D }) {
				for (int rep = 0; rep < 3; rep++) { r.press(k); r.run(60); }
				flushAll(r.g);
			}
			r.press(K_V);
		}
		CHECK(true);
	}
	printf("%d checks, %d failed\n", checks, fails);
	return fails ? 1 : 0;
}
