#include "zicglue.h"
#include "zicpages.h"
#include "zicmeter.h"
#include "minidexed.h"
#include <circle/timer.h>
#include <circle/logger.h>
#include <string.h>
#include <assert.h>

LOGMODULE ("zicui");

ZicMeter g_ZicMeter;

// ----------------------------------------------------------------- meters
void ZicMeterFeed (const float *pLevels, unsigned nStride, unsigned nTGs, unsigned nFrames)
{
	for (unsigned tg = 0; tg < nTGs && tg < 8; tg++)
	{
		const float *p = pLevels + tg * nStride;
		float fPeak = 0.0f;
		for (unsigned i = 0; i < nFrames; i++)
		{
			float v = p[i] < 0 ? -p[i] : p[i];
			if (v > fPeak) fPeak = v;
		}
		if (fPeak > g_ZicMeter.peak[tg]) g_ZicMeter.peak[tg] = fPeak;
	}

	unsigned nTap = (unsigned) g_ZicMeter.scopeTG;
	if (nTap < nTGs)
	{
		const float *p = pLevels + nTap * nStride;
		unsigned h = g_ZicMeter.head;
		for (unsigned i = 0; i < nFrames; i += 4)
		{
			float v = p[i] * 32767.0f;
			if (v > 32767.0f) v = 32767.0f;
			if (v < -32767.0f) v = -32767.0f;
			g_ZicMeter.ring[h++ % ZicMeter::Taps] = (int16_t) v;
		}
		g_ZicMeter.head = h;
	}
}

// ---------------------------------------------------------------- encoder
// Lookup indexed by (oldCLK << 3 | oldDT << 2 | newCLK << 1 | newDT), as in CKY040.
static const signed char s_Direction[16] = { 0, -1, 1, 0, 1, 0, 0, -1, -1, 0, 0, 1, 0, 1, -1, 0 };

CZicEncoder::CZicEncoder (unsigned nClockPin, unsigned nDataPin, CGPIOManager *pGPIOManager,
			  unsigned nDetents, bool bInvert)
:	m_Clock (nClockPin, GPIOModeInputPullUp, pGPIOManager),
	m_Data (nDataPin, GPIOModeInputPullUp, pGPIOManager),
	m_nDetents (nDetents),
	m_bInvert (bInvert),
	m_bConnected (false),
	m_nCounter (0),
	m_nLastDetentTicks (0),
	m_nSteps (0),
	m_nRaw (0)
{
	m_nLastClock = m_Clock.Read ();
	m_nLastData = m_Data.Read ();
}

CZicEncoder::~CZicEncoder (void)
{
	if (m_bConnected)
	{
		m_Clock.DisableInterrupt2 ();
		m_Clock.DisableInterrupt ();
		m_Clock.DisconnectInterrupt ();
		m_Data.DisableInterrupt2 ();
		m_Data.DisableInterrupt ();
		m_Data.DisconnectInterrupt ();
	}
}

bool CZicEncoder::Initialize (void)
{
	m_bConnected = true;
	m_Clock.ConnectInterrupt (InterruptStub, this);
	m_Data.ConnectInterrupt (InterruptStub, this);
	m_Clock.EnableInterrupt (GPIOInterruptOnFallingEdge);
	m_Clock.EnableInterrupt2 (GPIOInterruptOnRisingEdge);
	m_Data.EnableInterrupt (GPIOInterruptOnFallingEdge);
	m_Data.EnableInterrupt2 (GPIOInterruptOnRisingEdge);
	return true;
}

void CZicEncoder::InterruptStub (void *pParam)
{
	static_cast<CZicEncoder *> (pParam)->Interrupt ();
}

