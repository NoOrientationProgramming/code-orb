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

using namespace std;

enum SwtFlowControlBytes
{
	FlowCtrlToTarget = 0xF0,
	FlowTargetToCtrl
};

enum SwtContentIdInBytes
{
	ContentInNone = 0x00,
	ContentInLog = 0xC0,
	ContentInCmd,
	ContentInProc,
};

enum SwtContentIdOutBytes
{
	ContentOutCmd = 0xC0,
};

#define dTimeoutTargetInitMs	500

SingleWireControlling::SingleWireControlling()
	: Processing("SingleWireControlling")
	, mStartMs(0)
	, mStateRet(StStart)
	, mRefUart(RefDeviceUartInvalid)
	, mDevUartIsOnline(false)
	, mTargetIsOnline(false)
{
	mState = StStart;
}

/* member functions */

Success SingleWireControlling::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	ssize_t lenDone;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		mState = StUartInit;

		break;
	case StUartInit:

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

		mStartMs = curTimeMs;
		mState = StTargetInitDoneWait;

		break;
	case StTargetInitDoneWait:

		lenDone = uartRead(mRefUart, mBufRcv, sizeof(mBufRcv));
		if (lenDone < 0)
		{
			mState = StUartInit;
			break;
		}

		if (diffMs > dTimeoutTargetInitMs)
		{
			mState = StTargetInit;
			break;
		}

		if (!lenDone)
			break;

		hexDump(mBufRcv, lenDone);

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
	uartSend(mRefUart, cmd);
	uartSend(mRefUart, 0x00);
}

void SingleWireControlling::dataRequest()
{
	uartSend(mRefUart, FlowTargetToCtrl);
}

void SingleWireControlling::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	dInfo("UART: %s\t%sline\n",
			env.deviceUart.c_str(),
			mDevUartIsOnline ? "On" : "Off");
	dInfo("Target\t\t\t%sline\n", mTargetIsOnline ? "On" : "Off");
}

/* static functions */

