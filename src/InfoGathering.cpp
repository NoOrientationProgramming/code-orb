/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 11.04.2025

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

#include "InfoGathering.h"
#include "SingleWireScheduling.h"

#include "LibTime.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StCmdSend) \
		gen(StRespCmdWait) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

const uint8_t cCntFiltMax = 4;

InfoGathering::InfoGathering()
	: Processing("InfoGathering")
	, mEntriesReceived()
	, mStartMs(0)
	, mIdReq(0)
	, mCntFilt(0)
{
	mState = StStart;
}

/* member functions */

Success InfoGathering::process()
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

		mEntriesReceived.clear();

		mState = StCmdSend;

		break;
	case StCmdSend:

		ok = SingleWireScheduling::commandSend("infoHelp", mIdReq, PrioSysLow);
		if (!ok)
			return procErrLog(-1, "could not send command");

		//procWrnLog("request ID: %u", mIdReq);

		mStartMs = curTimeMs;
		mState = StRespCmdWait;

		break;
	case StRespCmdWait:

		if (diffMs > cTimeoutCommandResponseMs)
		{
			if (mCntFilt >= cCntFiltMax)
				return procErrLog(-1, "timeout getting response");

			++mCntFilt;

			// clear?

			mState = StCmdSend;
			break;
		}

		success = entryNewGet();
		if (success == Pending)
			break;

		if (success == Positive)
		{
			mState = StCmdSend;
			break;
		}
#if 0
		procWrnLog("gathered information");

		{
			list<string>::iterator iter;

			iter = mEntriesReceived.begin();
			for (; iter != mEntriesReceived.end(); ++iter)
				procWrnLog("  %s", iter->c_str());
		}
#endif
		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

Success InfoGathering::entryNewGet()
{
	string resp;
	bool ok;

	ok = SingleWireScheduling::commandResponseGet(mIdReq, resp);
	if (!ok)
		return Pending;

	//procWrnLog("response received: %s", resp.c_str());
	mCntFilt = 0;

	ok = entryFound(resp);
	if (ok)
		return -1;

	mEntriesReceived.push_back(resp);

	return Positive;
}

bool InfoGathering::entryFound(const string &entry)
{
	list<string>::iterator iter;

	iter = mEntriesReceived.begin();
	for (; iter != mEntriesReceived.end(); ++iter)
	{
		if (*iter != entry)
			continue;

		return true;
	}

	return false;
}

void InfoGathering::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

