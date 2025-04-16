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
#include "SingleWireScheduling.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StSendReadyWait) \
		gen(StWelcomeSend) \
		gen(StMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

const string cWelcomeMsg = "\r\n" dPackageName "\r\n" \
			"Remote Terminal\r\n\r\n" \
			"type 'help' or just 'h' for a list of available commands\r\n\r\n";

list<EntryHelp> RemoteCommanding::listCmds;

RemoteCommanding::RemoteCommanding(SOCKET fd)
	: Processing("RemoteCommanding")
	//, mStartMs(0)
	, mFdSocket(fd)
	, mpFilt(NULL)
{
	mState = StStart;
}

/* member functions */

Success RemoteCommanding::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	PipeEntry<KeyUser> entKey;
	KeyUser key;
	string msg;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (mFdSocket == INVALID_SOCKET)
			return procErrLog(-1, "socket file descriptor not set");

		mpFilt = TelnetFiltering::create(mFdSocket);
		if (!mpFilt)
			return procErrLog(-1, "could not create process");

		mpFilt->titleSet("CodeOrb");

		mpFilt->procTreeDisplaySet(false);
		start(mpFilt);

		mState = StSendReadyWait;

		break;
	case StSendReadyWait:

		success = mpFilt->success();
		if (success != Pending)
			return success;

		if (!mpFilt->mSendReady)
			break;

		mState = StWelcomeSend;

		break;
	case StWelcomeSend:

		mpFilt->send(cWelcomeMsg.c_str(), cWelcomeMsg.size());
		promptSend();

		mState = StMain;

		break;
	case StMain:

		if (mpFilt->ppKeys.get(entKey) < 1)
			break;
		key = entKey.particle;

		procWrnLog("Got key: %s", key.str().c_str());

		// TODO: Implement 'Done' on empty string

		break;
	default:
		break;
	}

	if (!msg.size())
		return Pending;

	mpFilt->send(msg.c_str(), msg.size());

	return Pending;
}

void RemoteCommanding::promptSend(bool cursor, bool preNewLine, bool postNewLine)
{
	string msg;

	if (preNewLine)
		msg += "\r\n";

	msg += "\rcore@";
	msg += "remote";
	msg += ":";
	msg += "~"; // directory
	msg += "# ";

	(void)cursor;

	if (postNewLine)
		msg += "\r\n";

	mpFilt->send(msg.c_str(), msg.size());
}

void RemoteCommanding::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

void RemoteCommanding::listCommandsUpdate(const list<string> &listStr)
{
	list<string>::const_iterator iter;

	listCmds.clear();

	iter = listStr.begin();
	for (; iter != listStr.end(); ++iter)
	{
		const string &str = *iter;

		if (!str.size())
			continue;

		if (str == "infoHelp|||")
			continue;

		wrnLog("Entry received: %s", str.c_str());
		// TODO: Parse
	}
}

