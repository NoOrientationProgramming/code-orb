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

#ifndef SINGLE_WIRE_SCHEDULING_H
#define SINGLE_WIRE_SCHEDULING_H

#include <string>
#include <map>

#include "Processing.h"
#include "Pipe.h"
#include "SingleWire.h"
#include "LibUart.h"

enum PrioCmd
{
	PrioSysHigh = 0,
	PrioUser,
	PrioSysLow,
};

struct SingleWireResponse
{
	uint8_t idContent;
	std::string content;
	bool unsolicited;
};

typedef ssize_t (*FuncUartSend)(RefDeviceUart refUart, const void *pBuf, size_t lenReq);

struct CommandReqResp
{
	CommandReqResp(std::string cmd, uint32_t id, uint32_t start)
		: str(std::move(cmd))
		, idReq(id)
		, startMs(start)
	{}

	std::string str;
	uint32_t idReq;
	uint32_t startMs;
};

const uint32_t cTimeoutCommandResponseMs = 1500;

class SingleWireScheduling : public Processing
{

public:

	static SingleWireScheduling *create()
	{
		return new dNoThrow SingleWireScheduling;
	}

	// input
	static uint8_t monitoring;

	// output
	bool mDevUartIsOnline;
	bool mTargetIsOnline;

	bool contentProcChanged();
	std::string mContentProc;

	Pipe<std::string> ppEntriesLog;

	static bool commandSend(const std::string &cmd,
					uint32_t &idReq,
					PrioCmd prio = PrioUser);
	static bool commandResponseGet(uint32_t idReq, std::string &resp);

protected:

	SingleWireScheduling();
	virtual ~SingleWireScheduling() {}

private:

	SingleWireScheduling(const SingleWireScheduling &) = delete;
	SingleWireScheduling &operator=(const SingleWireScheduling &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);
	void fragmentsPrint(char *pBuf, char *pBufEnd);
	void queuesCmdPrint(char *pBuf, char *pBufEnd);
	void requestsCmdPrint(char * &pBuf, char *pBufEnd);
	void responsesCmdPrint(char * &pBuf, char *pBufEnd);

	Success cmdQueueConsume();
	void cmdResponseReceived(const std::string &resp);
	void cmdResponsesClear(uint32_t curTimeMs);
	bool cmdSend(const std::string &cmd);
	bool dataRequest();
	Success contentDistribute();
	Success dataReceive();
	Success byteProcess(uint8_t ch, uint32_t curTimeMs);
	void targetOnlineSet(bool online = true);
	void responseReset(uint8_t idContent = IdContentTaToScNone);
	void fragmentAppend(uint8_t ch);
	void fragmentFinish();
	void fragmentDelete();

	/* member variables */
	uint32_t mStateSwt;
	uint32_t mStartMs;
	RefDeviceUart mRefUart;
	char mBufRcv[13];
	char *mpBuf;
	ssize_t mLenDone;
	std::map<int, std::string> mFragments;
	SingleWireResponse mResp;
	bool mContentProcChanged;
	size_t mCntBytesRcvd;
	size_t mCntContentNoneRcvd;
	uint32_t mLastProcTreeRcvdMs;
	bool mTargetIsOnlineOld;
	bool mTargetIsOfflineMarked;
	bool mContentIgnore;
	bool mCmdExpected;
	uint8_t mByteLast;
	std::list<CommandReqResp> *mpListCmdCurrent;
	uint8_t mCntDelayPrioLow;
	uint8_t mCntRerequest;

	/* static functions */

	// Manual Control
	static void commandsRegister();
	static void cmdMonitoringToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdCtrlManualToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdDataUartSend(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdStrUartSend(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdDataUartRead(char *pArgs, char *pBuf, char *pBufEnd);

	static void cmdModeUartVirtSet(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdMountedUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdTimeoutUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdDataUartRcv(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdStrUartRcv(char *pArgs, char *pBuf, char *pBufEnd);

	static void dataUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend);
	static void strUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend);

	// Commands
	static void cmdCommandSend(char *pArgs, char *pBuf, char *pBufEnd);

	/* static variables */
	static uint8_t uartVirtualTimeout;
	static RefDeviceUart refUart;
	static std::list<CommandReqResp> requestsCmd[3];
	static std::list<CommandReqResp> responsesCmd;
	static uint32_t idReqCmdNext;
	static std::mutex mtxRequests;
	static std::mutex mtxResponses;

	/* constants */
	static const size_t cSizeFragmentMax;
	static const uint32_t cTimeoutRespMs;
	static const uint32_t cTimeoutDequeueMs;

};

#endif

