// The page table. Other pages (FM, OPS, MOD, BROWSE, KEYS) register here.
#include "zicpages.h"
#include "soundhub.h"

namespace zic {

__attribute__((weak)) bool SoundDesignShiftKey(Ctx&, int) { return false; }
__attribute__((weak)) bool SoundDesignDirty(Ctx&) { return false; }
__attribute__((weak)) bool SoundDesignCompare(Ctx&, int, bool) { return false; }

void RegisterPages(Ui& ui)
{
	static Page* voice = CreateVoicePage();
	static Page* mix = CreateMixPage();
	static Page* fm = CreateFmPage();
	static Page* ops = CreateOpsPage();
	static Page* mod = CreateModPage();
	static Page* browse = CreateBrowsePage();
	static Page* keys = CreateKeysPage();
	static Page* save = CreateSavePage();
	ui.addPage(SLOT_VOICE, voice);
	ui.addPage(SLOT_FM, fm);
	ui.addPage(SLOT_OPS, ops);
	ui.addPage(SLOT_MOD, mod);
	ui.addPage(SLOT_BROWSE, browse);
	ui.addPage(SLOT_KEYS, keys);
	ui.addPage(SLOT_MIX, mix);
	ui.addPage(SLOT_SAVE, save);
}

} // namespace zic
