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

#include <sstream>

#include "RemoteCommanding.h"
#include "SingleWireScheduling.h"
#include "LibTime.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StSendReadyWait) \
		gen(StWelcomeSend) \
		gen(StMain) \
		gen(StResponseRcvdWait) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 1
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

const uint32_t cTimeoutResponseMs = 300;

// https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
#define dColorGreen "\033[38;5;46m"
#define dColorOrange "\033[38;5;220m"
#define dColorGrey "\033[38;5;240m"
#define dColorClear "\033[0m"

//#define dSizeHistoryMax 31
#define dSizeHistoryMax 5

const string cWelcomeMsg = "\r\n" dPackageName "\r\n" \
			"Remote Terminal\r\n\r\n" \
			"type 'help' or just 'h' for a list of available commands\r\n\r\n";

const string cInternalCmdCls = "dbg";
const int cSizeCmdIdMax = 16;

list<EntryHelp> RemoteCommanding::cmds;

RemoteCommanding::RemoteCommanding(SOCKET fd)
	: Processing("RemoteCommanding")
	, mpTargetIsOnline(NULL)
	, mStartMs(0)
	, mFdSocket(fd)
	, mpFilt(NULL)
	, mTxtPrompt()
	, mIdReq(0)
	, mTargetIsOnline(false)
	, mStartCmdMs(0)
	, mDelayResponseCmdMs(0)
	, mCmdLast("")
	, mHistory()
{
	mBufOut[0] = 0;
	miEntryHist = mHistory.end();

	mState = StStart;
}

/* member functions */

Success RemoteCommanding::process()
{
	uint32_t curTimeMs = millis();
	uint32_t diffMs = curTimeMs - mStartMs;
	Success success;
	PipeEntry<KeyUser> entKey;
	KeyUser key;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		if (mFdSocket == INVALID_SOCKET)
			return procErrLog(-1, "socket file descriptor not set");

		if (!mpTargetIsOnline)
			return procErrLog(-1, "target online pointer not set");

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

		mTxtPrompt.widthSet(25);
		mTxtPrompt.lenMaxSet(51);
		mTxtPrompt.cursorBoundSet(2);
		mTxtPrompt.focusSet(true);

		mTxtPrompt.frameEnabledSet(false);
		mTxtPrompt.paddingEnabledSet(false);

		mTargetIsOnline = not *mpTargetIsOnline;

		mState = StWelcomeSend;

		break;
	case StWelcomeSend:

		mpFilt->send(cWelcomeMsg.c_str(), cWelcomeMsg.size());
		promptSend();

		mState = StMain;

		break;
	case StMain:

		if (stateOnlineChanged())
		{
			promptSend();
			break;
		}

		if (mpFilt->ppKeys.get(entKey) < 1)
			break;
		key = entKey.particle;

		//procWrnLog("Got key: %s", key.str().c_str());

		if (historyNavigate(key))
		{
			promptSend();
			break;
		}

		if (key == keyEnter)
		{
			success = commandSend();
			if (success == Positive)
				break;

			if (success != Pending)
				break;

			lineAck();

			mStartCmdMs = curTimeMs;

			mStartMs = curTimeMs;
			mState = StResponseRcvdWait;
			break;
		}

		if (key == keyUp || key == keyDown)
			break;

		if (mTxtPrompt.keyProcess(key))
		{
			promptSend();
			break;
		}

		// TODO: Implement 'Done' on empty string

		break;
	case StResponseRcvdWait:

		if (diffMs > cTimeoutResponseMs)
		{
			string msg;

			msg += dColorGrey;
			msg += "<command response timeout>";
			msg += dColorClear;
			msg += "\r\n";

			mpFilt->send(msg.c_str(), msg.size());
			promptSend();

			mState = StMain;
			break;
		}

		success = responseReceive();
		if (success == Pending)
			break;

		mState = StMain;

		break;
	default:
		break;
	}

	return Pending;
}

bool RemoteCommanding::stateOnlineChanged()
{
	if (*mpTargetIsOnline == mTargetIsOnline)
		return false;

	mTargetIsOnline = *mpTargetIsOnline;

	return true;
}

