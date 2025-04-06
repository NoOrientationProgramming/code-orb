/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 31.03.2025

  Copyright (C) 2025, Johannes Natter

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "SingleWireControlling.h"
#include "SystemDebugging.h"
#include "LibTime.h"
#include "LibDspc.h"

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StUartInit) \
		gen(StDevUartInit) \
		gen(StTargetInit) \
		gen(StTargetInitDoneWait) \
		gen(StNextFlowDetermine) \
		gen(StContentReceive) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

#define dForEach_SwtState(gen) \
		gen(StSwtMain) \
		gen(StSwtDataReceive) \

#define dGenSwtStateEnum(s) s,
dProcessStateEnum(SwtState);

#if 1
#define dGenSwtStateString(s) #s,
dProcessStateStr(SwtState);
#endif

using namespace std;

enum SwtFlowDirection
{
	FlowCtrlToTarget = 0xF1,
	FlowTargetToCtrl
};

enum SwtContentIdOut
{
	ContentOutCmd = 0x90,
};

enum SwtContentId
{
	ContentNone = 0xA0,
	ContentProc,
	ContentLog,
	ContentCmd,
};

enum SwtContentEnd
{
	ContentCut = 0x0F,
	ContentEnd = 0x17,
};

#define dTimeoutTargetInitMs	50
const size_t cSizeFragmentMax = 4095;

static uint8_t uartVirtualTimeout = 0;

SingleWireControlling::SingleWireControlling()
	: Processing("SingleWireControlling")
	, mStateSwt(StSwtMain)
	, mStartMs(0)
	, mRefUart(RefDeviceUartInvalid)
	, mDevUartIsOnline(false)
	, mTargetIsOnline(false)
	, mpBuf(NULL)
	, mLenDone(0)
	, mFragments()
	, mContentCurrent(ContentNone)
{
	mResp.idContent = ContentNone;
	mResp.content = "";

	mState = StStart;
}

/* member functions */

Success SingleWireControlling::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		cmdReg("ctrlManualToggle", cmdCtrlManualToggle,      "",  "Toggle manual control",               "Manual Control");
		cmdReg("dataUartSend",     cmdDataUartSend,          "",  "Send byte stream",                    "Manual Control");
		cmdReg("strUartSend",      cmdStrUartSend,           "",  "Send string",                         "Manual Control");
		cmdReg("dataUartRead",     cmdDataUartRead,          "",  "Read data",                           "Manual Control");
		cmdReg("modeUartVirtSet",  cmdModeUartVirtSet,       "",  "Mode: uart, swart (default)",         "Virtual UART");
		cmdReg("uartVirtToggle",   cmdUartVirtToggle,        "",  "Enable/Disable virtual UART",         "Virtual UART");
		cmdReg("mountedToggle",    cmdMountedUartVirtToggle, "m", "Mount/Unmount virtual UART",          "Virtual UART");
		cmdReg("timeoutToggle",    cmdTimeoutUartVirtToggle, "t", "Enable/Disable virtual UART timeout", "Virtual UART");
		cmdReg("dataUartRcv",      cmdDataUartRcv,           "",  "Receive byte stream",                 "Virtual UART");
		cmdReg("strUartRcv",       cmdStrUartRcv,            "",  "Receive string",                      "Virtual UART");

		mState = StUartInit;

		break;
	case StUartInit:

		if (mRefUart != RefDeviceUartInvalid)
			devUartDeInit(mRefUart);

		mDevUartIsOnline = false;

		mState = StDevUartInit;

		break;
	case StDevUartInit:

		success = devUartInit(env.deviceUart, mRefUart);
		if (success == Pending)
			break;

		if (success != Positive)
			return procErrLog(-1, "could not initalize UART device");

		// clear UART device buffer?

		mDevUartIsOnline = true;

		mState = StTargetInit;

		break;
	case StTargetInit:

		mTargetIsOnline = false;

		cmdSend("aaaaa");
		dataRequest();

		mState = StTargetInitDoneWait;

		break;
	case StTargetInitDoneWait:

		success = dataReceive();
		if (success == Pending)
			break;

		if (success == SwtErrRcvNoUart)
		{
			mState = StUartInit;
			break;
		}

		if (success == SwtErrRcvNoTarget)
		{
			mState = StTargetInit;
			break;
		}

		hexDump(mBufRcv, mLenDone);

		mTargetIsOnline = true;

		mState = StNextFlowDetermine;

		break;
	case StNextFlowDetermine:

		// flow determine
#if 0
		dataRequest();
#endif
		break;
	case StContentReceive:

		break;
	default:
		break;
	}

	return Pending;
}

void SingleWireControlling::cmdSend(const string &cmd)
{
	uartSend(mRefUart, FlowCtrlToTarget);
	uartSend(mRefUart, ContentOutCmd);
	uartSend(mRefUart, cmd.data(), cmd.size());
	uartSend(mRefUart, 0x00);

	mStartMs = millis();
}

void SingleWireControlling::dataRequest()
{
	uartSend(mRefUart, FlowTargetToCtrl);
	mStartMs = millis();
}

Success SingleWireControlling::dataReceive()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;

	if (mLenDone > 0)
		mStartMs = millis();

	for (; mLenDone > 0; --mLenDone, ++mpBuf)
	{
		success = byteProcess(*mpBuf);
		if (success == Pending)
			continue;

		if (success != Positive)
			return SwtErrRcvProtocol;

		return Positive;
	}

	if (!uartVirtual && diffMs > dTimeoutTargetInitMs)
		return SwtErrRcvNoTarget;

	if (uartVirtual && uartVirtualTimeout)
		return SwtErrRcvNoTarget;

	mLenDone = uartRead(mRefUart, mBufRcv, sizeof(mBufRcv));
	if (!mLenDone)
		return Pending;

	if (mLenDone < 0)
		return SwtErrRcvNoUart;

	mpBuf = mBufRcv;

	mStartMs = millis();

	return Pending;
}

