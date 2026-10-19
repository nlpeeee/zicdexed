// Host simulator: the real src/ui + src/logic against a fake SD card directory.
// Bank loading is MiniDexed's own CSysExFileLoader, SAVE VOICE is the real zicsave.cpp.
//   usage: sim_sd <workdir> <nasty bank .syx> [script]
// Without a script it runs the built-in checks; with one it plays the script (see docs/simulator.md).
// Writes screenshots (PPM) to <workdir>/shots, exits non-zero if a check fails.
#include "zicpages.h"
#include "sdsynth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <string>
#include <memory>
#include <vector>
#include <unistd.h>

using namespace zic;

static int g_fail = 0, g_pass = 0;
#define CHECK(c, ...) do { if (c) { g_pass++; printf("  ok   "); } else { g_fail++; printf("  FAIL "); } printf(__VA_ARGS__); printf("\n"); } while (0)

static std::string g_shots;
static void dump(Gfx& g, const char* name)
{
	std::string p = g_shots + "/" + name + ".ppm";
	FILE* f = fopen(p.c_str(), "wb");
	fprintf(f, "P6 %d %d 255\n", W, H);
	for (int y = 0; y < H; y++) for (int x = 0; x < W; x++) { Color c = g.pixel(x, y); fputc(c >> 16, f); fputc(c >> 8, f); fputc(c, f); }
	fclose(f);
}

struct Rig {                                   // one "power on"
	Gfx g;
	SdSynth syn;
	Ui ui;
	uint32_t ms = 1000;
	explicit Rig(const std::string& sd) : syn(sd), ui(g, syn) { RegisterPages(ui); ui.start(ms); run(200); }
	void run(uint32_t dt) { for (uint32_t t = 0; t < dt; t += 20) { ms += 20; ui.tick(ms); } }
	void down(int k) { ui.key(k, true, ms); run(60); }
	void up(int k) { ui.key(k, false, ms); run(20); }
	void press(int k) { down(k); up(k); }
	void turn(int enc, int steps) { ui.encoder(enc, steps, steps > 0 ? 1 : -1, ms); run(20); }
	void shiftPress(int k) { ui.key(K_Z, true, ms); run(40); press(k); ui.key(K_Z, false, ms); run(20); }
	std::string name(int tg) { char n[16]; syn.voiceName(tg, n); std::string s(n); while (!s.empty() && s.back() == ' ') s.pop_back(); return s; }
};

#include <stdarg.h>
static std::string sh(const char* fmt, ...) { char buf[2048]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap); return buf; }

static bool readFile(const std::string& p, std::vector<uint8_t>& out)
{
	FILE* f = fopen(p.c_str(), "rb");
	if (!f) return false;
	out.clear(); uint8_t b[4096]; size_t n;
	while ((n = fread(b, 1, sizeof b, f)) > 0) out.insert(out.end(), b, b + n);
	fclose(f);
	return true;
}
static std::string packedName(const std::vector<uint8_t>& f, int i)
{
	std::string s((const char*)&f[6 + i * 128 + 118], 10);
	while (!s.empty() && s.back() == ' ') s.pop_back();
	return s;
}

