//
// shift_map.h - key map shown on the SHIFT screen (mockup 09) and the action
// each key triggers while Shift is held. Row-major: q w e r / a s d f / z x c v.
//
#pragma once

namespace zd {

enum ShiftAction {
	SA_NONE,
	SA_INIT, SA_MUTATE, SA_OP_MUTE, SA_FX,
	SA_TG_MUTE, SA_TG_SOLO, SA_SAVE_VOICE, SA_SAVE_PERF,
	SA_KEYS, SA_REVERT, SA_MENU
};

struct KeyDef {
	char key;
	const char* normal;      // label without Shift
	const char* shifted;     // label with Shift held
	ShiftAction action;
};

constexpr int kKeyDefCount = 12;
extern const KeyDef kKeyMap[kKeyDefCount];

// nullptr if the key is not one of the 12.
const KeyDef* FindKey(char key);

} // namespace zd
