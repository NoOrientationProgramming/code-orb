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

#include "GwMsgDispatching.h"
#if 0
#include "ColorTesting.h"
#include "ThreadPooling.h"
#endif

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StTargetOffline) \
		gen(StTargetOnline) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

#define dColorGreen "\033[38;5;46m"
#define dColorOrange "\033[38;5;220m"
#define dColorRed "\033[38;5;196m"
#define dColorClear "\033[0m"

#define dCursorHide "\033[?25l"
#define dCursorShow "\033[?25h"
#define dScreenClear "\033[2J\033[H"

typedef list<struct RemoteDebuggingPeer>::iterator PeerIter;

const string cSeqCtrlC = "\xff\xf4\xff\xfd\x06";
const size_t cLenSeqCtrlC = cSeqCtrlC.size();

GwMsgDispatching::GwMsgDispatching()
	: Processing("GwMsgDispatching")
	//, mStartMs(0)
	, mPortStart(3000)
	, mListenLocal(false)
	, mpLstProc(NULL)
	, mpLstLog(NULL)
	, mpLstCmd(NULL)
	, mpCtrl(NULL)
	, mpGather(NULL)
	, mCursorHidden(false)
	, mDevUartIsOnline(true)
	, mTargetIsOnline(false)
	, mListPeers()
{
	mState = StStart;
}

/* member functions */

Success GwMsgDispatching::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		mPortStart = env.startPortsTarget;

		ok = servicesStart();
		if (!ok)
			return procErrLog(-1, "could not start services");

		mpCtrl = SingleWireControlling::create();
		if (!mpCtrl)
			return procErrLog(-1, "could not create process");
#if 1
		start(mpCtrl);
#else
		start(mpCtrl, DrivenByNewInternalDriver);
#endif
		fprintf(stdout, "CodeOrb-25.04-1\n");
		fprintf(stdout, "Using device: %s\n", env.deviceUart.c_str());

		fprintf(stdout, "Listening on: %u, %u, %u\n",
								mPortStart,
								mPortStart + (uint16_t)2,
								mPortStart + (uint16_t)4);

		if (env.ctrlManual)
			fprintf(stdout, "Manual control enabled\n");

#ifndef _WIN32
		if (!env.verbosity)
		{
			fprintf(stdout, dCursorHide);
			fflush(stdout);
			mCursorHidden = true;
		}
#endif
		stateOnlineCheckAndPrint();
#if 0
		start(ColorTesting::create(), DrivenByParent);
		start(ColorTesting::create(), DrivenByNewInternalDriver);
#if 1
		ThreadPooling::procAdd(start(ColorTesting::create(), DrivenByExternalDriver));
#else
		start(ColorTesting::create(), DrivenByExternalDriver);
#endif
#endif
		mState = StTargetOffline;

		break;
	case StTargetOffline:

		stateOnlineCheckAndPrint();
		peerListUpdate();
		contentDistribute();

		if (!mTargetIsOnline)
			break;

		if (!mpGather)
			mpGather = InfoGathering::create();

		if (!mpGather)
			procWrnLog("could not create process");
		else
			start(mpGather);

		mState = StTargetOnline;

		break;
	case StTargetOnline:

		stateOnlineCheckAndPrint();
		peerListUpdate();
		contentDistribute();

		if (!mTargetIsOnline)
		{
			mState = StTargetOffline;
			break;
		}

		if (!mpGather)
			break;

		success = mpGather->success();
		if (success == Pending)
			break;

		if (success == Positive)
		{
			procWrnLog("gathered information");
		}

		repel(mpGather);
		mpGather = NULL;

		break;
	default:
		break;
	}

	return Pending;
}

Success GwMsgDispatching::shutdown()
{
#ifdef _WIN32
	fprintf(stdout, "\r\n");
	fflush(stdout);
#else
	if (mCursorHidden)
	{
		fprintf(stdout, dCursorShow);
		fflush(stdout);
		mCursorHidden = false;
	}
#endif

	return Positive;
}

void GwMsgDispatching::stateOnlineCheckAndPrint()
{
	if (mpCtrl->mDevUartIsOnline == mDevUartIsOnline &&
			mpCtrl->mTargetIsOnline == mTargetIsOnline)
		return;

	mDevUartIsOnline = mpCtrl->mDevUartIsOnline;
	mTargetIsOnline = mpCtrl->mTargetIsOnline;

	if (env.verbosity)
		return;

	fprintf(stdout, "\rUART [ ");
	onlinePrint(mDevUartIsOnline);
	fprintf(stdout, " ] - Target [ ");
	onlinePrint(mTargetIsOnline);
	fprintf(stdout, " ]  ");

	fflush(stdout);
}

void GwMsgDispatching::onlinePrint(bool online)
{
	const char *pContent = online ? "On" : "Off";
#ifdef _WIN32
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD idColor = online ? 2 : 6;
#else
	const char *pColor = online ? dColorGreen : dColorOrange;
#endif

#ifdef _WIN32
	SetConsoleTextAttribute(hConsole, idColor);
#else
	fprintf(stdout, "%s", pColor);
#endif

	fprintf(stdout, "%sline", pContent);

#ifdef _WIN32
	SetConsoleTextAttribute(hConsole, 7);
#else
	fprintf(stdout, "%s", dColorClear);
#endif
}