Success RemoteCommanding::commandSend()
{
	bool ok;

	mTxtPrompt.focusSet(false);
	string str = mTxtPrompt;

	if (!str.size())
	{
		lineAck();
		promptSend();
		return Positive;
	}

	mCmdLast = str;

	if (str == "help" || str == "h")
	{
		mBufOut[0] = 0;
		cmdHelpPrint(NULL, mBufOut, mBufOut + sizeof(mBufOut));

		string msg;
		lfToCrLf(mBufOut, msg);

		if (msg.size())
			msg += "\r\n";

		lineAck();
		mpFilt->send(msg.c_str(), msg.size());
		promptSend();

		return Positive;
	}

	//procWrnLog("sending command: %s", str.c_str());

	ok = SingleWireScheduling::commandSend(str, mIdReq);
	if (!ok)
		return procErrLog(-1, "could not send command");

	return Pending;
}

Success RemoteCommanding::responseReceive()
{
	string resp;
	bool ok;

	ok = SingleWireScheduling::commandResponseGet(mIdReq, resp);
	if (!ok)
		return Pending;

	//procWrnLog("response received: %s", resp.c_str());

	string msg;
	lfToCrLf(resp.data(), msg);

	if (msg.size())
		msg += "\r\n";

	mpFilt->send(msg.c_str(), msg.size());
	promptSend();

	mDelayResponseCmdMs = millis() - mStartCmdMs;

	return Positive;
}

void RemoteCommanding::lineAck()
{
	promptSend(false, false, true);

	mTxtPrompt = "";
	mTxtPrompt.focusSet(true);

	historyUpdate();
}

void RemoteCommanding::historyUpdate()
{
	miEntryHist = mHistory.end();

	if (!mCmdLast.size())
		return;

	// ignore duplicate
	if (mHistory.size() && mCmdLast == mHistory.back())
		return;

	mHistory.push_back(mCmdLast);

	while (mHistory.size() > dSizeHistoryMax)
		mHistory.pop_front();
}

bool RemoteCommanding::historyNavigate(KeyUser &key)
{
	if (key != keyUp && key != keyDown)
		return false;

	if (!mHistory.size())
		return false;

	if (key == keyUp && miEntryHist != mHistory.begin())
		--miEntryHist;

	if (key == keyDown && miEntryHist != mHistory.end())
		++miEntryHist;

	if (miEntryHist == mHistory.end())
		mTxtPrompt = "";
	else
		mTxtPrompt = *miEntryHist;

	mTxtPrompt.focusSet(true);

	return true;
}

void RemoteCommanding::promptSend(bool cursor, bool preNewLine, bool postNewLine)
{
	string msg;

	if (preNewLine)
		msg += "\r\n";

	msg += "\rcore@";

	if (!mTargetIsOnline && !postNewLine)
		msg += dColorOrange;

	if (mTargetIsOnline && !postNewLine)
		msg += dColorGreen;

	msg += "remote";
	msg += dColorClear;

	msg += ":";
	msg += "~"; // directory
	msg += "# ";

	if (!mTargetIsOnline && !postNewLine &&
			!mTxtPrompt.sizeDisplayed())
	{
		msg += dColorGrey;
		msg += "<target offline>";
		msg += dColorClear;
	}
	else
	{
		mTxtPrompt.cursorShow(cursor);
		mTxtPrompt.print(msg);
	}

	if (postNewLine)
		msg += "\r\n";

	mpFilt->send(msg.c_str(), msg.size());
}

void RemoteCommanding::cmdHelpPrint(char *pArgs, char *pBuf, char *pBufEnd)
{
	list<EntryHelp>::iterator iter;
	EntryHelp cmd;
	string group = "";

	(void)pArgs;

	dInfo("\nAvailable commands\n");

	iter = cmds.begin();
	for (; iter != cmds.end(); ++iter)
	{
		cmd = *iter;

		if (cmd.group != group)
		{
			dInfo("\n");

			if (cmd.group.size() && cmd.group != cInternalCmdCls)
				dInfo("%s\n", cmd.group.c_str());
			group = cmd.group;
		}

		dInfo("  ");

		if (cmd.shortcut != "")
			dInfo("%s, ", cmd.shortcut.c_str());
		else
			dInfo("   ");

		dInfo("%-*s", cSizeCmdIdMax + 2, cmd.id.c_str());

		if (cmd.desc.size())
			dInfo(".. %s", cmd.desc.c_str());

		dInfo("\n");
	}
}

