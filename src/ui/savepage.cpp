//
// SAVE page (Shift+key 7): name the voice and choose where it goes in the user
// bank (bank 99, 32 slots). Not on a page key; OpenSaveScreen() opens it.
//   Encoders: 1 Char (turn the character under the cursor), 2 Cursor,
//             3 Slot (NEW = first free slot, or 1..32 to overwrite), 4 Set (A-Z, 0-9, a-z, symbols)
//   Keys:     1 cursor left, 2 cursor right, 3 SAVE, 4 CANCEL (key 12 also leaves)
// The picture is the ten name cells and what the save will do.
// When the user bank file cannot be read, the same slot shows a RESET screen
// instead: key 3 twice replaces the file (with its backup when that is good).
//
#include "zicpages.h"
#include "soundhub.h"
#include "name_editor.h"
#include <string.h>

namespace zic {

namespace {

const Color WARN = 0xFFB020;
zd::NameEditor g_ed;
bool g_reset = false;     // the bank file is unreadable: offer RESET instead of the naming screen
bool g_armed = false;     // key 3 pressed once on the RESET screen

const char* const kGroupName[5] = { "SPACE", "A-Z", "0-9", "a-z", "SYMBOLS" };

int groupOf(char ch)
{
	const char* cs = zd::NameEditor::charset();
	int i = 0;
	for (; i < zd::NameEditor::charsetSize(); i++) if (cs[i] == ch) break;
	if (i >= 63) return 4;
	if (i >= 37) return 3;
	if (i >= 27) return 2;
	if (i >= 1) return 1;
	return 0;
}

void laneOf(int i, LaneView& out)
{
	out.bipolar = false;
	if (g_reset) { out.label[0] = 0; out.text[0] = 0; out.frac = 0.f; return; }
	const char ch = g_ed.name()[g_ed.cursor()];
	switch (i) {
	case 0: {
		copyStr(out.label, "Char", sizeof out.label);
		if (ch == ' ') copyStr(out.text, "SPACE", sizeof out.text);
		else { out.text[0] = ch; out.text[1] = 0; }
		const char* cs = zd::NameEditor::charset();
		int k = 0;
		while (k < zd::NameEditor::charsetSize() && cs[k] != ch) k++;
		out.frac = frac(k, 0, zd::NameEditor::charsetSize() - 1);
		break;
	}
	case 1:
		copyStr(out.label, "Cursor", sizeof out.label);
		fmtInt(out.text, g_ed.cursor() + 1);
		strcat(out.text, "/10");
		out.frac = frac(g_ed.cursor(), 0, zd::kNameLen - 1);
		break;
	case 2:
		copyStr(out.label, "Slot", sizeof out.label);
		if (g_ed.target() < 0) copyStr(out.text, "NEW", sizeof out.text);
		else fmtInt(out.text, g_ed.target() + 1);
		out.frac = frac(g_ed.target() + 1, 0, zd::kBankSlots);
		break;
	default:
		copyStr(out.label, "Set", sizeof out.label);
		copyStr(out.text, kGroupName[groupOf(ch)], sizeof out.text);
		out.frac = frac(groupOf(ch), 0, 4);
		break;
	}
}

bool turnOf(int i, int steps)
{
	if (g_reset) return true;
	const int dir = steps > 0 ? 1 : -1;
	switch (i) {
	case 0: g_ed.turnChar(steps); break;
	case 1: g_ed.moveCursor(dir); break;
	case 2: g_ed.turnTarget(dir); break;
	default: g_ed.jumpGroup(dir); break;
	}
	return true;
}

void drawReset(Ctx& c)
{
	Gfx& g = c.g;
	g.text(10, 46, "USER BANK FILE UNREADABLE", fBody, WARN);
	g.text(10, 70, "BANK 99 CANNOT BE SAVED TO AS IT IS.", fSmall, WHITE);
	g.text(10, 86, "RESET RESTORES THE BACKUP IF IT IS GOOD,", fSmall, GREY);
	g.text(10, 100, "OTHERWISE STARTS AN EMPTY BANK.", fSmall, GREY);
	g.text(10, 120, g_armed ? "KEY 3 AGAIN TO RESET   KEY 4 CANCELS" : "KEY 3 TWICE = RESET   KEY 4 CANCELS", fSmall, g_armed ? WARN : GREY);
}

void drawOf(Ctx& c)
{
	if (g_reset) { drawReset(c); return; }
	Gfx& g = c.g;
	const int cw = 30, x0 = 10, y0 = 50, ch = 40;
	for (int i = 0; i < zd::kNameLen; i++) {
		const int x = x0 + i * cw;
		const bool cur = i == g_ed.cursor();
		g.roundRect((float)x + 1, (float)y0, (float)cw - 2, (float)ch, 4, cur ? dim(c.accent, 70) : PANEL,
		            cur ? c.accent : Gfx::NoColor, 1.5f);
		const char s[2] = { g_ed.name()[i], 0 };
		if (s[0] == ' ') g.hline(x + 9, y0 + ch - 9, cw - 18, cur ? c.accent : DIM);
		else g.textAligned(x, y0 + 4, cw, s, fBig, WHITE, AL_CENTER);
	}
	char line[40], b[8];
	Color col = c.accent;
	g.text(10, 98, "BANK 99  USER", fLbl, GREY, 1.f);
	const int slot = g_ed.slot();
	if (!g_ed.nameOk()) {
		copyStr(line, "GIVE IT A NAME FIRST", sizeof line);
		col = WARN;
	} else if (slot < 0) {
		copyStr(line, "BANK FULL: TURN ENCODER 3 TO PICK A SLOT", sizeof line);
		col = WARN;
	} else if (g_ed.overwrites()) {
		char old[zd::kNameLen + 1];
		copyStr(old, g_ed.slotName(slot), sizeof old);
		int n = (int)strlen(old);
		while (n > 0 && old[n - 1] == ' ') old[--n] = 0;
		fmtInt(b, slot + 1);
		copyStr(line, "OVERWRITES SLOT ", sizeof line);
		strcat(line, b);
		strcat(line, ": ");
		strcat(line, old);
		col = WARN;
	} else {
		fmtInt(b, slot + 1);
		copyStr(line, "NEW VOICE IN SLOT ", sizeof line);
		strcat(line, b);
		strcat(line, g_ed.target() < 0 ? "  (NEXT FREE)" : "");
	}
	g.text(10, 114, line, fBody, col);
	g.text(10, 132, "KEY 3 SAVES, KEY 4 CANCELS", fSmall, GREY);
}

// put the edited name into the TG's edit voice (the synth gets it too)
void applyName(Ctx& c)
{
	SoundHub& h = SoundHub::get();
	h.bind(c);
	zd::EditSession& s = h.session();
	for (int i = 0; i < zd::kNameLen; i++) s.setParam(zd::G_NAME + i, (uint8_t)g_ed.name()[i], false);
}

void leaveTo(Ctx& c)
{
	Slot back = c.ui.prevSlot();
	c.ui.gotoSlot(back == SLOT_SAVE ? SLOT_VOICE : back);
}

// the naming screen, with the user bank's slot names read fresh
bool beginNaming(Ctx& c, int preferSlot)
{
	char names[zd::kBankSlots][zd::kNameLen + 1];
	if (c.syn.userBankNames(names) <= 0) return false;
	SoundHub& h = SoundHub::get();
	h.bind(c);
	char nm[zd::kNameLen + 1];
	h.session().edit().name(nm);
	g_ed.begin(nm, names);
	if (preferSlot >= 0) g_ed.setTarget(preferSlot);
	c.ui.gotoSlot(SLOT_SAVE);
	return true;
}

class SavePage : public Page {
public:
	const char* name() const override { return "SAVE"; }

