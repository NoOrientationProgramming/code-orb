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

#include "SingleWireScheduling.h"
#include "SingleWire.h"
#include "LibTime.h"

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StUartInit) \
		gen(StDevUartInit) \
		gen(StTargetInit) \
		gen(StTargetInitDoneWait) \
		gen(StNextFlowDetermine) \
		gen(StContentReceiveWait) \
		gen(StCtrlManual) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

#define dForEach_SwtState(gen) \
		gen(StSwtContentRcvWait) \
		gen(StSwtDataReceive) \

#define dGenSwtStateEnum(s) s,
dProcessStateEnum(SwtState);

#if 1
#define dGenSwtStateString(s) #s,
dProcessStateStr(SwtState);
#endif

using namespace std;

#define dTimeoutTargetInitMs	65
const uint8_t cKeyEscape = 0x1B;
const uint8_t cKeyTab = '\t';
const uint8_t cKeyCr = '\r';
const uint8_t cKeyLf = '\n';

const size_t SingleWireScheduling::cSizeFragmentMax = 4095;
const uint32_t SingleWireScheduling::cTimeoutCmduC = 100;
const uint32_t SingleWireScheduling::cTimeoutCmdReq = 5500;

uint8_t SingleWireScheduling::uartVirtualTimeout = 0;
RefDeviceUart SingleWireScheduling::refUart;

list<CommandReqResp> SingleWireScheduling::requestsCmd[3];
list<CommandReqResp> SingleWireScheduling::responsesCmd;
uint32_t SingleWireScheduling::idReqCmdNext = 0;

SingleWireScheduling::SingleWireScheduling()
	: Processing("SingleWireScheduling")
	, mDevUartIsOnline(false)
	, mTargetIsOnline(false)
	, mContentProc("")
	, mStateSwt(StSwtContentRcvWait)
	, mStartMs(0)
	, mRefUart(RefDeviceUartInvalid)
	, mpBuf(NULL)
	, mLenDone(0)
	, mFragments()
	, mContentProcChanged(false)
	, mCntBytesRcvd(0)
	, mCntContentNoneRcvd(0)
	, mLastProcTreeRcvdMs(0)
	, mTargetIsOnlineOld(true)
	, mTargetIsOfflineMarked(false)
	, mContentIgnore(false)
	, mpListCmdCurrent(NULL)
	, mCntDelayPrioLow(0)
	, mStartCmdMs(0)
{
	responseReset();
	mBufRcv[0] = 0;

	mState = StStart;
}

/* member functions */

Success SingleWireScheduling::process()
{
	uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		commandsRegister();

		mState = StUartInit;

		break;
	case StUartInit:

		if (mRefUart != RefDeviceUartInvalid)
		{
			devUartDeInit(mRefUart);
			refUart = mRefUart;
		}

		mDevUartIsOnline = false;
		targetOnlineSet(false);

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
		refUart = mRefUart;

		mState = StTargetInit;

		break;
	case StTargetInit:

		targetOnlineSet(false);

		if (env.ctrlManual)
		{
			mState = StCtrlManual;
			break;
		}

		ok = cmdSend(env.codeUart);
		if (!ok)
		{
			mState = StUartInit;
			break;
		}

		ok = dataRequest();
		if (!ok)
		{
			mState = StUartInit;
			break;
		}

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
#if 0
		procWrnLog("content received: %02X > '%s'",
						mResp.idContent,
						mResp.content.c_str());
#endif
		if (mResp.idContent != IdContentCmd)
			break;

		if (mResp.content != "Debug mode 1")
			break;

		targetOnlineSet();

		mpListCmdCurrent = NULL;
		mStartCmdMs = 0;

		mState = StNextFlowDetermine;

		break;
	case StNextFlowDetermine:

		if (env.ctrlManual)
		{
			mState = StCtrlManual;
			break;
		}

		commandsCheck(curTimeMs);

		// flow determine

		ok = cmdQueueCheck();
		if (ok)
			break;

		ok = dataRequest();
		if (!ok)
		{
			mState = StUartInit;
			break;
		}

		mState = StContentReceiveWait;

		break;
	case StContentReceiveWait:

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
#if 0
		if (mResp.idContent != IdContentNone)
		{
			procWrnLog("content received: %02X",
						mResp.idContent);
#if 0
			procWrnLog("'%s'", mResp.content.c_str());
#endif
		}
#endif
		if (mResp.idContent == IdContentProc &&
				mContentProc != mResp.content)
		{
			mTargetIsOfflineMarked = false;

			mContentProc = mResp.content;
			mContentProcChanged = true;
		}

		if (mResp.idContent == IdContentLog)
			ppEntriesLog.commit(mResp.content);

		if (mResp.idContent == IdContentCmd)
			cmdResponseReceived(mResp.content);

		responseReset();

		mState = StNextFlowDetermine;

		break;
	case StCtrlManual:

		if (!env.ctrlManual)
		{
			mState = StTargetInit;
			break;
		}

		break;
	default:
		break;
	}

	return Pending;
}

