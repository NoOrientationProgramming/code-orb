/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 10.10.2022

  Copyright (C) 2022, Johannes Natter
*/

#include "TelnetFiltering.h"
#include "LibTime.h"

#define dForEach_KeyState(gen) \
		gen(StKeyMain) \
		gen(StKeyUnicode) \
		gen(StKeyDrop1) \
		gen(StKeyEscMain) \
		gen(StKeyEscO) \
		gen(StKeyEscBracket) \
		gen(StKeyEsc1) \
		gen(StKeyMod) \
		gen(StKeyEsc2) \
		gen(StKeyEsc3) \
		gen(StKeyEscTilde) \
		gen(StKeyIac) \
		gen(StKeyIacWill) \
		gen(StKeyIacWont) \
		gen(StKeyIacDo) \
		gen(StKeyIacDont) \
		gen(StKeyPrint) \

#define dGenKeyStateEnum(s) s,
dProcessStateEnum(KeyState);

#if 1
#define dGenKeyStateString(s) #s,
dProcessStateStr(KeyState);
#endif

#define dByteCheckCtrlKeyCommit(b, k)	\
if (key == b) \
{ \
	keyCtrlCommit(k, mModShift, mModAlt, mModCtrl); \
	mStateKey = StKeyMain; \
	return Positive; \
}

#define dKeyIgnore(k)	\
if (key == k) \
{ \
	/* procInfLog("ignoring %u, 0x%02X", key, key); */ \
	mStateKey = StKeyMain; \
	return Pending; \
}

// --------------------

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StTelnetInit) \
		gen(StMain) \
		gen(StTelnetDeInit) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

// http://www.iana.org/assignments/telnet-options/telnet-options.xhtml#telnet-options-1
#define keyIacWill		0xFB // RFC854
#define keyIacWont		0xFC // RFC854
#define keyIacDo		0xFD // RFC854
#define keyIacDont		0xFE // RFC854
#define keyIac			0xFF // RFC854
#define keyEcho		0x01 // RFC857
#define keySuppGoAhd	0x03 // RFC858
#define keyStatus		0x05 // RFC859
#define keyLineMode		0x22 // RFC1184
#define keyEncrypt		0x26 // RFC2946

TelnetFiltering::TelnetFiltering(int fd)
	: KeyFiltering("TelnetFiltering")
	, mStateKey(StKeyMain)
	, mStateKeyRet(StKeyMain)
	, mSocketFd(fd)
	, mpConn(NULL)
#if CONFIG_PROC_HAVE_DRIVERS
	, mMtxConn()
#endif
	, mTitle("")
	, mFragmentUtf("")
	, mCntFragment(0)
	, mModShift(false)
	, mModAlt(false)
	, mModCtrl(false)
	, mNumCommited(0)
	, mLast()
{
	mState = StStart;
}

/* member functions */

void TelnetFiltering::titleSet(const string &title)
{
	mTitle = title;
}

Success TelnetFiltering::process()
{
	Success success;
	string msg = "";
#if 0
	dStateTrace;
#endif
	if (mpConn && mpConn->success() != Pending)
		return Positive;

	switch (mState)
	{
	case StStart:

		mpConn = TcpTransfering::create(mSocketFd);
		if (!mpConn)
			return procErrLog(-1, "Could not create process");

		mpConn->procTreeDisplaySet(false);

		start(mpConn);

		mState = StTelnetInit;

		break;
	case StTelnetInit:

		// IAC WILL ECHO
		msg += "\xFF\xFB\x01";

		// IAC WILL SUPPRESS_GO_AHEAD
		msg += "\xFF\xFB\x03";

		// IAC WONT LINEMODE
		msg += "\xFF\xFC\x22";
#if 0
		// Deactivate Application Cursor Mode
		msg += "\033[?1l";

		// ModifyOtherKeys Mode
		msg += "\033[>4;2m";

		// Xterm Extended Keyboard Protocol
		msg += "\033[>1;5m";
#endif
		// Hide cursor
		msg += "\033[?25l";

		// Alternative screen buffer
		msg += "\033[?1049h";

		// Set terminal title
		if (mTitle.size())
		{
			msg += "\033]2;";
			msg += mTitle;
			msg += "\a";
		}

		// Clear screen
		msg += "\033[2J\033[H";

		mSendReady = true;
		mState = StMain;

		break;
	case StMain:

		success = dataProcess();
		if (success != Pending)
			break;

		break;
	case StTelnetDeInit:

		return Positive;

		break;
	default:
		break;
	}

	if (!msg.size())
		return Pending;

	mpConn->send(msg.c_str(), msg.size());

	return Pending;
}

Success TelnetFiltering::shutdown()
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mMtxConn);
#endif
	if (!mpConn)
		return Positive;

	string msg;

	// Show cursor
	msg += "\033[?25h";

	// Restore screen buffer
	msg += "\033[?1049l";

	mpConn->send(msg.c_str(), msg.size());
	mpConn->doneSet();
	mpConn = NULL;

	return Positive;
}

ssize_t TelnetFiltering::read(void *pBuf, size_t len)
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mMtxConn);
#endif
	if (!mpConn)
		return -1;

	return mpConn->read(pBuf, len);
}

