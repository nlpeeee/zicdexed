// PC simulator: runs the real UI code against a fake synth and writes frames as PPM.
// usage: sim <outdir>   (then tools/sim/ppm2png.py <outdir>)
#include "zicpages.h"
#include "fakesynth.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string>

using namespace zic;

static void dump(Gfx& g, const std::string& dir, const char* name)
{
	std::string p = dir + "/" + name + ".ppm";
	FILE* f = fopen(p.c_str(), "wb");
	fprintf(f, "P6 %d %d 255\n", W, H);
	for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) {
		Color c = g.pixel(x, y);
		fputc(c >> 16, f); fputc(c >> 8, f); fputc(c, f);
	}
	fclose(f);
}

struct Sim {
	Gfx g;
	FakeSynth syn;
	Ui ui;
	uint32_t ms = 1000;
	Sim() : ui(g, syn) { RegisterPages(ui); ui.start(ms); }
	void run(uint32_t dt) { for (uint32_t t = 0; t < dt; t += 20) { ms += 20; ui.tick(ms); } }
	void press(int k, uint32_t hold = 100) { ui.key(k, true, ms); run(hold); ui.key(k, false, ms); run(20); }
	void turn(int enc, int steps) { ui.encoder(enc, steps, steps > 0 ? 1 : -1, ms); run(20); }
};

int main(int argc, char** argv)
{
	std::string dir = argc > 1 ? argv[1] : ".";
	mkdir(dir.c_str(), 0755);
	Sim s;
	s.syn.lvl[0] = 0.3f; s.syn.lvl[1] = 0.1f; s.syn.lvl[2] = 0.05f;
	s.syn.held[0] = true;
	s.run(400);
	dump(s.g, dir, "01_voice");

	s.turn(0, 3);                                   // cutoff, lane highlights
	dump(s.g, dir, "01b_voice_touched");
	s.run(800);

	s.press(K_Q);                                   // q again: play sub-page
	dump(s.g, dir, "01c_voice_play");
	s.press(K_Q);                                   // mix sub-page
	dump(s.g, dir, "01d_voice_mix");

	s.press(K_F);                                   // MIX page
	s.syn.lvl[3] = 0.2f;
	s.run(400);
	dump(s.g, dir, "06_mix");
	s.press(K_Q);                                   // mute TG1
	s.press(K_S);                                   // TG2
	s.press(K_W);                                   // solo TG2
	s.run(200);
	dump(s.g, dir, "06b_mix_mute_solo");

	s.press(K_W);                                   // FM page: not in this build
	dump(s.g, dir, "02_fm_missing");
	s.run(2000);

	s.ui.key(K_Z, true, s.ms);                      // Shift held: key map
	s.run(400);
	dump(s.g, dir, "09_shift");
	s.ui.key(K_Z, false, s.ms);
	s.run(100);
	s.press(K_V);
	s.ui.key(K_C, true, s.ms);
	s.run(100);
	dump(s.g, dir, "10_compare");
	s.ui.key(K_C, false, s.ms);
	return 0;
}
