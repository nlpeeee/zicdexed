#include "browse_model.h"

namespace zd {

void BrowseModel::begin(const Voice& current, int bank, int index, int targetTg)
{
	original_ = preview_ = current;
	bank_ = bank;
	index_ = index < 0 ? 0 : (index >= kBankVoices ? kBankVoices - 1 : index);
	setTarget(targetTg);
	active_ = true;
}

bool BrowseModel::load()
{
	Voice v;
	if (!src_ || !src_->readVoice(bank_, index_, v)) return false;
	preview_ = v;
	return true;
}

bool BrowseModel::moveVoice(int delta)
{
	if (!active_ || delta == 0) return false;
	index_ = ((index_ + delta) % kBankVoices + kBankVoices) % kBankVoices;
	return load();
}

bool BrowseModel::moveBank(int delta)
{
	if (!active_ || !src_ || delta == 0) return false;
	int n = src_->bankCount();
	int nb = bank_ + delta;
	if (nb < 0) nb = 0;
	if (nb > n - 1) nb = n - 1;
	if (nb == bank_) return false;
	bank_ = nb;
	return load();
}

int BrowseModel::firstVisible(int rows) const
{
	int first = index_ - rows / 2;
	if (first > kBankVoices - rows) first = kBankVoices - rows;
	return first < 0 ? 0 : first;
}

bool BrowseModel::rowName(int index, char out[kNameLen + 1]) const
{
	out[0] = 0;
	if (!src_ || index < 0 || index >= kBankVoices) return false;
	return src_->voiceName(bank_, index, out);
}

void BrowseModel::bankLabel(char out[kNameLen + 1]) const
{
	out[0] = 0;
	if (src_) src_->bankName(bank_, out);
}

} // namespace zd
