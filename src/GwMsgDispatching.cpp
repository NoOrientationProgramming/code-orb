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
#include "ThreadPooling.h"
#include "LibTime.h"

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

#define dColorGreen		"\033[38;5;46m"
#define dColorOrange	"\033[38;5;220m"
#define dColorRed		"\033[38;5;196m"
#define dColorGrey		"\033[38;5;240m"
#define dColorClear		"\033[0m"

#define dCursorHide		"\033[?25l"
#define dCursorShow		"\033[?25h"
#define dScreenClear	"\033[2J\033[H"

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
	, mpLstCmdAuto(NULL)
	, mpSched(NULL)
	, mpGather(NULL)
	, mCursorVisible(true)
	, mDevUartIsOnline(true)
	, mTargetIsOnline(false)
	, mListPeers()
	, mHdrDate("")
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

		mpSched = SingleWireScheduling::create();
		if (!mpSched)
			return procErrLog(-1, "could not create process");

		//mpSched->procTreeDisplaySet(false);
#if 1
		start(mpSched);
#else
		start(mpSched, DrivenByNewInternalDriver);
#endif
		userInfLog("%s\n", dVersion);
		userInfLog("Using device: %s\n", env.deviceUart.c_str());

		userInfLog("Listening on: %u, %u, %u\n",
								mPortStart,
								(uint16_t)(mPortStart + 2),
								(uint16_t)(mPortStart + 4));

		if (env.ctrlManual)
			userInfLog("Manual control enabled\n");

		if (!env.verbosity)
			cursorShow(false);

		stateOnlineCheckAndPrint();

		mState = StTargetOffline;

		break;
	case StTargetOffline:

		stateOnlineCheckAndPrint();
		peerListUpdate();
		commandAutoProcess();
		contentDistribute();

		if (!mTargetIsOnline)
			break;

		//procWrnLog("target is online");

		if (mpGather)
		{
			mState = StTargetOnline;
			break;
		}

		mpGather = InfoGathering::create();
		if (!mpGather)
		{
			procWrnLog("could not create process");

			mState = StTargetOnline;
			break;
		}

		//mpGather->procTreeDisplaySet(false);
		start(mpGather, DrivenByExternalDriver);
		ThreadPooling::procAdd(mpGather);

		mState = StTargetOnline;

		break;
	case StTargetOnline:

		stateOnlineCheckAndPrint();
		peerListUpdate();
		commandAutoProcess();
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
#if 1
		if (success != Positive)
			procWrnLog("could not gather information");
#endif
		if (success == Positive)
			RemoteCommanding::listCommandsUpdate(mpGather->mEntriesReceived);

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
	if (!mCursorVisible)
		cursorShow();

	return Positive;
}

void GwMsgDispatching::stateOnlineCheckAndPrint()
{
	if (mpSched->mDevUartIsOnline == mDevUartIsOnline &&
			mpSched->mTargetIsOnline == mTargetIsOnline)
		return;

	mDevUartIsOnline = mpSched->mDevUartIsOnline;
	mTargetIsOnline = mpSched->mTargetIsOnline;

	if (env.verbosity)
		return;

	userInfLog("\rUART [ ");
	onlinePrint(mDevUartIsOnline);
	userInfLog(" ] - Target [ ");
	onlinePrint(mTargetIsOnline);
	userInfLog(" ]  ");

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
	userInfLog("%s", pColor);
#endif

	userInfLog("%sline", pContent);

#ifdef _WIN32
	SetConsoleTextAttribute(hConsole, 7);
#else
	userInfLog("%s", dColorClear);
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

	mpLstCmdAuto = TcpListening::create();
	if (!mpLstCmdAuto)
		return procErrLog(-1, "could not create process");

	mpLstCmdAuto->portSet(mPortStart + 6, mListenLocal);
	mpLstCmdAuto->maxConnSet(4);

	mpLstCmdAuto->procTreeDisplaySet(false);
	start(mpLstCmdAuto);

	// thread pool
	ThreadPooling *pPool;

	pPool = ThreadPooling::create();
	if (!pPool)
		return procErrLog(-1, "could not create process");

	pPool->workerCntSet(3);

	pPool->procTreeDisplaySet(false);
	start(pPool);

	list<string> mEntriesDummy;
	RemoteCommanding::listCommandsUpdate(mEntriesDummy);

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

void GwMsgDispatching::commandAutoProcess()
{
	PipeEntry<SOCKET> peerFd;
	RemoteCommanding *pCmd;

	while (1)
	{
		if (mpLstCmdAuto->ppPeerFd.get(peerFd) < 1)
			break;

		pCmd = RemoteCommanding::create(peerFd.particle);
		if (!pCmd)
		{
			procErrLog(-1, "could not create process");
			continue;
		}

		pCmd->mpTargetIsOnline = &mTargetIsOnline;
		pCmd->modeAutoSet();

		pCmd->procTreeDisplaySet(false);
		whenFinishedRepel(start(pCmd));
	}
}

void GwMsgDispatching::contentDistribute()
{
	string msg;

	// proc tree
	if (mpSched->contentProcChanged())
	{
		const string &str = mpSched->mContentProc;

		mHdrDate = nowToStr("%Y-%m-%d  %H:%M:%S");

		msgProcHdr(msg, str.size());
		msg += str;

		contentSend(msg, RemotePeerProc);
	}

	// log
	PipeEntry<string> entryLog;

	while (1)
	{
		if (mpSched->ppEntriesLog.get(entryLog) < 1)
			break;

		msg += dColorGrey;
		msg += nowToStr("%Y-%m-%d  %H:%M:%S  ");
		msg += dColorClear;

		msg += entryLog.particle;
		msg += "\r\n";

		contentSend(msg, RemotePeerLog);
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
#if 1
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
#else
	return true;
#endif
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

			pCmd->mpTargetIsOnline = &mTargetIsOnline;

			pCmd->procTreeDisplaySet(false);
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
			const string &str = mpSched->mContentProc;
			string msg;

			msgProcHdr(msg, str.size());
			msg += str;

			pTrans->send(msg.data(), msg.size());
		}

		procDbgLog("adding %s peer. process: %p", pTypeDesc, pTrans);

		peer.type = peerType;
		peer.typeDesc = pTypeDesc;
		peer.pProc = pTrans;

		mListPeers.push_back(peer);
	}
}

void GwMsgDispatching::msgProcHdr(string &msg, size_t sz)
{
	msg = dScreenClear;

	msg += dColorGrey;
	msg += "{CodeOrb -- ";
	msg += mHdrDate;
	msg += " -- Size: ";
	msg += to_string(sz);
	msg += "}";
	msg += dColorClear;
	msg += "\r\n\r\n";
}

void GwMsgDispatching::cursorShow(bool val)
{
#ifdef _WIN32
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO infoCursor;

	GetConsoleCursorInfo(hOut, &infoCursor);

	infoCursor.bVisible = val ? TRUE : FALSE;
	SetConsoleCursorInfo(hOut, &infoCursor);
#else
	userInfLog(val ? dCursorShow : dCursorHide);
	fflush(stdout);
#endif
	mCursorVisible = val;
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

