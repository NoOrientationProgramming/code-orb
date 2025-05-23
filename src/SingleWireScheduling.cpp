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
		gen(StMain) \
		gen(StDataRequest) \
		gen(StTargetRespWait) \
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

#define dDebugCommand	0

const size_t SingleWireScheduling::cSizeFragmentMax = 4095;
const uint32_t SingleWireScheduling::cTimeoutRespMs = 330;
const uint32_t SingleWireScheduling::cTimeoutDequeueMs = 5500;

uint8_t SingleWireScheduling::monitoring = 1;
uint8_t SingleWireScheduling::uartVirtualTimeout = 0;
RefDeviceUart SingleWireScheduling::refUart;

list<CommandReqResp> SingleWireScheduling::requestsCmd[3];
list<CommandReqResp> SingleWireScheduling::responsesCmd;
uint32_t SingleWireScheduling::idReqCmdNext = 0;
mutex SingleWireScheduling::mtxRequests;
mutex SingleWireScheduling::mtxResponses;

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
	, mCmdExpected(false)
	, mByteLast(0)
	, mpListCmdCurrent(NULL)
	, mCntDelayPrioLow(0)
	, mCntRerequest(0)
{
	responseReset();
	mBufRcv[0] = 0;

	mState = StStart;
}

/* member functions */

Success SingleWireScheduling::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
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
		{
			mState = StUartInit;
			break;
		}

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

		mFragments.clear();
		mStateSwt = StSwtContentRcvWait;

		mStartMs = curTimeMs;
		mState = StTargetInitDoneWait;

		break;
	case StTargetInitDoneWait:

		if ((!uartVirtual && diffMs > cTimeoutRespMs) ||
			(uartVirtual && uartVirtualTimeout))
		{
			mState = StTargetInit;
			break;
		}

		success = dataReceive();
		if (success == Pending)
			break;

		if (success != Positive)
		{
			mState = StUartInit;
			break;
		}
#if 0
		procWrnLog("content received: 0x%02X > '%s'",
						mResp.idContent,
						mResp.content.c_str());
#endif
		if (mResp.idContent != IdContentCmd)
		{
			responseReset();
			break;
		}

		if (mResp.content != "Debug mode 1")
		{
			responseReset();
			break;
		}

		responseReset();

		targetOnlineSet();

		{
			Guard lock(mtxRequests);
			for (size_t i = 0; i < sizeof(requestsCmd) / sizeof(*requestsCmd); ++i)
				requestsCmd[i].clear();
		}
		{
			Guard lock(mtxResponses);
			responsesCmd.clear();
		}

		mpListCmdCurrent = NULL;

		mState = StMain;

		break;
	case StMain:

		// internal

		if (env.ctrlManual)
		{
			mState = StCtrlManual;
			break;
		}

		cmdResponsesClear(curTimeMs);

		// communication to target

		success = contentDistribute();
		if (success != Pending && success != Positive)
		{
			mState = StUartInit;
			break;
		}

		if (success == Positive)
			responseReset();

		success = cmdQueueConsume();
		if (success == Positive)
		{
			mCmdExpected = true;
			mCntRerequest = 0;

			mStartMs = curTimeMs;
			mState = StDataRequest;
			break;
		}

		if (success != Pending)
		{
			mState = StUartInit;
			break;
		}

		if (monitoring)
		{
			mCmdExpected = false;

			mStartMs = curTimeMs;
			mState = StDataRequest;
			break;
		}

		break;
	case StDataRequest:

		ok = dataRequest();
		if (!ok)
		{
			mState = StUartInit;
			break;
		}

		mState = StTargetRespWait;

		break;
	case StTargetRespWait:

		if ((!uartVirtual && diffMs > cTimeoutRespMs) ||
			(uartVirtual && uartVirtualTimeout))
		{
			//procErrLog(-1, "response timeout");
			mState = StTargetInit;
			break;
		}

		success = contentDistribute();
		if (success == Pending)
			break;

		if (success != Positive)
		{
			mState = StUartInit;
			break;
		}
#if 0
		procWrnLog("content received: 0x%02X%s",
							mResp.idContent,
							mResp.unsolicited ? " (unsolicited)" : "");
#endif
		if (mCmdExpected && mResp.idContent != IdContentCmd)
		{
			//procErrLog(-1, "re-request");

			responseReset();

			++mCntRerequest;
			if (mCntRerequest < 4)
			{
				mState = StDataRequest;
				break;
			}

			mpListCmdCurrent->pop_front();
			mpListCmdCurrent = NULL;

			mState = StMain;
			break;
		}
		mCmdExpected = false;

		responseReset();

		mState = StMain;

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