Success SingleWireControlling::byteProcess(char ch)
{
	(void)ch;

	switch (mStateSwt)
	{
	case StSwtMain:

		procInfLog("Received byte: 0x%02X '%c'", (uint8_t)ch, ch);

		//mStateSwt = StSwtDataReceive;

		break;
	case StSwtDataReceive:
#if 0
		if (*pBuf >= ContentLog && *pBuf <= ContentProc)
		{
			mContentCurrent = *pBuf;

			++pBuf;
			--mLenDone;
		}

		if (*pEnd == ContentEnd)
		{
			fragmentFinish(pBuf, mLenDone);
			return Positive;
		}

		fragmentAppend(pBuf, mLenDone);
#endif
		break;
	default:
		break;
	}

	return Pending;
}

Success SingleWireControlling::shutdown()
{

	if (mRefUart != RefDeviceUartInvalid)
		devUartDeInit(mRefUart);

	return Positive;
}

void SingleWireControlling::fragmentAppend(const char *pBuf, size_t len)
{
	bool fragmentFound =
			mFragments.find(mContentCurrent) != mFragments.end();

	if (!fragmentFound)
	{
		mFragments[mContentCurrent] = string(pBuf, len);
		return;
	}

	string &str = mFragments[mContentCurrent];

	if (str.size() > cSizeFragmentMax)
		return;

	mFragments[mContentCurrent] += string(pBuf, len);
}

void SingleWireControlling::fragmentFinish(const char *pBuf, size_t len)
{
	bool fragmentFound =
			mFragments.find(mContentCurrent) != mFragments.end();

	mResp.idContent = mContentCurrent;
	mResp.content = "";

	if (fragmentFound)
	{
		mResp.content = mFragments[mContentCurrent];
		mFragments.erase(mContentCurrent);
	};

	mResp.content += string(pBuf, len);

	mContentCurrent = ContentNone;
}

void SingleWireControlling::processInfo(char *pBuf, char *pBufEnd)
{
	dInfo("Manual control\t\t%sabled\n", env.ctrlManual ? "En" : "Dis");
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
#if 1
	dInfo("State SWT\t\t\t%s\n", SwtStateString[mStateSwt]);
#endif
	dInfo("Virtual UART mode\t\t%s\n", uartVirtualMode ? "uart" : "swart");
	dInfo("Virtual UART\t\t%sabled\n", uartVirtual ? "En" : "Dis");
	dInfo("UART: %s\t%sline\n",
			env.deviceUart.c_str(),
			mDevUartIsOnline ? "On" : "Off");
	dInfo("Target\t\t\t%sline\n", mTargetIsOnline ? "On" : "Off");
}

/* static functions */

void SingleWireControlling::cmdCtrlManualToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	env.ctrlManual ^= 1;
	dInfo("Manual control %sabled", env.ctrlManual ? "en" : "dis");
}

void SingleWireControlling::cmdDataUartSend(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	dataUartSend(pArgs, pBuf, pBufEnd, uartSend);
}

void SingleWireControlling::cmdStrUartSend(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	strUartSend(pArgs, pBuf, pBufEnd, uartSend);
}

void SingleWireControlling::cmdDataUartRead(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	// TODO: Implement cmdDataUartRead()
}

void SingleWireControlling::cmdModeUartVirtSet(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (pArgs && *pArgs == 'u')
		uartVirtualMode = 1;
	else
		uartVirtualMode = 0;

	dInfo("Virtual UART mode: %s", uartVirtualMode ? "uart" : "swart");
}

void SingleWireControlling::cmdUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtual ^= 1;
	dInfo("Virtual UART %sabled", uartVirtual ? "en" : "dis");
}

void SingleWireControlling::cmdMountedUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtualMounted ^= 1;
	dInfo("Virtual UART %smounted", uartVirtualMounted ? "" : "un");
}

void SingleWireControlling::cmdTimeoutUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtualTimeout ^= 1;
	dInfo("Virtual UART timeout %s", uartVirtualTimeout ? "set" : "cleared");
}

void SingleWireControlling::cmdDataUartRcv(char *pArgs, char *pBuf, char *pBufEnd)
{
	dataUartSend(pArgs, pBuf, pBufEnd, uartVirtRcv);
}

void SingleWireControlling::cmdStrUartRcv(char *pArgs, char *pBuf, char *pBufEnd)
{
	strUartSend(pArgs, pBuf, pBufEnd, uartVirtRcv);
}

void SingleWireControlling::dataUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend)
{
	if (!pArgs)
	{
		dInfo("No data given");
		return;
	}

	string str = string(pArgs, strlen(pArgs));

	if (str == "none") str = "A0";
	if (str == "proc") str = "A1";
	if (str == "log")  str = "A2";
	if (str == "cmd")  str = "A3";

	vector<char> vData = toHex(str);
	pFctSend(RefDeviceUartInvalid, vData.data(), vData.size());

	dInfo("Data received");
}

void SingleWireControlling::strUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend)
{
	if (!pArgs)
	{
		dInfo("No string given");
		return;
	}

	pFctSend(RefDeviceUartInvalid, pArgs, strlen(pArgs));
	dInfo("String received");
}

