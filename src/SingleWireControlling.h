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

struct SingleWireResponse
{
	uint8_t idContent;
	std::string content;
};

enum SwtErrRcv
{
	SwtErrRcvNoUart = -3,
	SwtErrRcvNoTarget,
	SwtErrRcvProtocol,
};

typedef ssize_t (*FuncUartSend)(RefDeviceUart refUart, const void *pBuf, size_t lenReq);

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

	void cmdSend(const std::string &cmd);
	void dataRequest();
	Success dataReceive();
	Success byteProcess(uint8_t ch);
	void fragmentAppend(uint8_t idContent, uint8_t ch);
	void fragmentFinish(uint8_t idContent);
	void fragmentDelete(uint8_t idContent);

	/* member variables */
	uint32_t mStateSwt;
	uint32_t mStartMs;
	RefDeviceUart mRefUart;
	char mBufRcv[13];
	char *mpBuf;
	ssize_t mLenDone;
	std::map<int, std::string> mFragments;
	uint8_t mContentCurrent;
	SingleWireResponse mResp;
	bool mContentProcChanged;
	bool mProcBytesSkip;

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

	/* static variables */

	/* constants */

};

#endif