void CZicEncoder::Interrupt (void)
{
	unsigned nClock = m_Clock.Read ();
	unsigned nData = m_Data.Read ();
	int nDir = s_Direction[(m_nLastClock << 3) | (m_nLastData << 2) | (nClock << 1) | nData];
	m_nLastClock = nClock;
	m_nLastData = nData;
	if (nDir == 0) return;

	m_nCounter += nDir;
	if (m_nCounter > -(int) m_nDetents && m_nCounter < (int) m_nDetents) return;
	nDir = m_nCounter > 0 ? 1 : -1;
	m_nCounter = 0;
	if (m_bInvert) nDir = -nDir;

	// Turn speed -> step size: slow turns are exact, fast turns race through 0..99 ranges.
	unsigned nNow = CTimer::GetClockTicks ();
	unsigned nDelta = nNow - m_nLastDetentTicks;
	m_nLastDetentTicks = nNow;
	int nMult = nDelta < 12000 ? 6 : (nDelta < 25000 ? 3 : (nDelta < 60000 ? 2 : 1));

	__atomic_fetch_add (&m_nSteps, nDir * nMult, __ATOMIC_RELAXED);
	__atomic_fetch_add (&m_nRaw, nDir, __ATOMIC_RELAXED);
}

void CZicEncoder::Take (int &nSteps, int &nRaw)
{
	nSteps = __atomic_exchange_n (&m_nSteps, 0, __ATOMIC_ACQ_REL);
	nRaw = __atomic_exchange_n (&m_nRaw, 0, __ATOMIC_ACQ_REL);
}

// ----------------------------------------------------------------- the UI
// ZicPixel wiring (GPIO numbers): keys q w e r / a s d f / z x c v, active low.
static const unsigned s_KeyPin[zic::kKeyCount] = { 20, 16, 25, 14, 12, 1, 24, 15, 7, 8, 23, 22 };
// encoders 1..4: clock, data
static const unsigned s_EncPin[4][2] = { { 26, 13 }, { 6, 5 }, { 0, 9 }, { 27, 4 } };

static const unsigned DebounceMs = 6;
static const int ChunkPixels = 2400;		// per Process() call: about 3 ms of SPI

CZicUI::CZicUI (CMiniDexed *pMiniDexed, CGPIOManager *pGPIOManager, CConfig *pConfig,
		CST7789Display *pDisplay, IZicMenuHost *pMenuHost)
:	m_pMiniDexed (pMiniDexed),
	m_pGPIOManager (pGPIOManager),
	m_pConfig (pConfig),
	m_pDisplay (pDisplay),
	m_pMenuHost (pMenuHost),
	m_pGfx (0),
	m_pSynth (0),
	m_pUi (0),
	m_bMenu (false),
	m_bMenuShift (false),
	m_pChunk (0)
{
	for (unsigned i = 0; i < zic::kKeyCount; i++)
	{
		m_pKey[i] = 0;
		m_bKeyRaw[i] = m_bKeyState[i] = false;
		m_nKeyChangeMs[i] = 0;
	}
	for (unsigned i = 0; i < 4; i++) m_pEncoder[i] = 0;
}

CZicUI::~CZicUI (void)
{
	for (unsigned i = 0; i < zic::kKeyCount; i++) delete m_pKey[i];
	for (unsigned i = 0; i < 4; i++) delete m_pEncoder[i];
	delete m_pUi;
	delete m_pSynth;
	delete m_pGfx;
	delete [] m_pChunk;
}

unsigned CZicUI::Now (void)
{
	return (unsigned) (CTimer::GetClockTicks64 () / 1000);
}

bool CZicUI::Initialize (void)
{
	m_pGfx = new zic::Gfx;
	m_pChunk = new uint16_t[ChunkPixels + zic::W];
	m_pSynth = ZicCreateSynth (m_pMiniDexed, m_pConfig);
	m_pUi = new zic::Ui (*m_pGfx, *m_pSynth);
	if (!m_pGfx || !m_pChunk || !m_pSynth || !m_pUi) return false;

	for (unsigned i = 0; i < zic::kKeyCount; i++)
	{
		m_pKey[i] = new CGPIOPin (s_KeyPin[i], GPIOModeInputPullUp);
		m_bKeyRaw[i] = m_bKeyState[i] = (m_pKey[i]->Read () == LOW);
	}

	unsigned nInvert = m_pConfig->GetZicEncoderInvert ();
	for (unsigned i = 0; i < 4; i++)
	{
		m_pEncoder[i] = new CZicEncoder (s_EncPin[i][0], s_EncPin[i][1], m_pGPIOManager,
						 m_pConfig->GetEncoderDetents (), (nInvert >> i) & 1);
		if (!m_pEncoder[i]->Initialize ()) return false;
	}

	zic::RegisterPages (*m_pUi);
	m_pUi->start (Now ());
	LOGDBG ("ZicDexed UI started");
	return true;
}

