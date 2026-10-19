//
// ISynth on top of MiniDexed. All calls come from the UI loop, the same context
// the stock buttons use.
//
#include "zicglue.h"
#include "zicmeter.h"
#include "zicsave.h"
#include "voice_pack.h"
#include "minidexed.h"
#include <string.h>

using namespace zic;

namespace {

const int kTransposeOffset = 144;	// DX7 voice parameter, 0..48 with 24 = C3

class CZicSynth : public ISynth
{
public:
	CZicSynth (CMiniDexed *pMiniDexed, CConfig *pConfig)
	:	m_pMini (pMiniDexed), m_pConfig (pConfig)
	{
		memset (m_Saved, 0, sizeof m_Saved);
		memset (m_bComparing, 0, sizeof m_bComparing);
		memset (&g_ZicMeter, 0, sizeof g_ZicMeter);
	}

	int tgCount () override
	{
		unsigned n = m_pConfig->GetToneGenerators ();
		return n > (unsigned) kMaxTG ? kMaxTG : (n ? (int) n : 1);
	}

	int get (TGParam p, int tg) override
	{
		switch (p)
		{
		case P_TRANSPOSE: return (int) m_pMini->GetVoiceParameter (kTransposeOffset, CMiniDexed::NoOP, tg) - 24;
		default: return m_pMini->GetTGParameter (Map (p), tg);
		}
	}

	void set (TGParam p, int tg, int v) override
	{
		const ParamRange &r = paramRange (p);
		if (v < r.min) v = r.min;
		if (v > r.max) v = r.max;
		if (p == P_TRANSPOSE)
			m_pMini->SetVoiceParameter (kTransposeOffset, (uint8_t) (v + 24), CMiniDexed::NoOP, tg);
		else
			m_pMini->SetTGParameter (Map (p), v, tg);
		if (p == P_BANK || p == P_PROGRAM)
		{
			// MiniDexed just loaded this program from its boot-time banks; for the
			// user bank that is stale after a save, so load the saved voice instead.
			unsigned bank = m_pMini->GetTGParameter (CMiniDexed::TGParameterVoiceBank, tg);
			unsigned prog = m_pMini->GetTGParameter (CMiniDexed::TGParameterProgram, tg);
			uint8_t buf[kVoiceSize];
			if (UserCopy ((int) bank) && readVoiceIn ((int) bank, (int) prog, buf)) loadVoice (tg, buf);
		}
	}

	int getFx (FxParam p) override { return m_pMini->GetParameter (MapFx (p)); }
	void setFx (FxParam p, int v) override { m_pMini->SetParameter (MapFx (p), v); }

	void voiceName (int tg, char out[kNameLen + 1]) override
	{
		std::string s = m_pMini->GetVoiceName (tg);
		CopyName (out, s);
	}

	void bankName (int bank, char out[32]) override
	{
		std::string s = m_pMini->GetSysExFileLoader ()->GetBankName (bank);
		if (s.empty () && UserCopy (bank)) s = "ZICUSER";
		unsigned n = 0;
		for (; n < s.size () && n < 31; n++) out[n] = s[n];
		out[n] = 0;
	}

	void loadVoice (int tg, const uint8_t v[kVoiceSize]) override
	{
		uint8_t frame[161];
		memset (frame, 0, sizeof frame);
		frame[0] = 0xF0; frame[1] = 0x43; frame[4] = 0x01; frame[5] = 0x1B;
		memcpy (frame + 6, v, kVoiceSize);
		m_pMini->loadVoiceParameters (frame, tg);
	}

	void readVoice (int tg, uint8_t v[kVoiceSize]) override
	{
		uint8_t dump[163];
		m_pMini->getSysExVoiceDump (dump, tg);
		memcpy (v, dump + 6, kVoiceSize);
	}

	void noteOn (int tg, int note, int velocity) override { m_pMini->keydown (note, velocity, tg); }
	void noteOff (int tg, int note) override { m_pMini->keyup (note, tg); }

	void panic () override
	{
		for (int tg = 0; tg < tgCount (); tg++) { m_pMini->panic (0, tg); m_pMini->notesOff (0, tg); }
	}

	void compareBegin (int tg) override
	{
		if (m_bComparing[tg]) return;
		readVoice (tg, m_Saved[tg]);
		m_bComparing[tg] = true;
		LoadStored (tg);
	}

	void compareEnd (int tg) override
	{
		if (!m_bComparing[tg]) return;
		m_bComparing[tg] = false;
		loadVoice (tg, m_Saved[tg]);
	}

	void revert (int tg) override { LoadStored (tg); }

	// MiniDexed reads its bank files once at boot. The user bank (file 99, index 98)
	// is served from the copy zicsave keeps after a save, so a saved voice shows up
	// in BROWSE straight away. (Program changes over MIDI still see the boot copy.)
	static constexpr int kUserBank = 98;
	static const uint8_t *UserCopy (int bank) { return bank == kUserBank ? UserBankFile () : nullptr; }

	int bankCount () override
	{
		int n = (int) m_pMini->GetSysExFileLoader ()->GetNumHighestBank () + 1;
		return UserBankFile () && n < kUserBank + 1 ? kUserBank + 1 : n;
	}
	bool bankValid (int bank) override { return UserCopy (bank) || m_pMini->GetSysExFileLoader ()->IsValidBank (bank); }

	bool voiceNameIn (int bank, int index, char out[kNameLen + 1]) override
	{
		if (const uint8_t *u = UserCopy (bank))
		{
			if (index < 0 || index >= zd::kBankSlots) return false;
			zd::SlotName (u, index, out);
			return true;
		}
		if (!bankValid (bank)) return false;
		std::string s = m_pMini->GetSysExFileLoader ()->GetVoiceName (bank, index);
		CopyName (out, s);
		return true;
	}

