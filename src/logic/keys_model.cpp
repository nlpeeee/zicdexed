#include "keys_model.h"

namespace zd {

struct ScaleDef { const char* name; uint8_t n; uint8_t steps[12]; };
static const ScaleDef kScales[] = {
	{ "CHROM",  12, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 } },
	{ "MAJOR",   7, { 0, 2, 4, 5, 7, 9, 11 } },
	{ "MINOR",   7, { 0, 2, 3, 5, 7, 8, 10 } },
	{ "DORIAN",  7, { 0, 2, 3, 5, 7, 9, 10 } },
	{ "PHRYG",   7, { 0, 1, 3, 5, 7, 8, 10 } },
	{ "LYDIAN",  7, { 0, 2, 4, 6, 7, 9, 11 } },
	{ "MIXO",    7, { 0, 2, 4, 5, 7, 9, 10 } },
	{ "HARM MIN", 7, { 0, 2, 3, 5, 7, 8, 11 } },
	{ "PENT MAJ", 5, { 0, 2, 4, 7, 9 } },
	{ "PENT MIN", 5, { 0, 3, 5, 7, 10 } },
	{ "BLUES",   6, { 0, 3, 5, 6, 7, 10 } },
	{ "WHOLE",   6, { 0, 2, 4, 6, 8, 10 } },
};

int KeysModel::ScaleCount() { return (int)(sizeof kScales / sizeof kScales[0]); }

const char* KeysModel::ScaleName(int i)
{
	if (i < 0 || i >= ScaleCount()) i = 0;
	return kScales[i].name;
}

const char* KeysModel::NoteName(int pc)
{
	static const char* const n[12] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
	return n[((pc % 12) + 12) % 12];
}

int KeysModel::KeyIndex(char c)
{
	static const char order[] = "qwerasdf";
	for (int i = 0; i < kKeyCount; i++) if (order[i] == c) return i;
	return -1;
}

int KeysModel::noteForKey(int key) const
{
	if (key < 0 || key >= kKeyCount) return -1;
	const ScaleDef& s = kScales[scale];
	int note = (octave + 2) * 12 + root + (key / s.n) * 12 + s.steps[key % s.n];
	return note > 127 ? 127 : note;
}

void KeysModel::keyDown(int key, VoiceSink* sink)
{
	if (key < 0 || key >= kKeyCount || sounding_[key] >= 0) return;
	sounding_[key] = noteForKey(key);
	if (sink) sink->noteOn(sounding_[key], velocity);
}

void KeysModel::keyUp(int key, VoiceSink* sink)
{
	if (key < 0 || key >= kKeyCount || sounding_[key] < 0) return;
	if (sink) sink->noteOff(sounding_[key]);
	sounding_[key] = -1;
}

void KeysModel::allOff(VoiceSink* sink)
{
	for (int k = 0; k < kKeyCount; k++) keyUp(k, sink);
}

} // namespace zd
