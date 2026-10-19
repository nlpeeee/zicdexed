//
// zicpages.h - the pages of this build, and small helpers shared by pages.
//
// To add a page: implement zic::Page, give it a factory like the ones below,
// and add one line to RegisterPages() in zicregistry.cpp.
//
#pragma once
#include "zicui.h"

namespace zic {

Page* CreateVoicePage();   // q: cutoff/reso/detune/volume, play, mix
Page* CreateMixPage();     // f: the eight channel strips

// Shift+q/w/e (INIT, MUTATE, OP MUTE), from any page. Returns false if not handled.
// A weak default in zicregistry.cpp is replaced by the sound-design pages.
bool SoundDesignShiftKey(Ctx& c, int key);

// Shift+11 (UNDO, then REVERT) is handled by SoundDesignShiftKey too.
// True while the TG's edit voice differs from the stored one (drives the dirty dot).
bool SoundDesignDirty(Ctx& c);
// COMPARE (key 10, hold): the voice to hear is the session's stored voice. Returns false when
// not handled, then the framework falls back to ISynth::compareBegin/End.
bool SoundDesignCompare(Ctx& c, int tg, bool begin);

// Shift+7: opens the naming screen (SLOT_SAVE) for the current TG's edit voice.
// Toasts and returns false in a build without SAVE VOICE. An unreadable bank file
// opens the screen in RESET mode (key 3 twice).
bool OpenSaveScreen(Ctx& c, int preferSlot = -1);   // preferSlot: user-bank slot to overwrite by default

// Called once at start-up: puts every page of this build into its slot.
void RegisterPages(Ui& ui);

// ---- helpers for lane tables
void fmtInt(char* out, int v, bool plus = false);
void fmtPan(char* out, int pan);          // C, L12, R40
void fmtChannel(char* out, int ch);       // 1..16, OMNI, OFF
// Fast encoder turns are multiplied up for 0..99 ranges; on a volume lane that
// is a jump of up to 6 dB per click, so cap it.
inline int volumeSteps(int steps) { return steps > 4 ? 4 : (steps < -4 ? -4 : steps); }
inline float frac(int v, int lo, int hi) { return hi > lo ? (float)(v - lo) / (float)(hi - lo) : 0.f; }
void copyStr(char* dst, const char* src, int cap);
void trimName(char* dst, const char* src);   // voice name without trailing spaces

} // namespace zic
