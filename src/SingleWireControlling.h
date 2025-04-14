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

#ifndef SINGLE_WIRE_CONTROLLING_H
#define SINGLE_WIRE_CONTROLLING_H

#include <string>
#include <map>

#include "Processing.h"
#include "Pipe.h"
#include "LibUart.h"

enum SwtContentId
{
	IdContentNone = 0x15,
	IdContentProc = 0x11,
	IdContentLog,
	IdContentCmd,
};

enum SwtErrRcv
{
	SwtErrRcvNoUart = -3,
	SwtErrRcvNoTarget,
	SwtErrRcvProtocol,
};

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
};

typedef ssize_t (*FuncUartSend)(RefDeviceUart refUart, const void *pBuf, size_t lenReq);

struct EntryHelp
{
	std::string id;
	std::string shortcut;
	std::string desc;
	std::string group;
};

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

class SingleWireControlling : public Processing
{

public:

	static SingleWireControlling *create()
	{
		return new dNoThrow SingleWireControlling;
	}

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

	SingleWireControlling();
	virtual ~SingleWireControlling() {}

private:

	SingleWireControlling(const SingleWireControlling &) = delete;
	SingleWireControlling &operator=(const SingleWireControlling &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	bool cmdQueueCheck();
	void cmdResponseReceived(const std::string &resp);
	void cmdResponsesClear();
	void cmdSend(const std::string &cmd);
	void dataRequest();
	Success dataReceive();
	Success byteProcess(uint8_t ch, uint32_t curTimeMs);
	void fragmentAppend(uint8_t ch);
	void fragmentFinish();
	void fragmentDelete();

	void targetOnlineSet(bool online = true);
	bool entryHelpAdd(const std::string &str);

	void responseReset(uint8_t idContent = IdContentNone);

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
	bool mHelpSynced;
	std::string mEntryHelpFirst;
	size_t mCntHelp;
	uint32_t mLastProcTreeRcvdMs;
	bool mTargetIsOnlineOld;
	bool mTargetIsOfflineMarked;
	bool mContentIgnore;
	std::list<CommandReqResp> *mpListCmdCurrent;
	uint8_t mCntDelayPrioLow;

	/* static functions */

	// Manual Control
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
	static std::list<CommandReqResp> requestsCmd[3];
	static std::list<CommandReqResp> responsesCmd;
	static uint32_t idReqCmdNext;

	/* constants */

};

#endif