Success SingleWireScheduling::cmdQueueConsume()
{
	if (mpListCmdCurrent)
		return Pending;

	Guard lock(mtxRequests);

	if (requestsCmd[PrioUser].size())
		mpListCmdCurrent = &requestsCmd[PrioUser];
	else
	if (requestsCmd[PrioSysLow].size())
	{
		if (monitoring && mCntDelayPrioLow)
			return Pending;

		mpListCmdCurrent = &requestsCmd[PrioSysLow];
		mCntDelayPrioLow = 4;
	}

	if (!mpListCmdCurrent)
		return Pending;

	const CommandReqResp *pReq = &mpListCmdCurrent->front();
	bool ok;

	ok = cmdSend(pReq->str);
	if (!ok)
	{
		mpListCmdCurrent = NULL;
		return -1;
	}

	return Positive;
}

void SingleWireScheduling::cmdResponseReceived(const string &resp)
{
	if (!mpListCmdCurrent)
		return;
#if 0
	procWrnLog("command response received: %s",
				resp.c_str());
#endif
	uint32_t idReq;

	{
		Guard lock(mtxRequests);

		idReq = mpListCmdCurrent->front().idReq;
		mpListCmdCurrent->pop_front();

		mpListCmdCurrent = NULL;
	}

	{
		Guard lock(mtxResponses);

		responsesCmd.emplace_back(resp, idReq, millis());
	}
}

void SingleWireScheduling::cmdResponsesClear(uint32_t curTimeMs)
{
	Guard lock(mtxResponses);

	list<CommandReqResp>::iterator iter;
	uint32_t diffMs;

	iter = responsesCmd.begin();
	while (iter != responsesCmd.end())
	{
		diffMs = curTimeMs - iter->startMs;

		if (diffMs < cTimeoutDequeueMs)
		{
			++iter;
			continue;
		}
#if 0
		procWrnLog("dequeue timeout for: %u",
					iter->idReq);
#endif
		iter = responsesCmd.erase(iter);
	}
}

bool SingleWireScheduling::cmdSend(const string &cmd)
{
	bool failed = false;

	failed |= uartSend(mRefUart, FlowSchedToTarget) < 0;
	failed |= uartSend(mRefUart, IdContentScToTaCmd) < 0;
	failed |= uartSend(mRefUart, cmd.data(), cmd.size()) < 0;
	failed |= uartSend(mRefUart, 0x00) < 0;
	failed |= uartSend(mRefUart, IdContentEnd) < 0;

	if (failed)
		return false;
#if dDebugCommand
	procWrnLog("cmd sent: %s", cmd.c_str());
#endif
	return true;
}

bool SingleWireScheduling::dataRequest()
{
	ssize_t lenWritten;

	lenWritten = uartSend(mRefUart, FlowTargetToSched);
	if (lenWritten < 0)
		return false;

	//procWrnLog("data requested");

	if (mCntDelayPrioLow)
	{
		--mCntDelayPrioLow;
		//procWrnLog("low prio delay: %u", mCntDelayPrioLow);
	}

	return true;
}

Success SingleWireScheduling::contentDistribute()
{
	Success success;

	while (1)
	{
		success = dataReceive();
		if (success == Pending)
			break;

		if (success != Positive)
			return success;
#if dDebugCommand
		if (mResp.idContent != IdContentNone)
		{
			procWrnLog("content received: 0x%02X%s",
								mResp.idContent,
								mResp.unsolicited ? " (unsolicited)" : "");
#if 1
			if (mResp.idContent != IdContentProc)
#endif
				printf("%s\n", mResp.content.c_str());
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

		if (!mResp.unsolicited)
			return Positive;

		mStartMs = millis();
		responseReset();
	}

	return Pending;
}

Success SingleWireScheduling::dataReceive()
{
	uint32_t curTimeMs = millis();
	Success success;

	while (mLenDone > 0)
	{
		success = byteProcess((uint8_t)*mpBuf, curTimeMs);
		mByteLast = *mpBuf;

		++mpBuf;
		--mLenDone;

		if (success == Positive)
			return Positive;
	}

	mLenDone = uartRead(mRefUart, mBufRcv, sizeof(mBufRcv));
	if (!mLenDone)
		return Pending;

	if (mLenDone < 0)
	{
		mFragments.clear();
		mStateSwt = StSwtContentRcvWait;

		return -1;
	}

	mpBuf = mBufRcv;

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

		if (mByteLast == FlowTargetToSched)
		{
			//procWrnLog("got unsolicited. type: 0x%02X", mResp.idContent);
			mResp.unsolicited = true;
		}

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

		if (mContentIgnore)
			break;

		fragmentAppend(ch);

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
	dInfo("Monitoring\t\t%sabled\n", monitoring ? "En" : "Dis");
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