// Script mode. One command per line, '#' comments. Keys are numbered 1..12 in the order
// q w e r / a s d f / z x c v (so Shift = 9); encoders 1..4.
static int runScript(Rig& rig0, const std::string& sd, const char* path)
{
	std::unique_ptr<Rig> r(&rig0);
	FILE* f = fopen(path, "r");
	if (!f) { fprintf(stderr, "cannot open %s\n", path); r.release(); return 2; }
	char line[256];
	int n = 0, rc = 0;
	while (fgets(line, sizeof line, f)) {
		n++;
		char cmd[32] = "", arg[200] = "";
		int a = 0, b = 0;
		char* hash = strchr(line, '#'); if (hash) *hash = 0;
		if (sscanf(line, "%31s", cmd) != 1) continue;
		const char* rest = line + strlen(cmd) + (strstr(line, cmd) - line);
		sscanf(rest, "%d %d", &a, &b);
		sscanf(rest, " %199[^\n]", arg);
		const int k = a - 1;
		const bool keyOk = k >= 0 && k < kKeyCount;
		if (!strcmp(cmd, "press") && keyOk) r->press(k);
		else if (!strcmp(cmd, "down") && keyOk) r->down(k);
		else if (!strcmp(cmd, "up") && keyOk) r->up(k);
		else if (!strcmp(cmd, "shift") && keyOk) r->shiftPress(k);
		else if (!strcmp(cmd, "turn") && a >= 1 && a <= 4) r->turn(a - 1, b);
		else if (!strcmp(cmd, "wait")) r->run(a);
		else if (!strcmp(cmd, "shot")) { dump(r->g, arg); printf("shot %s\n", arg); }
		else if (!strcmp(cmd, "reboot")) { r.reset(new Rig(sd)); }
		else if (!strcmp(cmd, "mkdir")) { (void)!system(sh("mkdir -p '%s/%s'", sd.c_str(), arg).c_str()); }
		else if (!strcmp(cmd, "net")) { r->syn.net = arg; }
		else if (!strcmp(cmd, "print")) { for (int t = 0; t < r->syn.tgCount(); t++) printf("TG%d %s (bank %d, program %d)\n", t + 1, r->name(t).c_str(), r->syn.get(P_BANK, t) + 1, r->syn.get(P_PROGRAM, t) + 1); }
		else { fprintf(stderr, "%s:%d: bad command: %s", path, n, line); rc = 2; }
	}
	fclose(f);
	return rc;
}