	const char* const* keyLabels(Ctx&, int) override
	{
		static const char* const l[4] = { "< CURSOR", "CURSOR >", "SAVE", "CANCEL" };
		static const char* const r[4] = { "", "", "RESET", "CANCEL" };
		return g_reset ? r : l;
	}

	void lane(Ctx&, int, int i, LaneView& out) override { laneOf(i, out); }
	bool turn(Ctx&, int, int i, int steps, bool) override { return turnOf(i, steps); }
	void draw(Ctx& c, int) override { drawOf(c); }

	bool key(Ctx& c, int, int k, bool down) override
	{
		if (!down) return false;
		if (g_reset) return resetKey(c, k);
		switch (k) {
		case K_Q: g_ed.moveCursor(-1); c.ui.redrawPicture(); return true;
		case K_W: g_ed.moveCursor(1); c.ui.redrawPicture(); return true;
		case K_E: save(c); return true;
		case K_R: c.ui.toast("SAVE CANCELLED"); leaveTo(c); return true;
		case K_A: case K_S: return true;               // TG-/TG+ would change which voice gets saved
		default: return false;
		}
	}

private:
	bool resetKey(Ctx& c, int k)
	{
		if (k != K_E) g_armed = false;
		switch (k) {
		case K_E:
			if (!g_armed) {
				g_armed = true;
				c.ui.toast("PRESS KEY 3 AGAIN TO RESET");
				c.ui.redrawPicture();
				return true;
			}
			g_armed = false;
			doReset(c);
			return true;
		case K_R: c.ui.toast("RESET CANCELLED"); leaveTo(c); return true;
		case K_Q: case K_W: case K_A: case K_S: c.ui.redrawPicture(); return true;
		default: c.ui.redrawPicture(); return false;
		}
	}