bool SingleWireScheduling::contentProcChanged()
{
	bool tmp = mContentProcChanged;
	mContentProcChanged = false;
	return tmp;
}

bool SingleWireScheduling::cmdQueueCheck()
{
	if (mpListCmdCurrent)
		return false;

	if (requestsCmd[PrioUser].size())
		mpListCmdCurrent = &requestsCmd[PrioUser];
	else
	if (requestsCmd[PrioSysLow].size())
	{
		if (mCntDelayPrioLow)
			return false;

		mpListCmdCurrent = &requestsCmd[PrioSysLow];
		mCntDelayPrioLow = 4;
	}

	if (!mpListCmdCurrent)
		return false;
	mStartCmdMs = millis();

	bool ok;

	const CommandReqResp *pReq = &mpListCmdCurrent->front();
	ok = cmdSend(pReq->str);
	if (!ok)
		return false;

	return true;
}

void SingleWireScheduling::cmdResponseReceived(const string &resp)
{
	if (!mpListCmdCurrent)
		return;
#if 0
	procWrnLog("command response received: %s",
				resp.c_str());
#endif
	uint32_t idReq = mpListCmdCurrent->front().idReq;
	mpListCmdCurrent->pop_front();
	responsesCmd.emplace_back(resp, idReq, millis());

	mpListCmdCurrent = NULL;
}

void SingleWireScheduling::commandsCheck(uint32_t curTimeMs)
{
	cmdResponsesClear(curTimeMs);

	if (!mpListCmdCurrent)
		return;

	uint32_t diffMs = curTimeMs - mStartCmdMs;

	if (diffMs > cTimeoutCmduC)
		mpListCmdCurrent = NULL;
}

void SingleWireScheduling::cmdResponsesClear(uint32_t curTimeMs)
{
	list<CommandReqResp>::iterator iter;
	uint32_t diffMs;

	iter = responsesCmd.begin();
	while (iter != responsesCmd.end())
	{
		diffMs = curTimeMs - iter->startMs;

		if (diffMs < cTimeoutCmdReq)
		{
			++iter;
			continue;
		}
#if 0
		procWrnLog("response timeout for: %u",
					iter->idReq);
#endif
		iter = responsesCmd.erase(iter);
	}
}

bool SingleWireScheduling::cmdSend(const string &cmd)
{
	ssize_t lenWritten;

	lenWritten = uartSend(mRefUart, FlowSchedToTarget);
	if (lenWritten < 0)
		return false;

	(void)uartSend(mRefUart, IdContentScToTaCmd);
	(void)uartSend(mRefUart, cmd.data(), cmd.size());
	(void)uartSend(mRefUart, 0x00);
	(void)uartSend(mRefUart, IdContentEnd);

	mStartMs = millis();

	//procWrnLog("cmd sent: %s", cmd.c_str());

	return true;
}