ssize_t TelnetFiltering::send(const void *pData, size_t len)
{
#if CONFIG_PROC_HAVE_DRIVERS
	Guard lock(mMtxConn);
#endif
	if (!mpConn)
		return -1;

	if (!mSendReady)
		return procErrLog(-1, "unable to send data. Not ready");

	return mpConn->send(pData, len);
}

Success TelnetFiltering::dataProcess()
{
	ssize_t numBytesRead;
	char buf[8];
	const char *pKey;
	Success success;

	numBytesRead = mpConn->read(buf, sizeof(buf) - 1);
	if (numBytesRead <= 0)
		return Pending;

	buf[numBytesRead] = 0;
	pKey = buf;
#if 0
	procWrnLog("data received. len = %d", numBytesRead);
	hexDump(buf, numBytesRead);
#endif
	if (numBytesRead == 1 && buf[0] == keyEsc)
	{
		keyCtrlCommit(keyEsc);
		return Positive;
	}

	if (numBytesRead == 2 && buf[0] == keyEsc && buf[1] == '[')
	{
		keyPrintCommit(buf[1], false, true);
		return Positive;
	}

	for (ssize_t i = 0; i < numBytesRead; ++i, ++pKey)
	{
		success = keyGet(*pKey);
		if (success == Pending)
			continue;

		if (success != Positive)
		{
			mState = StTelnetDeInit;
			return procErrLog(-1, "could not get key");
		}
	}

	return Positive;;
}

