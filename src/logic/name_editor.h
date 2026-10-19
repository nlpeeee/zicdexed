//
// name_editor.h - editing a 10-character DX7 voice name with encoders and keys,
// and choosing where the voice goes in the user bank. Pure logic; the SAVE page
// in src/ui/savepage.cpp draws it.
//
#pragma once
#include "dx7_voice.h"
#include "voice_pack.h"

namespace zd {

class NameEditor {
public:
	// Characters the encoder steps through: space, A-Z, 0-9, a-z, punctuation.
	static const char* charset();
	static int charsetSize();

	// `slotNames`: the 32 names currently in the user bank ("INIT VOICE" = free).
	void begin(const char name[kNameLen + 1], const char slotNames[kBankSlots][kNameLen + 1]);

	const char* name() const { return name_; }      // always 10 chars, space padded
	int cursor() const { return cur_; }
	void moveCursor(int d);                         // clamps to 0..9
	void turnChar(int steps);                       // cycle the character under the cursor
	void jumpGroup(int d);                          // A-Z, 0-9, a-z, punctuation, space

	// Target slot: -1 = the first free slot. It follows the name (same name as a
	// slot = overwrite it) until the user picks one with turnTarget().
	int target() const { return target_; }
	void turnTarget(int steps);                     // NEW, 1..32
	int freeSlot() const { return free_; }          // first free slot or -1 when full
	bool overwrites() const { return slot() >= 0 && !SlotFree(slot()); }
	// Slot the save will really use, or -1 when the bank is full.
	int slot() const { return target_ >= 0 ? target_ : free_; }
	const char* slotName(int s) const { return names_[s]; }
	bool SlotFree(int s) const { return free_names_[s]; }

	bool blank() const;                             // only spaces
	// A name that cannot be saved to the bank: blank, or the free-slot marker.
	bool nameOk() const;
	// Name with trailing spaces removed, for display.
	void trimmed(char out[kNameLen + 1]) const;

private:
	void follow();
	char name_[kNameLen + 1];
	char names_[kBankSlots][kNameLen + 1];
	bool free_names_[kBankSlots];
	int cur_ = 0, target_ = -1, free_ = -1;
	bool manual_ = false;
};

} // namespace zd