bool SingleWireScheduling::dataRequest()
{
	ssize_t lenWritten;

	lenWritten = uartSend(mRefUart, FlowTargetToSched);
	if (lenWritten < 0)
		return false;

	mStartMs = millis();

	//procWrnLog("data requested");

	if (mCntDelayPrioLow)
	{
		--mCntDelayPrioLow;
		//procWrnLog("low prio delay: %u", mCntDelayPrioLow);
	}

	return true;
}

Success SingleWireScheduling::dataReceive()
{
	if (mLenDone > 0)
		mStartMs = millis();

	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;

	while (mLenDone > 0)
	{
		success = byteProcess((uint8_t)*mpBuf, curTimeMs);

		++mpBuf;
		--mLenDone;

		if (success == Positive)
			return Positive;
	}

	//procInfLog("diff: %u <=> %u", diffMs, dTimeoutTargetInitMs);

	if ((!uartVirtual && diffMs > dTimeoutTargetInitMs) ||
		(uartVirtual && uartVirtualTimeout))
	{
		mFragments.clear();
		mStateSwt = StSwtContentRcvWait;

		return SwtErrRcvNoTarget;
	}

	mLenDone = uartRead(mRefUart, mBufRcv, sizeof(mBufRcv));
	if (!mLenDone)
		return Pending;

	if (mLenDone < 0)
	{
		mFragments.clear();
		mStateSwt = StSwtContentRcvWait;

		return SwtErrRcvNoUart;
	}

	mpBuf = mBufRcv;

	mStartMs = millis();

	return Pending;
}

Success SingleWireScheduling::byteProcess(uint8_t ch, uint32_t curTimeMs)
{
	uint32_t diffMs = curTimeMs - mLastProcTreeRcvdMs;
#if 0
	procInfLog("received byte in %s: 0x%02X '%c'",
				SwtStateString[mStateSwt], ch, ch);
#endif
	++mCntBytesRcvd;

	switch (mStateSwt)
	{
	case StSwtContentRcvWait:

		if (ch == IdContentNone)
		{
			//procWrnLog("received IdContentNone");
			++mCntContentNoneRcvd;

			responseReset();

			return Positive;
		}

		if (ch < IdContentProc || ch > IdContentCmd)
			break;

		responseReset(ch);
		mContentIgnore = false;

		if (ch != IdContentProc)
		{
			mStateSwt = StSwtDataReceive;
			break;
		}

		// Process Tree filter

		if (diffMs > env.rateRefreshMs)
		{
			mLastProcTreeRcvdMs = curTimeMs;
			mStateSwt = StSwtDataReceive;
			break;
		}

		responseReset();
		mContentIgnore = true;

		mStateSwt = StSwtDataReceive;

		break;
	case StSwtDataReceive:

		if (ch == IdContentCut)
		{
			mStateSwt = StSwtContentRcvWait;
			break;
		}

		if (ch == IdContentEnd)
		{
			if (!mContentIgnore)
				fragmentFinish();

			mStateSwt = StSwtContentRcvWait;
			return Positive;
		}

		if (!ch)
			break;

		if (isprint(ch) ||
			ch == cKeyEscape ||
			ch == cKeyTab ||
			ch == cKeyCr ||
			ch == cKeyLf)
		{
			if (!mContentIgnore)
				fragmentAppend(ch);
			break;
		}

		if (!mContentIgnore)
			fragmentDelete();

		mStateSwt = StSwtContentRcvWait;
		return SwtErrRcvProtocol;

		break;
	default:
		break;
	}

	return Pending;
}

Success SingleWireScheduling::shutdown()
{

	if (mRefUart != RefDeviceUartInvalid)
	{
		devUartDeInit(mRefUart);
		refUart = mRefUart;
	}

	return Positive;
}

void SingleWireScheduling::processInfo(char *pBuf, char *pBufEnd)
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
	dInfo("Bytes received\t\t%zu\n", mCntBytesRcvd);
	dInfo("IdContentNone received\t%zu\n", mCntContentNoneRcvd);
#if 0
	fragmentsPrint(pBuf, pBufEnd);
#endif
#if 0
	queuesCmdPrint(pBuf, pBufEnd);
#endif
}

/* static functions */

