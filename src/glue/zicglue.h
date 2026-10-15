//
// zicglue.h - connects the ZicDexed screen framework (src/ui) to the hardware
// and to MiniDexed: 12 keys, 4 encoders, the ST7789, and the synth.
//
#ifndef _zicglue_h
#define _zicglue_h

#include "zicui.h"
#include "zicsynth.h"
#include "config.h"
#include <circle/gpiomanager.h>
#include <circle/gpiopin.h>
#include <display/st7789display.h>

class CMiniDexed;

// The stock menu (Shift+v) is driven through this.
enum TZicMenuEvent
{
	ZicMenuUp, ZicMenuDown, ZicMenuBack, ZicMenuSelect, ZicMenuHome,
	ZicMenuPgmUp, ZicMenuPgmDown, ZicMenuBankUp, ZicMenuBankDown, ZicMenuTGUp, ZicMenuTGDown
};

class IZicMenuHost
{
public:
	virtual ~IZicMenuHost (void) {}
	virtual void ZicMenuOpen (void) = 0;			// clear the screen and show the menu
	virtual void ZicMenuEvent (TZicMenuEvent Event) = 0;
};

// Quadrature encoder, interrupt driven, with turn-speed acceleration.
class CZicEncoder
{
public:
	CZicEncoder (unsigned nClockPin, unsigned nDataPin, CGPIOManager *pGPIOManager, unsigned nDetents, bool bInvert);
	~CZicEncoder (void);
	bool Initialize (void);
	// Detents turned since the last call: steps is accelerated, raw is not.
	void Take (int &nSteps, int &nRaw);

private:
	static void InterruptStub (void *pParam);
	void Interrupt (void);

	CGPIOPin m_Clock;
	CGPIOPin m_Data;
	unsigned m_nDetents;
	bool m_bInvert;
	bool m_bConnected;
	unsigned m_nLastClock, m_nLastData;
	int m_nCounter;
	unsigned m_nLastDetentTicks;
	volatile int m_nSteps, m_nRaw;
};

class CZicUI
{
public:
	CZicUI (CMiniDexed *pMiniDexed, CGPIOManager *pGPIOManager, CConfig *pConfig,
		CST7789Display *pDisplay, IZicMenuHost *pMenuHost);
	~CZicUI (void);

	bool Initialize (void);
	void Process (void);			// from the main loop; cheap when idle

	bool InMenu (void) const { return m_bMenu; }

private:
	void OnKey (unsigned nKey, bool bDown, unsigned nMs);
	void OnMenuKey (unsigned nKey, bool bDown);
	void Flush (void);
	static unsigned Now (void);

private:
	CMiniDexed *m_pMiniDexed;
	CGPIOManager *m_pGPIOManager;
	CConfig *m_pConfig;
	CST7789Display *m_pDisplay;
	IZicMenuHost *m_pMenuHost;

	zic::Gfx *m_pGfx;
	zic::ISynth *m_pSynth;
	zic::Ui *m_pUi;

	CGPIOPin *m_pKey[zic::kKeyCount];
	bool m_bKeyRaw[zic::kKeyCount];
	bool m_bKeyState[zic::kKeyCount];
	unsigned m_nKeyChangeMs[zic::kKeyCount];

	CZicEncoder *m_pEncoder[4];
	bool m_bMenu;
	bool m_bMenuShift;
	uint16_t *m_pChunk;
};

// The synth adaptor (zicsynth_dexed.cpp)
zic::ISynth *ZicCreateSynth (CMiniDexed *pMiniDexed, CConfig *pConfig);

#endif