void CZicUI::Process (void)
{
	unsigned nMs = Now ();

	// keys: accept a change once it has been stable for a few ms
	for (unsigned i = 0; i < zic::kKeyCount; i++)
	{
		bool bRaw = m_pKey[i]->Read () == LOW;
		if (bRaw != m_bKeyRaw[i])
		{
			m_bKeyRaw[i] = bRaw;
			m_nKeyChangeMs[i] = nMs;
		}
		else if (bRaw != m_bKeyState[i] && nMs - m_nKeyChangeMs[i] >= DebounceMs)
		{
			m_bKeyState[i] = bRaw;
			OnKey (i, bRaw, nMs);
		}
	}

	for (unsigned i = 0; i < 4; i++)
	{
		int nSteps, nRaw;
		m_pEncoder[i]->Take (nSteps, nRaw);
		if (!nRaw) continue;
		if (m_bMenu)
		{
			for (int n = nRaw < 0 ? -nRaw : nRaw; n > 0; n--)
				m_pMenuHost->ZicMenuEvent (nRaw > 0 ? ZicMenuUp : ZicMenuDown);
		}
		else
		{
			m_pUi->encoder (i, nSteps, nRaw, nMs);
		}
	}

	if (m_bMenu) return;

	m_pUi->tick (nMs);
	if (m_pUi->menuRequested ())
	{
		m_bMenu = true;
		m_bMenuShift = false;
		m_pGfx->clearDirty ();
		m_pMenuHost->ZicMenuOpen ();
		return;
	}
	Flush ();
}

void CZicUI::Flush (void)
{
	zic::Rect r;
	if (!m_pGfx->takeChunk (r, m_pChunk, ChunkPixels)) return;
	CDisplay::TArea Area;
	Area.x1 = r.x;
	Area.x2 = r.x + r.w - 1;
	Area.y1 = r.y;
	Area.y2 = r.y + r.h - 1;
	m_pDisplay->SetArea (Area, m_pChunk);
}

void CZicUI::OnKey (unsigned nKey, bool bDown, unsigned nMs)
{
	if (m_bMenu)
	{
		OnMenuKey (nKey, bDown);
		if (!m_bMenu) m_pUi->resume (nMs);
		return;
	}
	m_pUi->key (nKey, bDown, nMs);
}

// In the stock menu: v Select, c Back, d Home, a/s Program -/+, q/w TG -/+,
// e/r Bank -/+, and Shift+v (z held) goes back to the ZicDexed screens.
void CZicUI::OnMenuKey (unsigned nKey, bool bDown)
{
	if (nKey == zic::K_Z) { m_bMenuShift = bDown; return; }
	if (!bDown) return;
	switch (nKey)
	{
	case zic::K_V:
		if (m_bMenuShift) m_bMenu = false;
		else m_pMenuHost->ZicMenuEvent (ZicMenuSelect);
		break;
	case zic::K_C: m_pMenuHost->ZicMenuEvent (ZicMenuBack); break;
	case zic::K_D: m_pMenuHost->ZicMenuEvent (ZicMenuHome); break;
	case zic::K_A: m_pMenuHost->ZicMenuEvent (ZicMenuPgmDown); break;
	case zic::K_S: m_pMenuHost->ZicMenuEvent (ZicMenuPgmUp); break;
	case zic::K_Q: m_pMenuHost->ZicMenuEvent (ZicMenuTGDown); break;
	case zic::K_W: m_pMenuHost->ZicMenuEvent (ZicMenuTGUp); break;
	case zic::K_E: m_pMenuHost->ZicMenuEvent (ZicMenuBankDown); break;
	case zic::K_R: m_pMenuHost->ZicMenuEvent (ZicMenuBankUp); break;
	default: break;
	}
}
