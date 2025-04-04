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
#include "LibUart.h"

struct SingleWireResponse
{
	char idContent;
	std::string content;
};

enum SwtErrRcv
{
	SwtErrRcvNoUart = -3,
	SwtErrRcvNoTarget,
	SwtErrRcvProtocol,
};

class SingleWireControlling : public Processing
{

public:

	static SingleWireControlling *create()
	{
		return new dNoThrow SingleWireControlling;
	}

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
	void processInfo(char *pBuf, char *pBufEnd);

	void cmdSend(const std::string &cmd);
	void dataRequest();
	Success dataReceive();
	Success byteProcess(char ch);
	void fragmentAppend(const char *pBuf, size_t len);
	void fragmentFinish(const char *pBuf, size_t len);

	/* member variables */
	uint32_t mStateSwt;
	uint32_t mStartMs;
	RefDeviceUart mRefUart;
	bool mDevUartIsOnline;
	bool mTargetIsOnline;
	char mBufRcv[13];
	char *mpBuf;
	ssize_t mLenDone;
	std::map<int, std::string> mFragments;
	char mContentCurrent;
	SingleWireResponse mResp;

	/* static functions */
	static void cmdUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdUartVirtMountedToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdUartVirtTimeoutToggle(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdUartDataRcv(char *pArgs, char *pBuf, char *pBufEnd);
	static void cmdUartStrRcv(char *pArgs, char *pBuf, char *pBufEnd);

	/* static variables */

	/* constants */

};

#endif

