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

#include "RemoteCommanding.h"

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

RemoteCommanding::RemoteCommanding(SOCKET fd)
	: Processing("RemoteCommanding")
	//, mStartMs(0)
	, mFdSocket(fd)
	, mpTrans(NULL)
	, mDone(false)
{
	mState = StStart;
}

/* member functions */

Success RemoteCommanding::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (mFdSocket == INVALID_SOCKET)
			return procErrLog(-1, "socket file descriptor not set");

		mpTrans = TcpTransfering::create(mFdSocket);
		if (!mpTrans)
			return procErrLog(-1, "could not create process");

		mpTrans->procTreeDisplaySet(false);
		start(mpTrans);

		mState = StMain;

		break;
	case StMain:

		success = mpTrans->success();
		if (success != Pending)
			return success;

		dataReceive();

		if (!mDone)
			break;

		return Positive;

		break;
	case StNop:

		break;
	default:
		break;
	}

	return Pending;
}

void RemoteCommanding::dataReceive()
{
}

void RemoteCommanding::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