	void doReset(Ctx& c)
	{
		const int rc = c.syn.resetUserBank();
		if (rc < 0) { c.ui.toast("RESET FAILED - SD CARD"); return; }
		c.ui.toast(rc == 1 ? "BANK 99 RESTORED FROM BACKUP" : "BANK 99 RESET - EMPTY");
		g_reset = false;
		if (!beginNaming(c, -1)) leaveTo(c);
	}

	void save(Ctx& c)
	{
		if (!g_ed.nameOk()) { c.ui.toast("GIVE IT A NAME FIRST"); return; }
		const int slot = g_ed.slot();
		if (slot < 0) { c.ui.toast("USER BANK FULL - PICK A SLOT"); return; }
		applyName(c);
		SoundHub& h = SoundHub::get();
		zd::EditSession& s = h.session();
		const bool over = g_ed.overwrites();
		// the edit voice, not what is sounding: no op mutes in it
		const int rc = c.syn.saveVoiceTo(s.edit().d, slot);
		h.settle(c.tg);
		char msg[32], b[8];
		if (rc >= 0) {
			s.markSaved();
			fmtInt(b, rc + 1);
			copyStr(msg, over ? "OVERWROTE SLOT " : "SAVED IN SLOT ", sizeof msg);
			strcat(msg, b);
			strcat(msg, " OF BANK 99");
			c.ui.toast(msg);
			leaveTo(c);
			return;
		}
		c.ui.toast(rc == -2 ? "USER BANK FULL" : (rc == -1 ? "SAVE VOICE - NOT IN THIS BUILD" : "SAVE FAILED - SD CARD"));
	}
};

} // namespace

bool OpenSaveScreen(Ctx& c, int preferSlot)
{
	g_reset = false;
	g_armed = false;
	char probe[zd::kBankSlots][zd::kNameLen + 1];
	const int rc = c.syn.userBankNames(probe);
	if (rc == 0) {
		c.ui.toast("SAVE VOICE - NOT IN THIS BUILD");
		return false;
	}
	if (rc < 0) {                  // unreadable file: no dead end, offer the reset
		g_reset = true;
		c.ui.gotoSlot(SLOT_SAVE);
		return true;
	}
	if (!beginNaming(c, preferSlot)) return false;
	return true;
}

Page* CreateSavePage() { static SavePage p; return &p; }

} // namespace zic
