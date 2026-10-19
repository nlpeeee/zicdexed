#include "name_editor.h"

namespace zd {

static const char kChars[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789abcdefghijklmnopqrstuvwxyz-.+*/!?#&:";
// first index of each group: space, A-Z, 0-9, a-z, symbols
static const int kGroup[] = { 0, 1, 27, 37, 63 };
static const int kGroups = 5;

const char* NameEditor::charset() { return kChars; }
int NameEditor::charsetSize() { return (int)sizeof kChars - 1; }

static int indexOf(char c)
{
	for (int i = 0; i < NameEditor::charsetSize(); i++) if (kChars[i] == c) return i;
	return 0;
}

void NameEditor::begin(const char name[kNameLen + 1], const char slotNames[kBankSlots][kNameLen + 1])
{
	bool end = false;                               // a short name ends at its first NUL
	for (int i = 0; i < kNameLen; i++) {
		char c = name[i];
		if (!c) end = true;
		name_[i] = (!end && c >= 32 && c < 127 && kChars[indexOf(c)] == c) ? c : ' ';
	}
	name_[kNameLen] = 0;
	for (int i = 0; i < kBankSlots; i++) {
		memcpy(names_[i], slotNames[i], kNameLen + 1);
		names_[i][kNameLen] = 0;
		free_names_[i] = memcmp(names_[i], "INIT VOICE", kNameLen) == 0;
	}
	cur_ = 0;
	manual_ = false;
	follow();
}

void NameEditor::follow()
{
	free_ = -1;
	for (int i = 0; i < kBankSlots; i++) if (free_names_[i]) { free_ = i; break; }
	if (manual_) return;
	target_ = -1;
	for (int i = 0; i < kBankSlots; i++)
		if (!free_names_[i] && memcmp(names_[i], name_, kNameLen) == 0) { target_ = i; break; }
}

void NameEditor::moveCursor(int d)
{
	cur_ += d;
	if (cur_ < 0) cur_ = 0;
	if (cur_ > kNameLen - 1) cur_ = kNameLen - 1;
}

void NameEditor::turnChar(int steps)
{
	int n = charsetSize();
	int i = ((indexOf(name_[cur_]) + steps) % n + n) % n;
	name_[cur_] = kChars[i];
	follow();
}

void NameEditor::jumpGroup(int d)
{
	int i = indexOf(name_[cur_]);
	int g = 0;
	for (int k = 0; k < kGroups; k++) if (i >= kGroup[k]) g = k;
	g = ((g + d) % kGroups + kGroups) % kGroups;
	name_[cur_] = kChars[kGroup[g]];
	follow();
}

void NameEditor::turnTarget(int steps)
{
	// -1 (NEW), 0..31
	int t = target_ + steps;
	if (t < -1) t = -1;
	if (t > kBankSlots - 1) t = kBankSlots - 1;
	target_ = t;
	manual_ = true;
}

bool NameEditor::blank() const
{
	for (int i = 0; i < kNameLen; i++) if (name_[i] != ' ') return false;
	return true;
}

bool NameEditor::nameOk() const
{
	return !blank() && memcmp(name_, "INIT VOICE", kNameLen) != 0;
}

void NameEditor::trimmed(char out[kNameLen + 1]) const
{
	memcpy(out, name_, kNameLen + 1);
	int n = kNameLen;
	while (n > 0 && out[n - 1] == ' ') n--;
	out[n] = 0;
}

} // namespace zd
