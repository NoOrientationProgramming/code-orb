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

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StNop) \

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

#define dOnline  dColorGreen  "Online" dColorClear
#define dOffline dColorOrange "Offline" dColorClear

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
	//Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		ok = listenersStart();
		if (!ok)
			return procErrLog(-1, "could not start listeners");

		mpCtrl = SingleWireControlling::create();
		if (!mpCtrl)
			return procErrLog(-1, "could not create process");
#if 1
		start(mpCtrl);
#else
		start(mpCtrl, DrivenByNewInternalDriver);
#endif
		if (!env.verbosity)
		{
			fprintf(stdout, "\033[?25l");
			fflush(stdout);
			mCursorHidden = true;

			stateOnlineCheckAndPrint();
		}

		mState = StMain;

		break;
	case StMain:

		if (!env.verbosity)
			stateOnlineCheckAndPrint();

		peerListUpdate();
		contentDistribute();

		break;
	case StNop:

		break;
	default:
		break;
	}

	return Pending;
}

Success GwMsgDispatching::shutdown()
{
	if (mCursorHidden)
	{
		fprintf(stdout, "\033[?25h");
		fflush(stdout);
		mCursorHidden = false;
	}

	return Positive;
}

void GwMsgDispatching::stateOnlineCheckAndPrint()
{
	if (mpCtrl->mDevUartIsOnline == mDevUartIsOnline &&
			mpCtrl->mTargetIsOnline == mTargetIsOnline)
		return;

	mDevUartIsOnline = mpCtrl->mDevUartIsOnline;
	mTargetIsOnline = mpCtrl->mTargetIsOnline;

	fprintf(stdout, "\rUART [ %s ] - Target [ %s ]  ",
		mDevUartIsOnline ? dOnline : dOffline,
		mTargetIsOnline ? dOnline : dOffline);

	fflush(stdout);
}

bool GwMsgDispatching::listenersStart()
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
		string str("\033[2J\033[H");
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
	Processing *pProc = NULL;
	struct RemoteDebuggingPeer peer;

	while (1)
	{
		if (pListener->ppPeerFd.get(peerFd) < 1)
			break;

		if (peerType == RemotePeerCmd)
		{
			pProc = RemoteCommanding::create(peerFd.particle);
			if (!pProc)
			{
				procErrLog(-1, "could not create process");
				continue;
			}

			whenFinishedRepel(start(pProc));

			continue;
		}

		pProc = TcpTransfering::create(peerFd.particle);
		if (!pProc)
		{
			procErrLog(-1, "could not create process");
			continue;
		}

		pProc->procTreeDisplaySet(false);
		start(pProc);

		procDbgLog("adding %s peer. process: %p", pTypeDesc, pProc);

		peer.type = peerType;
		peer.typeDesc = pTypeDesc;
		peer.pProc = pProc;

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

