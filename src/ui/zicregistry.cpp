// The page table. Other pages (FM, OPS, MOD, BROWSE, KEYS) register here.
#include "zicpages.h"

namespace zic {

__attribute__((weak)) bool SoundDesignShiftKey(Ctx&, int) { return false; }

void RegisterPages(Ui& ui)
{
	static Page* voice = CreateVoicePage();
	static Page* mix = CreateMixPage();
	ui.addPage(SLOT_VOICE, voice);
	ui.addPage(SLOT_MIX, mix);
}

} // namespace zic
