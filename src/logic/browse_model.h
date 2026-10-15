//
// browse_model.h - BROWSE page: pick a voice from a bank, hearing each one live.
//
// Turning the voice knob previews the voice on the target TG. LOAD (c) keeps it,
// CANCEL (v) puts back what was there when BROWSE was opened.
//
#pragma once
#include "dx7_voice.h"

namespace zd {

constexpr int kBankVoices = 32;

// Implemented by the framework on top of MiniDexed's bank loader.
class BankSource {
public:
	virtual ~BankSource() {}
	virtual int bankCount() = 0;
	virtual bool bankValid(int /*bank*/) { return true; }
	virtual void bankName(int bank, char out[kNameLen + 1]) = 0;
	virtual bool voiceName(int bank, int index, char out[kNameLen + 1]) = 0;
	virtual bool readVoice(int bank, int index, Voice& out) = 0;
};

class BrowseModel {
public:
	explicit BrowseModel(BankSource* src = nullptr) : src_(src) {}
	void setSource(BankSource* s) { src_ = s; }

	// Open the browser on `current` (the voice that is on the target TG now).
	void begin(const Voice& current, int bank, int index, int targetTg);
	bool active() const { return active_; }

	// Each returns true when a new voice should be previewed (see preview()).
	bool moveVoice(int delta);   // wraps within the bank
	bool moveBank(int delta);    // clamps, keeps the index
	void setTarget(int tg) { tg_ = tg < 0 ? 0 : (tg > 7 ? 7 : tg); }

	const Voice& preview() const { return preview_; }
	const Voice& original() const { return original_; }
	int bank() const { return bank_; }
	int index() const { return index_; }
	int target() const { return tg_; }

	// End the browse. commit() returns the previewed voice, cancel() the original.
	const Voice& commit() { active_ = false; return preview_; }
	const Voice& cancel() { active_ = false; return original_; }

	// List window: with `rows` visible rows, the first row's voice index so the
	// selection stays near the middle. Missing voices read as an empty name.
	int firstVisible(int rows) const;
	bool rowName(int index, char out[kNameLen + 1]) const;
	void bankLabel(char out[kNameLen + 1]) const;

private:
	bool load();
	BankSource* src_;
	Voice original_, preview_;
	int bank_ = 0, index_ = 0, tg_ = 0;
	bool active_ = false;
};

} // namespace zd