	bool readVoiceIn (int bank, int index, uint8_t v[kVoiceSize]) override
	{
		if (const uint8_t *u = UserCopy (bank))
		{
			if (index < 0 || index >= zd::kBankSlots) return false;
			zd::Voice voice;
			zd::UnpackVoice (u + 6 + index * zd::kPackedVoice, voice);
			memcpy (v, voice.d, kVoiceSize);
			return true;
		}
		if (!bankValid (bank)) return false;
		uint8_t buf[156];
		m_pMini->GetSysExFileLoader ()->GetVoice (bank, index, buf);
		memcpy (v, buf, kVoiceSize);
		return true;
	}

	bool savePerformance () override { return m_pMini->SavePerformance (false); }
	int saveVoice (const uint8_t v[kVoiceSize]) override { return SaveVoiceToUserBank (v); }
	int saveVoiceTo (const uint8_t v[kVoiceSize], int slot) override { return SaveVoiceToUserSlot (v, slot); }
	int userBankNames (char names[32][kNameLen + 1]) override { return ReadUserBankNames (names) ? 1 : -3; }

	void setMute (int tg, bool muted) override { m_pMini->SetExpression (muted ? 0 : 127, tg); }

	float level (int tg) override
	{
		if (tg < 0 || tg >= 8) return 0.0f;
		float f = g_ZicMeter.peak[tg];
		g_ZicMeter.peak[tg] = 0.0f;
		return f;
	}

	void netStatus (char out[32]) override
	{
		const char *p = ZicGetNetStatus ();
		if (!p[0] && m_pConfig->GetNetworkEnabled ()) p = "WiFi connecting";
		unsigned n = 0;
		for (; p[n] && n < 31; n++) out[n] = p[n];
		out[n] = 0;
	}

	void setScopeTG (int tg) override { g_ZicMeter.scopeTG = tg; }

	int scope (int16_t *out, int n) override
	{
		if (n > (int) ZicMeter::Taps) n = ZicMeter::Taps;
		unsigned h = g_ZicMeter.head;
		for (int i = 0; i < n; i++)
			out[i] = g_ZicMeter.ring[(h + ZicMeter::Taps - n + i) % ZicMeter::Taps];
		return n;
	}

private:
	static void CopyName (char out[kNameLen + 1], const std::string &s)
	{
		unsigned n = 0;
		for (; n < s.size () && n < (unsigned) kNameLen; n++)
			out[n] = (s[n] >= 32 && s[n] < 127) ? s[n] : ' ';
		for (; n < (unsigned) kNameLen; n++) out[n] = ' ';
		out[kNameLen] = 0;
	}

	// reload the voice the TG's bank slot holds, dropping any edit
	void LoadStored (int tg)
	{
		uint8_t v[kVoiceSize];
		unsigned bank = m_pMini->GetTGParameter (CMiniDexed::TGParameterVoiceBank, tg);
		unsigned prog = m_pMini->GetTGParameter (CMiniDexed::TGParameterProgram, tg);
		if (readVoiceIn (bank, prog, v)) loadVoice (tg, v);
	}

	static CMiniDexed::TTGParameter Map (TGParam p)
	{
		switch (p)
		{
		case P_VOLUME: return CMiniDexed::TGParameterVolume;
		case P_PAN: return CMiniDexed::TGParameterPan;
		case P_REVERB: return CMiniDexed::TGParameterReverbSend;
		case P_CUTOFF: return CMiniDexed::TGParameterCutoff;
		case P_RESO: return CMiniDexed::TGParameterResonance;
		case P_DETUNE: return CMiniDexed::TGParameterMasterTune;
		case P_CHANNEL: return CMiniDexed::TGParameterMIDIChannel;
		case P_PROGRAM: return CMiniDexed::TGParameterProgram;
		case P_BANK: return CMiniDexed::TGParameterVoiceBank;
		case P_MONO: return CMiniDexed::TGParameterMonoMode;
		case P_PORTA_TIME: return CMiniDexed::TGParameterPortamentoTime;
		case P_PORTA_MODE: return CMiniDexed::TGParameterPortamentoMode;
		case P_PB_RANGE: return CMiniDexed::TGParameterPitchBendRange;
		default: return CMiniDexed::TGParameterVolume;
		}
	}

	static CMiniDexed::TParameter MapFx (FxParam p)
	{
		switch (p)
		{
		case FX_COMPRESSOR: return CMiniDexed::ParameterCompressorEnable;
		case FX_REVERB_ENABLE: return CMiniDexed::ParameterReverbEnable;
		case FX_REVERB_SIZE: return CMiniDexed::ParameterReverbSize;
		case FX_REVERB_HIGHDAMP: return CMiniDexed::ParameterReverbHighDamp;
		case FX_REVERB_LOWDAMP: return CMiniDexed::ParameterReverbLowDamp;
		case FX_REVERB_LOWPASS: return CMiniDexed::ParameterReverbLowPass;
		case FX_REVERB_DIFFUSION: return CMiniDexed::ParameterReverbDiffusion;
		default: return CMiniDexed::ParameterReverbLevel;
		}
	}

	CMiniDexed *m_pMini;
	CConfig *m_pConfig;
	uint8_t m_Saved[kMaxTG][kVoiceSize];
	bool m_bComparing[kMaxTG];
};

} // namespace

zic::ISynth *ZicCreateSynth (CMiniDexed *pMiniDexed, CConfig *pConfig)
{
	return new CZicSynth (pMiniDexed, pConfig);
}
