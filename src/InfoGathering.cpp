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
#include "SingleWireControlling.h"

#include "LibTime.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \
		gen(StRespCmdWait) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

InfoGathering::InfoGathering()
	: Processing("InfoGathering")
	, mStartMs(0)
	, mIdReq(0)
{
	mState = StStart;
}

const uint32_t cTimeoutResponseMs = 3300;

/* member functions */

Success InfoGathering::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	//Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		mState = StMain;

		break;
	case StMain:

		ok = SingleWireControlling::commandSend("infoHelp", mIdReq, PrioSysLow);
		if (!ok)
			return procErrLog(-1, "could not send command");

		procWrnLog("request ID: %u", mIdReq);

		mStartMs = curTimeMs;
		mState = StRespCmdWait;

		break;
	case StRespCmdWait:

		if (diffMs > cTimeoutResponseMs)
			return procErrLog(-1, "timeout getting response");

		ok = responseCheck();
		if (!ok)
			break;

		procWrnLog("gathered information");

		return Positive;

		break;
	default:
		break;
	}

	return Pending;
}

bool InfoGathering::responseCheck()
{
	string resp;
	bool ok;

	ok = SingleWireControlling::commandResponseGet(mIdReq, resp);
	if (!ok)
		return false;

	procWrnLog("response received: %s", resp.c_str());

	return true;
}

void InfoGathering::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