// https://en.wikipedia.org/wiki/ANSI_escape_code#Terminal_input_sequences
Success TelnetFiltering::keyGet(uint8_t key)
{
#if 0
	procWrnLog("mState = %s", KeyStateString[mStateKey]);
#endif
#if 0
	procInfLog("key received: 0x%02X '%c'", key, key);
#endif
	switch (mStateKey)
	{
	case StKeyMain:

		mModShift = false;
		mModAlt = false;
		mModCtrl = false;

		if (key == keyIac)
		{
			//procWrnLog("Telnet command received");
			mStateKey = StKeyIac;
			break;
		}

		if (key == keyEsc)
		{
			mStateKey = StKeyEscMain;
			break;
		}

		if (key == keyCtrlD)
		{
			mState = StTelnetDeInit;
			break;
		}

		if (key == keyEnter)
		{
			mStateKey = StKeyDrop1;

			keyCtrlCommit(key);
			return Positive;
		}

		if (key & 0x80)
		{
			if ((key & 0xE0) == 0xC0) mCntFragment = 2;
			if ((key & 0xF0) == 0xE0) mCntFragment = 3;
			if ((key & 0xF8) == 0xF0) mCntFragment = 4;

			mFragmentUtf.clear();
			mFragmentUtf.push_back(key);

			mStateKey = StKeyUnicode;
			break;
		}

		if (isprint(key))
			keyPrintCommit(key);
		else
			keyCtrlCommit(key);

		return Positive;

		break;
	case StKeyUnicode:

		if ((key & 0xC0) == 0x80)
		{
			mFragmentUtf.push_back(key);

			if (mFragmentUtf.size() < mCntFragment)
				break;

			u32string ustr;
			strToUtf(mFragmentUtf, ustr);

			keyPrintCommit(ustr[0]);
			mStateKey = StKeyMain;
			return Positive;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyDrop1:

		mStateKey = StKeyMain;

		break;
	case StKeyEscMain:

		if (key == keyEsc)
		{
			keyCtrlCommit(keyEsc);
			return Positive;
		}

		if (key == '[')
		{
			mStateKey = StKeyEscBracket;
			break;
		}

		if (key == 'O')
		{
			mStateKey = StKeyEscO;
			break;
		}

		if (isprint(key))
		{
			keyPrintCommit(key, false, true);
			mStateKey = StKeyMain;
			return Positive;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyEscO:

		if (key >= 'A' || key <= 'D')
		{
			mModCtrl = true;

			dByteCheckCtrlKeyCommit('A', keyUp);
			dByteCheckCtrlKeyCommit('B', keyDown);
			dByteCheckCtrlKeyCommit('C', keyRight);
			dByteCheckCtrlKeyCommit('D', keyLeft);
		}

		break;
	case StKeyEscBracket:

		dByteCheckCtrlKeyCommit('A', keyUp);
		dByteCheckCtrlKeyCommit('B', keyDown);
		dByteCheckCtrlKeyCommit('C', keyRight);
		dByteCheckCtrlKeyCommit('D', keyLeft);

		dByteCheckCtrlKeyCommit('F', keyEnd);
		dByteCheckCtrlKeyCommit('H', keyHome);

		dByteCheckCtrlKeyCommit('Z', keyTab);

		if (key == '1')
		{
			mStateKey = StKeyEsc1;
			break;
		}

		if (key == '2')
		{
			mStateKey = StKeyEsc2;
			break;
		}

		if (key == '3')
		{
			mStateKey = StKeyEsc3;
			break;
		}

		if (key == '4' || key == '8')
		{
			keyCtrlCommit(keyEnd);
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key == '5')
		{
			keyCtrlCommit(keyPgUp);
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key == '6')
		{
			keyCtrlCommit(keyPgDn);
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key == '7')
		{
			keyCtrlCommit(keyHome);
			mStateKey = StKeyEscTilde;
			break;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyEsc1:

		dByteCheckCtrlKeyCommit('~', keyHome);

		if (key == ';')
		{
			statesKeySet(StKeyMod, StKeyEscBracket);
			break;
		}

		if (key >= '0' && key <= '5')
		{
			keyCtrlCommit(keyF0 + key);
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key >= '7' && key <= '9')
		{
			keyCtrlCommit(keyF6 + (key - 7));
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key >= 'P' && key <= 'S')
		{
			keyCtrlCommit(keyF1 + (key - 'P'));
			mStateKey = StKeyMain;
			return Positive;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyMod:

		key -= '1';

		if (key & 1)
			mModShift = true;

		if (key & 2)
			mModAlt = true;

		if (key & 4)
			mModCtrl = true;

		mStateKey = mStateKeyRet;

		break;
	case StKeyEsc2:

		dByteCheckCtrlKeyCommit('~', keyInsert);

		if (key == ';')
		{
			statesKeySet(StKeyMod, StKeyEsc2);
			break;
		}

		if (key >= '0' && key <= '1')
		{
			keyCtrlCommit(keyF9 + key);
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key >= '3' && key <= '6')
		{
			keyCtrlCommit(keyF11 + (key - 3));
			mStateKey = StKeyEscTilde;
			break;
		}

		if (key >= '8' && key <= '9')
		{
			keyCtrlCommit(keyF15 + (key - 8));
			mStateKey = StKeyEscTilde;
			break;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyEsc3:

		dByteCheckCtrlKeyCommit('~', keyDelete);

		if (key == ';')
		{
			statesKeySet(StKeyMod, StKeyEsc3);
			break;
		}

		if (key >= '1' && key <= '4')
		{
			keyCtrlCommit(keyF17 + (key - 1));
			mStateKey = StKeyEscTilde;
			break;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyEscTilde:

		if (key == '~')
		{
			mStateKey = StKeyMain;
			return Positive;
		}

		return procErrLog(-1, "unexpected key 0x%02X '%c' in state %s",
							key, key, KeyStateString[mStateKey]);

		break;
	case StKeyIac:

		if (key == keyIacWill)
			mStateKey = StKeyIacWill;
		else
		if (key == keyIacWont)
			mStateKey = StKeyIacWont;
		else
		if (key == keyIacDo)
			mStateKey = StKeyIacDo;
		else
		if (key == keyIacDont)
			mStateKey = StKeyIacDont;
		else
			return procErrLog(-1, "unsupported IAC: 0x%02X", key);

		break;
	case StKeyIacWill:

		dKeyIgnore(keyEncrypt);

		procWrnLog("unknown WILL option: 0x%02X", key);
		mStateKey = StKeyMain;

		break;
	case StKeyIacWont:

		dKeyIgnore(keyLineMode);

		procWrnLog("unknown WONT option: 0x%02X", key);
		mStateKey = StKeyMain;

		break;
	case StKeyIacDo:

		dKeyIgnore(keyEcho);
		dKeyIgnore(keySuppGoAhd);
		dKeyIgnore(keyStatus);

		procWrnLog("unknown DO option: 0x%02X", key);
		mStateKey = StKeyMain;

		break;
	case StKeyIacDont:

		procWrnLog("unknown DONT option: 0x%02X", key);
		mStateKey = StKeyMain;

		break;
	case StKeyPrint:

		procWrnLog("got key 0x%02X '%c'", key, key);

		break;
	default:
		break;
	}

	return Pending;
}

void TelnetFiltering::keyPrintCommit(char32_t key, bool modShift, bool modAlt, bool modCtrl)
{
	KeyUser cKey;

	cKey = key;

	cKey.modShiftSet(modShift);
	cKey.modAltSet(modAlt);
	cKey.modCtrlSet(modCtrl);

	ppKeys.commit(cKey, millis());

	++mNumCommited;
	mLast = cKey;
}

void TelnetFiltering::keyCtrlCommit(CtrlKeyUser key, bool modShift, bool modAlt, bool modCtrl)
{
	KeyUser cKey;

	cKey.keyCtrlSet(key);

	cKey.modShiftSet(modShift);
	cKey.modAltSet(modAlt);
	cKey.modCtrlSet(modCtrl);

	ppKeys.commit(cKey, millis());

	++mNumCommited;
	mLast = cKey;
}

void TelnetFiltering::statesKeySet(uint32_t state, uint32_t stateRet)
{
	mStateKey = state;
	mStateKeyRet = stateRet;
}

void TelnetFiltering::processInfo(char *pBuf, char *pBufEnd)
{
	(void)pBuf;
	(void)pBufEnd;
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
	dInfo("State Key\t\t%s\n", KeyStateString[mStateKey]);
	dInfo("Commited\t\t%zu\n", mNumCommited);
	dInfo("Last\t\t\t%s\n", mLast.str().c_str());
#endif
}

/* static functions */