int main(int argc, char** argv)
{
	if (argc < 3) { fprintf(stderr, "usage: sim_sd <workdir> <nasty bank .syx> [script]\n"); return 2; }
	const std::string wd = argv[1], sd = wd + "/sdcard";
	g_shots = wd + "/shots";
	(void)!system(sh("rm -rf '%s' && mkdir -p '%s/sdcard/sysex/voice' '%s/shots'", wd.c_str(), wd.c_str(), wd.c_str()).c_str());
	const std::string nastyPath = argv[2];
	const std::string nastyFile = nastyPath.substr(nastyPath.rfind('/') + 1);
	const int NB = atoi(nastyFile.c_str()) - 1;      // file number 1-based, bank index 0-based
	(void)!system(sh("cp '%s' '%s/sysex/voice/%s'", argv[2], sd.c_str(), nastyFile.c_str()).c_str());
	std::string script;
	if (argc > 3) { char rp[PATH_MAX]; if (!realpath(argv[3], rp)) { perror(argv[3]); return 2; } script = rp; }
	if (chdir(wd.c_str())) return 2;           // zicsave.cpp writes to the relative path sdcard/sysex/voice/000099_ZICUSER.syx

	if (argc > 3) {                            // script mode: the card starts with the nasty bank only
		return runScript(*new Rig(sd), sd, script.c_str());
	}
	std::vector<uint8_t> nasty;
	readFile(sd + "/sysex/voice/" + nastyFile, nasty);
	printf("[1] boot with bank %s on the card\n", nastyFile.c_str());
	std::unique_ptr<Rig> r(new Rig(sd));
	CHECK(r->syn.bankCount() == NB + 1 && r->syn.bankValid(NB), "%s loads as bank index %d (bankCount=%d)", nastyFile.c_str(), NB, r->syn.bankCount());
	{ char bn[32]; r->syn.bankName(NB, bn); CHECK(!strcmp(bn, "nasty"), "bank name '%s'", bn); }
	bool all = true;
	for (int i = 0; i < 32; i++) { char n[16]; r->syn.voiceNameIn(NB, i, n); std::string s(n); while (!s.empty() && s.back() == ' ') s.pop_back(); if (s != packedName(nasty, i)) { all = false; printf("    voice %d: '%s' vs file '%s'\n", i, s.c_str(), packedName(nasty, i).c_str()); } }
	CHECK(all, "all 32 voice names match the file bytes");
	CHECK(r->name(0) == packedName(nasty, 0) && r->name(7) == packedName(nasty, 7), "TG1 '%s' and TG8 '%s' booted from the bank", r->name(0).c_str(), r->name(7).c_str());
	dump(r->g, "01_voice_boot");

	printf("[2] BROWSE shows the nasty bank voices; turn and LOAD (c)\n");
	r->press(K_D);
	dump(r->g, "02_browse_enter");
	r->turn(0, 5);                             // Voice lane
	dump(r->g, "03_browse_voice6");
	CHECK(r->name(0) == packedName(nasty, 5), "audition on TG1 plays '%s' (expect '%s')", r->name(0).c_str(), packedName(nasty, 5).c_str());
	uint8_t want[kVoiceSize], got[kVoiceSize];
	r->syn.readVoiceIn(NB, 5, want); r->syn.readVoice(0, got);
	CHECK(!memcmp(want, got, kVoiceSize), "previewed voice bytes equal the bank voice");
	r->press(K_C);
	dump(r->g, "04_browse_loaded");
	CHECK(r->syn.get(P_PROGRAM, 0) == 5 && r->syn.get(P_BANK, 0) == NB, "LOAD sets TG1 bank/program to %d/5", NB);
	r->press(K_D); r->turn(0, 3); r->press(K_V);   // exit without load must put the old voice back
	CHECK(r->name(0) == packedName(nasty, 5), "leaving BROWSE with v undoes the preview ('%s')", r->name(0).c_str());

	printf("[3] Shift+d SAVE VOICE into bank 99\n");
	const std::string ub = sd + "/sysex/voice/000099_ZICUSER.syx";
	r->press(K_W); r->turn(0, 3);              // FM page, change algorithm so the voice differs from the stored one
	r->shiftPress(K_D);
	dump(r->g, "05_save_voice");
	std::vector<uint8_t> u;
	CHECK(readFile(ub, u) && u.size() == 4104, "000099_ZICUSER.syx written, %zu bytes", u.size());
	CHECK(u.size() == 4104 && u[0] == 0xF0 && u[1] == 0x43 && u[3] == 0x09 && u[4103] == 0xF7, "valid 32-voice bank header/footer");
	std::string saved = u.size() == 4104 ? packedName(u, 0) : "";
	CHECK(saved == r->name(0), "slot 1 holds '%s' (TG1 voice '%s')", saved.c_str(), r->name(0).c_str());
	r->shiftPress(K_D);
	readFile(ub, u);
	{ int nonblank = 0; for (int i = 0; i < 32; i++) if (u.size() == 4104 && packedName(u, i) != "INIT VOICE" && !packedName(u, i).empty()) nonblank++; CHECK(nonblank == 1, "same name replaces its slot (%d filled)", nonblank); }
	// different name -> second slot: init (Shift+q) renames the voice
	r->shiftPress(K_Q); r->shiftPress(K_D);
	readFile(ub, u);
	CHECK(u.size() == 4104 && packedName(u, 0) == "NOISE HAT" && packedName(u, 1) == "INIT VOICE", "different name takes the next slot, slot 1 intact (slots: '%s', '%s')", packedName(u, 0).c_str(), packedName(u, 1).c_str());

	printf("[4] restart: bank 99 appears in BROWSE and its voice loads\n");
	r.reset(new Rig(sd));
	CHECK(r->syn.bankCount() == 99 && r->syn.bankValid(98) && r->syn.bankValid(NB) && !r->syn.bankValid(50), "bankCount=%d, bank 98 valid", r->syn.bankCount());
	{ char bn[32]; r->syn.bankName(98, bn); CHECK(!strcmp(bn, "ZICUSER"), "bank name '%s'", bn); }
	r->press(K_D);
	r->turn(1, 1);                             // Bank lane: next existing bank (skips the gap)
	dump(r->g, "06_browse_bank99");
	{ char n[16]; r->syn.voiceNameIn(98, 0, n); CHECK(r->name(0) == std::string(n).substr(0, r->name(0).size()), "BROWSE jumped to bank 99, audition plays '%s'", r->name(0).c_str()); }
	r->turn(1, -1);
	{ char n[16]; r->syn.voiceNameIn(NB, 0, n); CHECK(r->name(0) == std::string(n).substr(0, r->name(0).size()), "bank lane back to the nasty bank plays '%s'", r->name(0).c_str()); }
	r->press(K_V);

	printf("[5] Shift+f SAVE PERFORMANCE\n");
	r->shiftPress(K_F);
	dump(r->g, "07_save_perf_no_folder");
	CHECK(r->syn.perfWrites == 0, "no /performance folder on the card: save is refused (the toast says NO PERFORMANCE FOLDER ON SD)");
	(void)!system("mkdir -p sdcard/performance");
	r->shiftPress(K_F);
	dump(r->g, "08_save_perf_with_folder");
	CHECK(r->syn.perfWrites == 1, "with a /performance folder on the card: saved");

	printf("[6] Shift+r FX\n");
	r->press(K_Q);
	r->shiftPress(K_R);
	dump(r->g, "09_fx");
	CHECK(r->ui.slot() == SLOT_MOD && r->ui.sub() == 2, "lands on MOD sub-page 2 (FX)");
	r->turn(0, 10); r->turn(2, 20); r->turn(3, 1);
	CHECK(r->syn.fx[FX_REVERB_SIZE] != 0 || r->syn.fx[FX_REVERB_LEVEL] != 0, "FX encoders move reverb/compressor");
	dump(r->g, "10_fx_turned");

	printf("[7] WiFi status line on the Shift screen\n");
	const char* states[3] = { "", "WiFi connecting", "WiFi 192.168.1.42" };
	r->syn.netEnabled = true;
	for (int s = 0; s < 3; s++) {
		r->syn.net = s == 2 ? states[2] : "";
		if (s == 1) r->syn.net.clear();
		char nb[32]; r->syn.netStatus(nb);
		r->ui.key(K_Z, true, r->ms); r->run(600);
		char nm[32]; snprintf(nm, sizeof nm, "11_shift_wifi_%d", s);
		dump(r->g, nm);
		CHECK(!strcmp(nb, s == 0 || s == 1 ? "WiFi connecting" : states[2]), "status text '%s'", nb);
		r->ui.key(K_Z, false, r->ms); r->run(200);
	}
	{   // the IP arrives while the Shift map is on screen: does the line update?
		r->syn.net.clear(); r->ui.key(K_Z, true, r->ms); r->run(600);
		Gfx before = r->g;
		r->syn.net = "WiFi 10.0.0.7"; r->run(1500);
		bool changed = false;
		for (int y = 0; y < 24 && !changed; y++) for (int x = 150; x < W; x++) if (before.pixel(x, y) != r->g.pixel(x, y)) { changed = true; break; }
		dump(r->g, "12_shift_wifi_late_ip");
		printf("  note %s\n", changed ? "an IP assigned while the Shift map is held shows up live" : "an IP assigned while the Shift map is held only shows after release and re-press (cosmetic: the map is a held peek)");
		r->ui.key(K_Z, false, r->ms); r->run(200);
	}
	r->syn.netEnabled = false; r->syn.net.clear();
	{ char nb[32]; r->syn.netStatus(nb); CHECK(nb[0] == 0, "network off: no status line"); }

	printf("\n%d checks passed, %d failed\n", g_pass, g_fail);
	return g_fail ? 1 : 0;
}
