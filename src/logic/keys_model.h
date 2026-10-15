//
// keys_model.h - KEYS page: the 8 upper keys (q w e r / a s d f) become a playable
// scale, for sounding a voice with no keyboard attached.
//
#pragma once
#include "edit_session.h" // VoiceSink

namespace zd {

constexpr int kKeyCount = 8;

class KeysModel {
public:
	int octave = 3;    // 0..6, C3 = MIDI 60 (Yamaha numbering, like the DX7)
	int scale = 2;     // index into the scale table, 2 = Minor
	int root = 0;      // 0..11, C..B
	int velocity = 100; // 1..127

	static int ScaleCount();
	static const char* ScaleName(int i);
	static const char* NoteName(int pitchClass); // "C", "Db", ...
	// Physical key letter -> 0..7, or -1.
	static int KeyIndex(char c);

	void setOctave(int v) { octave = Clamp(v, 0, 6); }
	void setScale(int v) { scale = Clamp(v, 0, ScaleCount() - 1); }
	void setRoot(int v) { root = Clamp(v, 0, 11); }
	void setVelocity(int v) { velocity = Clamp(v, 1, 127); }

	// MIDI note for key 0..7 with the current settings (clamped to 0..127).
	int noteForKey(int key) const;
	// Pitch class name on the key cap ("Eb"), and whether the key is the root.
	const char* keyLabel(int key) const { return NoteName(noteForKey(key) % 12); }
	bool isRoot(int key) const { return noteForKey(key) % 12 == root; }

	// Key events. A note keeps sounding until its key is released, even if
	// octave/scale/root change in the meantime.
	void keyDown(int key, VoiceSink* sink);
	void keyUp(int key, VoiceSink* sink);
	void allOff(VoiceSink* sink);
	bool held(int key) const { return key >= 0 && key < kKeyCount && sounding_[key] >= 0; }

private:
	static int Clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }
	int sounding_[kKeyCount] = { -1, -1, -1, -1, -1, -1, -1, -1 };
};

} // namespace zd
