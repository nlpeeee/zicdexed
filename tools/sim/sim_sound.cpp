// Screenshots of the sound-design pages. usage: sim_sound <outdir>  (then ppm2png.py <outdir>)
#include "zicpages.h"
#include "soundsynth.h"
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
	SoundSynth syn;
	Ui ui;
	uint32_t ms = 1000;
	Sim() : ui(g, syn) { RegisterPages(ui); ui.start(ms); }
	void run(uint32_t dt) { for (uint32_t t = 0; t < dt; t += 20) { ms += 20; ui.tick(ms); } }
	void down(int k) { ui.key(k, true, ms); run(60); }
	void up(int k) { ui.key(k, false, ms); run(20); }
	void press(int k) { down(k); up(k); }
	void turn(int enc, int steps) { ui.encoder(enc, steps, steps > 0 ? 1 : -1, ms); run(20); }
	void shiftPress(int k) { ui.key(K_Z, true, ms); run(40); press(k); ui.key(K_Z, false, ms); run(20); }
};

int main(int argc, char** argv)
{
	std::string dir = argc > 1 ? argv[1] : ".";
	mkdir(dir.c_str(), 0755);
	Sim s;
	s.syn.set(P_PROGRAM, 0, 4);                     // algorithm 26, nice mixed shape
	s.syn.loadVoice(0, [] { static uint8_t v[kVoiceSize]; zd::Voice x = zd::Voice::Init();
		x.set(zd::G_ALG, 4); x.set(zd::G_FB, 7);
		const int lv[6] = { 99, 82, 99, 88, 97, 76 }, pc[6] = { 1, 1, 0, 1, 2, 3 }, pf[6] = { 0, 0, 0, 1, 0, 50 };
		for (int o = 0; o < 6; o++) { x.set(zd::OpOff(o, zd::OP_OL), lv[o]); x.set(zd::OpOff(o, zd::OP_PC), pc[o]); x.set(zd::OpOff(o, zd::OP_PF), pf[o]);
			x.set(zd::OpOff(o, zd::OP_R1), 92); x.set(zd::OpOff(o, zd::OP_R2), 60); x.set(zd::OpOff(o, zd::OP_R3), 40); x.set(zd::OpOff(o, zd::OP_R4), 55);
			x.set(zd::OpOff(o, zd::OP_L1), 99); x.set(zd::OpOff(o, zd::OP_L2), 80); x.set(zd::OpOff(o, zd::OP_L3), 70); x.set(zd::OpOff(o, zd::OP_L4), 0); }
		x.set(zd::OpOff(5, zd::OP_PD), 10);
		x.set(zd::G_LFW, 0); x.set(zd::G_LFS, 62); x.set(zd::G_LPMD, 0); x.set(zd::G_LAMD, 70);
		x.setName("REESE BS"); memcpy(v, x.d, kVoiceSize); return v; }());
	s.run(100);

	s.press(K_W);                                   // FM
	s.turn(2, 18);                                  // Bright +18
	s.run(900);
	dump(s.g, dir, "02_fm");
	s.press(K_W);
	dump(s.g, dir, "02b_fm_macros2");
	for (int alg = 0; alg < 32; alg += 1) {         // contact sheet material: every algorithm
		s.syn.voice[0][zd::G_ALG] = (uint8_t)alg;
		s.press(K_Q); s.press(K_W);
		char name[32]; snprintf(name, sizeof name, "alg_%02d", alg + 1);
		s.run(100);
		dump(s.g, dir, name);
	}
	s.syn.voice[0][zd::G_ALG] = 4;

	s.press(K_E);                                   // OPS
	s.turn(0, 5);                                   // op 6
	dump(s.g, dir, "03_ops");
	s.press(K_E);                                   // envelope
	dump(s.g, dir, "04_envelope");

	s.press(K_R);                                   // MOD: LFO
	s.run(500);
	dump(s.g, dir, "05_mod");
	s.press(K_R);                                   // pitch EG
	s.ui.key(K_Z, true, s.ms); s.turn(1, 15); s.turn(2, -12); s.ui.key(K_Z, false, s.ms);
	dump(s.g, dir, "05b_peg");
	s.press(K_R);                                   // FX
	s.turn(0, 40); s.turn(2, 25); s.turn(3, 1);
	dump(s.g, dir, "05c_fx");

	s.press(K_D);                                   // BROWSE
	s.turn(0, 3);
	dump(s.g, dir, "07_browse");

	s.press(K_V);
	s.ui.key(K_Z, true, s.ms); s.run(40); s.press(K_X); s.ui.key(K_Z, false, s.ms);   // Shift+x: KEYS
	s.down(K_Q); s.down(K_S);
	dump(s.g, dir, "08_keys");
	s.up(K_Q); s.up(K_S);

	s.press(K_V);
	s.press(K_W); s.shiftPress(K_W);                // MUTATE
	s.run(300);
	dump(s.g, dir, "11_mutated");

	// SAVE VOICE: the naming screen, one voice already in slot 1 under the same name
	s.press(K_V);
	s.shiftPress(K_D);
	s.turn(0, 4); s.press(K_W); s.press(K_W); s.turn(0, 1);
	s.run(2000);
	dump(s.g, dir, "12_save_new");
	s.press(K_E);
	s.shiftPress(K_D);
	s.run(100);
	dump(s.g, dir, "13_save_overwrite");

	// RENAME: the VOICE page's last sub-page
	s.press(K_V);
	s.press(K_Q); s.press(K_Q); s.press(K_Q);
	s.run(2000);
	dump(s.g, dir, "14_rename");

	// BROWSE on the user bank: key 3 reads RENAME
	s.press(K_V);
	s.press(K_D);
	s.turn(1, 3);
	s.run(2000);
	dump(s.g, dir, "15_browse_user");

	// BROWSE key 2: performances
	s.press(K_V);
	s.press(K_D); s.press(K_W);
	s.turn(0, 2);
	s.run(2000);
	dump(s.g, dir, "16_browse_perfs");

	// SAVE VOICE with an unreadable user bank file: the RESET screen, then armed
	s.press(K_V);
	s.syn.userStatus = -3;
	s.shiftPress(K_D);
	s.run(500);
	dump(s.g, dir, "17_save_reset");
	s.press(K_E);
	s.run(200);
	dump(s.g, dir, "18_save_reset_armed");
	return 0;
}