bool GwMsgDispatching::servicesStart()
{
	// proc tree
	mpLstProc = TcpListening::create();
	if (!mpLstProc)
		return procErrLog(-1, "could not create process");

	mpLstProc->portSet(mPortStart, mListenLocal);

	mpLstProc->procTreeDisplaySet(false);
	start(mpLstProc);
#if CONFIG_PROC_HAVE_LOG
	// log
	mpLstLog = TcpListening::create();
	if (!mpLstLog)
		return procErrLog(-1, "could not create process");

	mpLstLog->portSet(mPortStart + 2, mListenLocal);

	mpLstLog->procTreeDisplaySet(false);
	start(mpLstLog);
#endif
	// command
	mpLstCmd = TcpListening::create();
	if (!mpLstCmd)
		return procErrLog(-1, "could not create process");

	mpLstCmd->portSet(mPortStart + 4, mListenLocal);
	mpLstCmd->maxConnSet(4);

	mpLstCmd->procTreeDisplaySet(false);
	start(mpLstCmd);
#if 0
	ThreadPooling *pPool;

	pPool = ThreadPooling::create();
	if (!pPool)
		return procErrLog(-1, "could not create process");

	pPool->procTreeDisplaySet(false);
	start(pPool);
#endif
	return true;
}

void GwMsgDispatching::peerListUpdate()
{
	peerCheck();
	peerAdd(mpLstProc, RemotePeerProc, "process tree");
#if CONFIG_PROC_HAVE_LOG
	peerAdd(mpLstLog, RemotePeerLog, "log");
#endif
	peerAdd(mpLstCmd, RemotePeerCmd, "command");
}

void GwMsgDispatching::contentDistribute()
{
	// proc tree
	if (mpCtrl->contentProcChanged())
	{
		string str(dScreenClear);
		str += mpCtrl->mContentProc;
		contentSend(str, RemotePeerProc);
	}

	// log
	PipeEntry<string> entryLog;

	while (1)
	{
		if (mpCtrl->ppEntriesLog.get(entryLog) < 1)
			break;

		contentSend(entryLog.particle, RemotePeerLog);
	}
}

void GwMsgDispatching::contentSend(const string &str, RemotePeerType typePeer)
{
	PeerIter iter;
	TcpTransfering *pTrans;

	iter = mListPeers.begin();
	for (; iter != mListPeers.end(); ++iter)
	{
		if (iter->type != typePeer)
			continue;

		pTrans = (TcpTransfering *)iter->pProc;

		pTrans->send(str.data(), str.size());
	}
}

bool GwMsgDispatching::disconnectRequestedCheck(TcpTransfering *pTrans)
{
	if (!pTrans)
		return false;

	char buf[31];
	ssize_t lenReq, lenPlanned, lenDone;

	lenReq = sizeof(buf) - 1;
	lenPlanned = lenReq;

	buf[0] = 0;
	buf[lenReq] = 0;

	lenDone = pTrans->read(buf, lenPlanned);
	if (!lenDone)
		return false;

	if (lenDone < 0)
		return true;

	buf[lenDone] = 0;

	if ((buf[0] == 0x03) || (buf[0] == 0x04))
	{
		procDbgLog("end of transmission");
		return true;
	}

	if (!strncmp(buf, cSeqCtrlC.c_str(), cLenSeqCtrlC))
	{
		procDbgLog("transmission cancelled");
		return true;
	}

	return false;
}

void GwMsgDispatching::peerCheck()
{
	PeerIter iter;
	struct RemoteDebuggingPeer peer;
	Processing *pProc;
	bool disconnectReq, removeReq;

	iter = mListPeers.begin();
	while (iter != mListPeers.end())
	{
		peer = *iter;
		pProc = peer.pProc;

		if (peer.type == RemotePeerProc)
			disconnectReq = disconnectRequestedCheck((TcpTransfering *)pProc);
		else
		if (peer.type == RemotePeerLog)
		{
			TcpTransfering *pTrans = (TcpTransfering *)pProc;
			disconnectReq = disconnectRequestedCheck(pTrans);
		}
		else
			disconnectReq = false;

		removeReq = (pProc->success() != Pending) || disconnectReq;
		if (!removeReq)
		{
			++iter;
			continue;
		}

		procDbgLog("removing %s peer. process: %p", peer.typeDesc.c_str(), pProc);
		repel(pProc);

		iter = mListPeers.erase(iter);
	}
}

void GwMsgDispatching::peerAdd(TcpListening *pListener, enum RemotePeerType peerType, const char *pTypeDesc)
{
	PipeEntry<SOCKET> peerFd;
	TcpTransfering *pTrans;
	struct RemoteDebuggingPeer peer;

	while (1)
	{
		if (pListener->ppPeerFd.get(peerFd) < 1)
			break;

		if (peerType == RemotePeerCmd)
		{
			RemoteCommanding *pCmd;

			pCmd = RemoteCommanding::create(peerFd.particle);
			if (!pCmd)
			{
				procErrLog(-1, "could not create process");
				continue;
			}

			whenFinishedRepel(start(pCmd));

			continue;
		}

		pTrans = TcpTransfering::create(peerFd.particle);
		if (!pTrans)
		{
			procErrLog(-1, "could not create process");
			continue;
		}

		pTrans->procTreeDisplaySet(false);
		start(pTrans);

		if (peerType == RemotePeerProc)
		{
			string str(dScreenClear);
			str += mpCtrl->mContentProc;
			pTrans->send(str.data(), str.size());
		}

		procDbgLog("adding %s peer. process: %p", pTypeDesc, pTrans);

		peer.type = peerType;
		peer.typeDesc = pTypeDesc;
		peer.pProc = pTrans;

		mListPeers.push_back(peer);
	}
}

void GwMsgDispatching::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	dInfo("Number of peers\t\t%zu\n", mListPeers.size());
	dInfo("Refresh rate\t\t%u [ms]\n", env.rateRefreshMs);
}

/* static functions */