void RemoteCommanding::processInfo(char *pBuf, char *pBufEnd)
{
#if 1
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
	dInfo("Last command\t\t%s\n",
			mCmdLast.size() ? mCmdLast.c_str() : "<none>");
	dInfo("Command delay\t\t%u [ms]\n", mDelayResponseCmdMs);
#if 1
	list<string>::iterator iter;

	dInfo("Command history\n");

	if (!mHistory.size())
		dInfo("  <none>\n");

	iter = mHistory.begin();
	for (; iter != mHistory.end(); ++iter)
	{
		dInfo("%c %s\n",
				iter == miEntryHist ? '>' : ' ',
				iter->c_str());
	}
#endif
}

/* static functions */

void RemoteCommanding::listCommandsUpdate(const list<string> &listStr)
{
	list<string>::const_iterator iter;
	vector<string> partsEntry;
	EntryHelp entry;

	cmds.clear();

	iter = listStr.begin();
	for (; iter != listStr.end(); ++iter)
	{
		const string &str = *iter;

		if (!str.size())
			continue;

		if (str == "infoHelp|||")
			continue;

		//wrnLog("entry received: %s", str.c_str());

		partsEntry = split(str, '|');
		if (partsEntry.size() != 4)
		{
			wrnLog("wrong number of parts for entry: %zu", partsEntry.size());
			continue;
		}

		entry.id = partsEntry[0];
		entry.shortcut = partsEntry[1];
		entry.desc = partsEntry[2];
		entry.group = partsEntry[3];

		cmds.push_back(entry);
	}

	entry.id = "help";
	entry.shortcut = "h";
	entry.desc = "This help screen";
	entry.group = cInternalCmdCls;

	cmds.push_back(entry);
	cmds.sort(commandSort);
}

bool RemoteCommanding::commandSort(const EntryHelp &cmdFirst, const EntryHelp &cmdSecond)
{
	if (cmdFirst.group == cInternalCmdCls && cmdSecond.group != cInternalCmdCls)
		return true;
	if (cmdFirst.group != cInternalCmdCls && cmdSecond.group == cInternalCmdCls)
		return false;

	if (cmdFirst.group < cmdSecond.group)
		return true;
	if (cmdFirst.group > cmdSecond.group)
		return false;

	if (cmdFirst.shortcut != "" && cmdSecond.shortcut == "")
		return true;
	if (cmdFirst.shortcut == "" && cmdSecond.shortcut != "")
		return false;

	if (cmdFirst.id < cmdSecond.id)
		return true;
	if (cmdFirst.id > cmdSecond.id)
		return false;

	return true;
}

vector<string> RemoteCommanding::split(const string &str, char delimiter)
{
	vector<string> result;
	stringstream ss(str);
	string item;

	while (getline(ss, item, delimiter))
		result.push_back(item);

	return result;
}

void RemoteCommanding::lfToCrLf(const char *pBuf, string &str)
{
	const char *pBufLineStart, *pBufIter;
	const char *pBufEnd;
	size_t lenBuf;

	str.clear();

	if (!pBuf || !*pBuf)
		return;

	lenBuf = strlen(pBuf);
	str.reserve(lenBuf << 1);

	pBufEnd = pBuf + lenBuf;
	pBufLineStart = pBufIter = pBuf;

	while (1)
	{
		if (pBufIter >= pBufEnd)
			break;

		if (*pBufIter != '\n')
		{
			++pBufIter;
			continue;
		}

		str += string(pBufLineStart, pBufIter - pBufLineStart);
		str += "\r\n";

		++pBufIter;
		pBufLineStart = pBufIter;
	}

	str += pBufLineStart;
}

