// Host tests for the screen framework: key routing, encoders, pages, drawing bounds.
#include "zicpages.h"
#include "fakesynth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace zic;

static int fails = 0, checks = 0;
#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

struct Rig {
	Gfx g;
	FakeSynth syn;
	Ui ui;
	uint32_t ms = 1000;
	Rig() : ui(g, syn) { RegisterPages(ui); ui.start(ms); }
	void run(uint32_t dt) { for (uint32_t t = 0; t < dt; t += 20) { ms += 20; ui.tick(ms); } }
	void down(int k) { ui.key(k, true, ms); run(20); }
	void up(int k) { ui.key(k, false, ms); run(20); }
	void press(int k) { down(k); up(k); }
	void turn(int e, int n) { ui.encoder(e, n, n > 0 ? 1 : -1, ms); run(20); }
	int pixelsDirty() { return g.dirtyPixels(); }
};

static void flush(Gfx& g) { Rect r; static uint16_t buf[W * H]; while (g.takeChunk(r, buf, 5000)) {} }

int main()
{
	{	// encoders edit the current TG's parameters; Shift gives the raw (fine) step
		Rig r;
		CHECK(r.syn.get(P_CUTOFF, 0) == 72);
		r.turn(0, 5);
		CHECK(r.syn.get(P_CUTOFF, 0) == 77);
		r.ui.key(K_Z, true, r.ms);
		r.ui.encoder(0, 6, 1, r.ms);                 // accelerated 6, raw 1
		CHECK(r.syn.get(P_CUTOFF, 0) == 78);
		r.ui.key(K_Z, false, r.ms);
		r.turn(0, 1000);
		CHECK(r.syn.get(P_CUTOFF, 0) == 99);        // clamped
		r.turn(2, -200);
		CHECK(r.syn.get(P_DETUNE, 0) == -99);
		r.turn(3, 7);
		CHECK(r.syn.get(P_VOLUME, 0) == 107);
	}
	{	// page keys: q cycles the VOICE sub-pages; FM/OPS/MOD are not in this build
		Rig r;
		CHECK(r.ui.slot() == SLOT_VOICE && r.ui.sub() == 0);
		r.press(K_Q); CHECK(r.ui.sub() == 1);
		r.turn(0, 1); CHECK(r.syn.get(P_MONO, 0) == 1);
		r.press(K_Q); CHECK(r.ui.sub() == 2);
		r.turn(3, 1); CHECK(r.syn.get(P_PROGRAM, 0) == 1);   // program steps one voice per click
		r.press(K_Q); CHECK(r.ui.sub() == 0);
		r.press(K_W); CHECK(r.ui.slot() == SLOT_VOICE);
		r.press(K_E); CHECK(r.ui.slot() == SLOT_VOICE);
		r.press(K_R); CHECK(r.ui.slot() == SLOT_VOICE);
		r.press(K_D); CHECK(r.ui.slot() == SLOT_VOICE);
		r.press(K_F); CHECK(r.ui.slot() == SLOT_MIX);
		r.press(K_V); CHECK(r.ui.slot() == SLOT_VOICE && r.ui.sub() == 0);   // EXIT goes home
	}
	{	// TG selection wraps; lanes follow the current TG
		Rig r;
		r.press(K_S); CHECK(r.ui.tg() == 1);
		r.turn(0, 1); CHECK(r.syn.get(P_CUTOFF, 1) == 73 && r.syn.get(P_CUTOFF, 0) == 72);
		r.press(K_A); r.press(K_A); CHECK(r.ui.tg() == 7);
		r.press(K_S); CHECK(r.ui.tg() == 0);
		r.syn.n = 4;
		r.press(K_A); CHECK(r.ui.tg() == 3);
	}
	{	// MIX: q mute, w solo, encoder 1 picks the TG
		Rig r;
		r.press(K_F);
		r.press(K_Q); CHECK(r.ui.muted(0) && r.syn.muted[0]);
		r.press(K_Q); CHECK(!r.ui.muted(0) && !r.syn.muted[0]);
		r.turn(0, 1); CHECK(r.ui.tg() == 1);
		r.press(K_W); CHECK(r.ui.soloed(1));
		CHECK(r.syn.muted[0] && !r.syn.muted[1] && r.syn.muted[2]);    // solo silences the others
		r.press(K_W); CHECK(!r.syn.muted[0] && !r.syn.muted[2]);
		r.turn(1, -300); CHECK(r.syn.get(P_VOLUME, 1) == 0);
		r.turn(2, 5); CHECK(r.syn.get(P_PAN, 1) == 55);
		CHECK(r.ui.slot() == SLOT_MIX);
	}
	{	// PLAY holds a note; COMPARE holds the stored voice; Shift+c reverts
		Rig r;
		r.down(K_X); CHECK(r.syn.held[0]);
		r.up(K_X); CHECK(!r.syn.held[0]);
		r.down(K_X); r.press(K_S); r.up(K_X);          // moving to another TG while held still releases the right note
		CHECK(!r.syn.held[0] && !r.syn.held[1]);
		r.down(K_C); CHECK(r.syn.compares == 1);
		r.up(K_C); CHECK(r.syn.compares == 0);
		r.down(K_Z); r.press(K_C); r.up(K_Z); CHECK(r.syn.reverts == 1);
	}
	{	// long v = panic; short v = exit; Shift+v = menu
		Rig r;
		r.syn.held[0] = true;
		r.down(K_V); r.run(800); r.up(K_V);
		CHECK(r.syn.panics == 1 && !r.syn.held[0]);
		r.down(K_Z); r.press(K_V); r.up(K_Z);
		CHECK(r.ui.menuRequested());
		CHECK(r.ui.suspended());
		r.ui.key(K_Q, true, r.ms);                       // ignored while the menu has the screen
		r.ui.resume(r.ms);
		CHECK(!r.ui.suspended());
	}
	{	// Shift map appears after a hold, not on a quick combo
		Rig r;
		flush(r.g);
		r.down(K_Z);
		r.run(100);
		CHECK(!r.g.dirty());
		r.run(300);
		CHECK(r.g.dirty());                              // map drawn
		r.up(K_Z);
		r.run(40);
		CHECK(r.g.dirty());                              // restored
	}
	{	// idle: nothing is redrawn, so nothing is sent to the panel
		Rig r;
		r.run(200);
		flush(r.g);
		r.run(1000);
		CHECK(r.g.dirtyPixels() == 0);
	}
	{	// every drawing call stays inside the screen (run under ASan/UBSan)
		Rig r;
		for (int page = 0; page < 2; page++) {
			for (int s = 0; s < 3; s++) {
				for (int k = 0; k < 4; k++) for (int d = -300; d <= 300; d += 150) r.turn(k, d);
				r.press(page ? K_F : K_Q);
			}
			r.press(K_F);
		}
		for (int i = 0; i < kMaxTG; i++) { r.syn.lvl[i] = 0.4f; r.syn.held[i] = true; }
		r.run(500);
		flush(r.g);
	}
	{	// a quiet voice with an empty name does not crash the layouts
		Rig r;
		memset(r.syn.names[0], ' ', kNameLen);
		r.syn.names[0][kNameLen] = 0;
		r.run(200);
		strcpy(r.syn.names[0], "ABCDEFGHIJ");
		r.run(200);
		CHECK(true);
	}
	printf("%d checks, %d failed\n", checks, fails);
	return fails ? 1 : 0;
}
