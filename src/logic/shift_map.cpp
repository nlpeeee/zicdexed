#include "shift_map.h"

namespace zd {

const KeyDef kKeyMap[kKeyDefCount] = {
	{ 'q', "VOICE",   "INIT",     SA_INIT },
	{ 'w', "FM",      "MUTATE",   SA_MUTATE },
	{ 'e', "OPS",     "OP MUTE",  SA_OP_MUTE },
	{ 'r', "MOD",     "FX",       SA_FX },
	{ 'a', "< TG",    "TG MUTE",  SA_TG_MUTE },
	{ 's', "TG >",    "TG SOLO",  SA_TG_SOLO },
	{ 'd', "BROWSE",  "SAVE VCE", SA_SAVE_VOICE },
	{ 'f', "MIX",     "SAVE PRF", SA_SAVE_PERF },
	{ 'z', "SHIFT",   "SHIFT",    SA_NONE },
	{ 'x', "PLAY",    "KEYS",     SA_KEYS },
	{ 'c', "COMPARE", "UNDO",     SA_REVERT },
	{ 'v', "EXIT",    "MENU",     SA_MENU },
};

const KeyDef* FindKey(char key)
{
	for (int i = 0; i < kKeyDefCount; i++) if (kKeyMap[i].key == key) return &kKeyMap[i];
	return nullptr;
}

} // namespace zd
