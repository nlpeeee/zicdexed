//
// SAVE page (Shift+key 7): name the voice and choose where it goes in the user
// bank (bank 99, 32 slots). Not on a page key; OpenSaveScreen() opens it.
//   Encoders: 1 Char (turn the character under the cursor), 2 Cursor,
//             3 Slot (NEW = first free slot, or 1..32 to overwrite), 4 Set (A-Z, 0-9, a-z, symbols)
//   Keys:     1 cursor left, 2 cursor right, 3 SAVE, 4 CANCEL (key 12 also leaves)
// The picture is the ten name cells and what the save will do.
//
// The same editor is the VOICE page's RENAME sub-page (the Rename* functions at
// the bottom): it only renames the voice on the current TG, nothing goes to a bank.
//
#include "zicpages.h"
#include "soundhub.h"
#include "name_editor.h"
#include <string.h>

namespace zic {

namespace {

const Color WARN = 0xFFB020;
zd::NameEditor g_ed;

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

void laneOf(Ctx& c, bool rename, int i, LaneView& out)
{
	out.bipolar = false;
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
		if (rename) {                                   // nothing to choose: the voice stays on its TG
			copyStr(out.label, "TG", sizeof out.label);
			fmtInt(out.text, c.tg + 1);
			out.frac = 0;
			break;
		}
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

bool turnOf(bool rename, int i, int steps)
{
	const int dir = steps > 0 ? 1 : -1;
	switch (i) {
	case 0: g_ed.turnChar(steps); break;
	case 1: g_ed.moveCursor(dir); break;
	case 2: if (!rename) g_ed.turnTarget(dir); break;
	default: g_ed.jumpGroup(dir); break;
	}
	return true;
}

void drawOf(Ctx& c, bool rename)
{
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
	if (rename) {
		g.text(10, 98, "RENAME VOICE", fLbl, GREY, 1.f);
		if (g_ed.blank()) { copyStr(line, "GIVE IT A NAME FIRST", sizeof line); col = WARN; }
		else { copyStr(line, "RENAMES THE VOICE ON TG ", sizeof line); fmtInt(b, c.tg + 1); strcat(line, b); }
		g.text(10, 114, line, fBody, col);
		g.text(10, 132, "NOT SAVED TO A BANK: SHIFT + KEY 7 DOES THAT", fSmall, GREY);
		return;
	}
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

class SavePage : public Page {
public:
	const char* name() const override { return "SAVE"; }

	const char* const* keyLabels(Ctx&, int) override
	{
		static const char* const l[4] = { "< CURSOR", "CURSOR >", "SAVE", "CANCEL" };
		return l;
	}

	void lane(Ctx& c, int, int i, LaneView& out) override { laneOf(c, false, i, out); }
	bool turn(Ctx&, int, int i, int steps, bool) override { return turnOf(false, i, steps); }
	void draw(Ctx& c, int) override { drawOf(c, false); }

	bool key(Ctx& c, int, int k, bool down) override
	{
		if (!down) return false;
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
	char names[zd::kBankSlots][zd::kNameLen + 1];
	const int rc = c.syn.userBankNames(names);
	if (rc <= 0) {
		c.ui.toast(rc == 0 ? "SAVE VOICE - NOT IN THIS BUILD" : "USER BANK FILE UNREADABLE");
		return false;
	}
	SoundHub& h = SoundHub::get();
	h.bind(c);
	char nm[zd::kNameLen + 1];
	h.session().edit().name(nm);
	g_ed.begin(nm, names);
	if (preferSlot >= 0) g_ed.setTarget(preferSlot);
	c.ui.gotoSlot(SLOT_SAVE);
	return true;
}

// ---- RENAME (VOICE page, last sub-page) -------------------------------------------

void RenameEnter(Ctx& c)
{
	char names[zd::kBankSlots][zd::kNameLen + 1];
	for (int i = 0; i < zd::kBankSlots; i++) strcpy(names[i], "INIT VOICE");
	SoundHub& h = SoundHub::get();
	h.bind(c);
	char nm[zd::kNameLen + 1];
	h.session().edit().name(nm);
	g_ed.begin(nm, names);
}

void RenameLane(Ctx& c, int i, LaneView& out) { laneOf(c, true, i, out); }
bool RenameTurn(Ctx&, int i, int steps) { return turnOf(true, i, steps); }
void RenameDraw(Ctx& c) { drawOf(c, true); }

const char* const* RenameLabels()
{
	static const char* const l[4] = { "< CURSOR", "CURSOR >", "RENAME", "CANCEL" };
	return l;
}

bool RenameKey(Ctx& c, int k)
{
	switch (k) {
	case K_Q: g_ed.moveCursor(-1); c.ui.redrawPicture(); return true;
	case K_W: g_ed.moveCursor(1); c.ui.redrawPicture(); return true;
	case K_E:
		if (g_ed.blank()) { c.ui.toast("GIVE IT A NAME FIRST"); return true; }
		applyName(c);
		SoundHub::get().settle(c.tg);
		c.ui.toast("VOICE RENAMED");
		c.ui.gotoSub(SLOT_VOICE, 0);
		return true;
	case K_R:
		c.ui.toast("RENAME CANCELLED");
		c.ui.gotoSub(SLOT_VOICE, 0);
		return true;
	case K_A: case K_S: return true;                   // the name belongs to this TG's voice
	default: return false;
	}
}

Page* CreateSavePage() { static SavePage p; return &p; }

} // namespace zic
