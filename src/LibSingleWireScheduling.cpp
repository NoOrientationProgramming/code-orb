/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 21.04.2025

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

#include "SingleWireScheduling.h"
#include "SystemDebugging.h"
#include "LibDspc.h"

#include "env.h"

using namespace std;

const size_t cNumRequestsCmdMax = 40;

void SingleWireScheduling::targetOnlineSet(bool online)
{
	mTargetIsOnline = online;

	if (mTargetIsOnlineOld == mTargetIsOnline)
		return;
	mTargetIsOnlineOld = online;

	if (online)
		return;

	if (mTargetIsOfflineMarked)
		return;
	mTargetIsOfflineMarked = true;

	mContentProc += "\r\n[Target is offline]\r\n";
	mContentProcChanged = true;
}

void SingleWireScheduling::responseReset(uint8_t idContent)
{
	mResp.idContent = idContent;
	mResp.content = "";
}

void SingleWireScheduling::fragmentAppend(uint8_t ch)
{
	if (!ch)
		return;

	uint8_t idContent = mResp.idContent;
	bool fragmentFound =
			mFragments.find(idContent) != mFragments.end();

	if (!fragmentFound)
	{
		mFragments[idContent] = string(1, ch);
		return;
	}

	const string &str = mFragments[idContent];

	if (str.size() > cSizeFragmentMax)
		return;

	mFragments[idContent] += string(1, ch);

	return;
}

void SingleWireScheduling::fragmentFinish()
{
	uint8_t idContent = mResp.idContent;
	bool fragmentFound =
			mFragments.find(idContent) != mFragments.end();

	if (!fragmentFound)
		return;

	mResp.content = mFragments[idContent];
	mFragments.erase(idContent);
}

void SingleWireScheduling::fragmentDelete()
{
	uint8_t idContent = mResp.idContent;
	bool fragmentFound =
			mFragments.find(idContent) != mFragments.end();

	if (!fragmentFound)
		return;

	mFragments.erase(idContent);
}

void SingleWireScheduling::fragmentsPrint(char *pBuf, char *pBufEnd)
{
	dInfo("Fragments\n");

	if (!mFragments.size())
	{
		dInfo("  <none>\n");
		return;
	}

	map<int, string>::iterator iter;

	iter = mFragments.begin();
	for (; iter != mFragments.end(); ++iter)
		dInfo("  %02X > '%s'\n", iter->first, iter->second.c_str());
}

void SingleWireScheduling::queuesCmdPrint(char *pBuf, char *pBufEnd)
{
	requestsCmdPrint(pBuf, pBufEnd);
	responsesCmdPrint(pBuf, pBufEnd);
}

void SingleWireScheduling::requestsCmdPrint(char * &pBuf, char *pBufEnd)
{
	Guard lock(mtxRequests);

	dInfo("Command requests\n");
	dInfo("ID next\t\t\t%u\n", idReqCmdNext);

	list<CommandReqResp> *pList;
	list<CommandReqResp>::iterator iter;
	uint32_t curTimeMs = millis();
	uint32_t diffMs;

	for (size_t i = 0; i < 3; ++i)
	{
		pList = &requestsCmd[i];

		dInfo("Priority %zu\n", i);

		if (!pList->size())
		{
			dInfo("  <none>\n");
			continue;
		}

		iter = pList->begin();
		for (; iter != pList->end(); ++iter)
		{
			diffMs = curTimeMs - iter->startMs;

			if (diffMs > cTimeoutCmdReq)
				diffMs = cTimeoutCmdReq;

			dInfo("  Req %u: %s (%u)\n",
					iter->idReq,
					iter->str.c_str(),
					diffMs);
		}
	}
}

void SingleWireScheduling::responsesCmdPrint(char * &pBuf, char *pBufEnd)
{
	Guard lock(mtxResponses);

	list<CommandReqResp>::iterator iter;
	uint32_t curTimeMs = millis();
	uint32_t diffMs;

	dInfo("Command responses\n");
	iter = responsesCmd.begin();
	for (; iter != responsesCmd.end(); ++iter)
	{
		diffMs = curTimeMs - iter->startMs;

		if (diffMs > cTimeoutCmdReq)
			diffMs = cTimeoutCmdReq;

		dInfo("  Resp %u: %s (%u)\n",
				iter->idReq,
				iter->str.c_str(),
				diffMs);
	}
}

bool SingleWireScheduling::commandSend(const string &cmd, uint32_t &idReq, PrioCmd prio)
{
	{
		Guard lock(mtxResponses);

		if (responsesCmd.size() > cNumRequestsCmdMax)
			return false;
	}

	Guard lock(mtxRequests);

	list<CommandReqResp> *pList = &requestsCmd[prio];

	if (pList->size() > cNumRequestsCmdMax)
		return false;

	idReq = idReqCmdNext;
	++idReqCmdNext;

	pList->emplace_back(cmd, idReq, millis());

	return true;
}

bool SingleWireScheduling::commandResponseGet(uint32_t idReq, string &resp)
{
	Guard lock(mtxResponses);

	list<CommandReqResp>::iterator iter;

	iter = responsesCmd.begin();
	while (iter != responsesCmd.end())
	{
		if (iter->idReq != idReq)
		{
			++iter;
			continue;
		}

		resp = iter->str;
		iter = responsesCmd.erase(iter);

		return true;
	}

	return false;
}

void SingleWireScheduling::commandsRegister()
{
	cmdReg("ctrlManualToggle", cmdCtrlManualToggle,      "",  "Toggle manual control",               "Manual Control");
	cmdReg("dataUartSend",     cmdDataUartSend,          "",  "Send byte stream",                    "Manual Control");
	cmdReg("strUartSend",      cmdStrUartSend,           "",  "Send string",                         "Manual Control");
	cmdReg("dataUartRead",     cmdDataUartRead,          "",  "Read data",                           "Manual Control");
	cmdReg("modeUartVirtSet",  cmdModeUartVirtSet,       "",  "Mode: uart, swart (default)",         "Virtual UART");
	cmdReg("uartVirtToggle",   cmdUartVirtToggle,        "",  "Enable/Disable virtual UART",         "Virtual UART");
	cmdReg("mountedToggle",    cmdMountedUartVirtToggle, "m", "Mount/Unmount virtual UART",          "Virtual UART");
	cmdReg("timeoutToggle",    cmdTimeoutUartVirtToggle, "t", "Enable/Disable virtual UART timeout", "Virtual UART");
	cmdReg("dataUartRcv",      cmdDataUartRcv,           "",  "Receive byte stream",                 "Virtual UART");
	cmdReg("strUartRcv",       cmdStrUartRcv,            "",  "Receive string",                      "Virtual UART");
}

void SingleWireScheduling::cmdCtrlManualToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	env.ctrlManual ^= 1;
	dInfo("Manual control %sabled", env.ctrlManual ? "en" : "dis");
}

void SingleWireScheduling::cmdDataUartSend(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	dataUartSend(pArgs, pBuf, pBufEnd, uartSend);
}

void SingleWireScheduling::cmdStrUartSend(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	strUartSend(pArgs, pBuf, pBufEnd, uartSend);
}

void SingleWireScheduling::cmdDataUartRead(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	if (!env.ctrlManual)
	{
		dInfo("Manual control disabled");
		return;
	}

	ssize_t lenDone;
	char buf[55];

	lenDone = uartRead(refUart, buf, sizeof(buf));
	if (!lenDone)
	{
		dInfo("No data");
		return;
	}

	if (lenDone < 0)
	{
		dInfo("Error receiving data: %zu", lenDone);
		return;
	}

	ssize_t lenMax = 32;
	ssize_t lenReq = PMIN(lenMax, lenDone);

	hexDumpPrint(pBuf, pBufEnd, buf, lenReq, NULL, 8);
}

void SingleWireScheduling::cmdModeUartVirtSet(char *pArgs, char *pBuf, char *pBufEnd)
{
	if (pArgs && *pArgs == 'u')
		uartVirtualMode = 1;
	else
		uartVirtualMode = 0;

	dInfo("Virtual UART mode: %s", uartVirtualMode ? "uart" : "swart");
}

void SingleWireScheduling::cmdUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtual ^= 1;
	dInfo("Virtual UART %sabled", uartVirtual ? "en" : "dis");
}

void SingleWireScheduling::cmdMountedUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtualMounted ^= 1;
	dInfo("Virtual UART %smounted", uartVirtualMounted ? "" : "un");
}

void SingleWireScheduling::cmdTimeoutUartVirtToggle(char *pArgs, char *pBuf, char *pBufEnd)
{
	(void)pArgs;

	uartVirtualTimeout ^= 1;
	dInfo("Virtual UART timeout %s", uartVirtualTimeout ? "set" : "cleared");
}

void SingleWireScheduling::cmdDataUartRcv(char *pArgs, char *pBuf, char *pBufEnd)
{
	dataUartSend(pArgs, pBuf, pBufEnd, uartVirtRcv);
}

void SingleWireScheduling::cmdStrUartRcv(char *pArgs, char *pBuf, char *pBufEnd)
{
	strUartSend(pArgs, pBuf, pBufEnd, uartVirtRcv);
}

void SingleWireScheduling::dataUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend)
{
	if (!pArgs)
	{
		dInfo("No data given");
		return;
	}

	string str = string(pArgs, strlen(pArgs));

	if (str == "flowOut")  str = "0B";
	if (str == "flowIn")   str = "0C";
	if (str == "cmdOut")   str = "1A";
	if (str == "none")     str = "15";
	if (str == "proc")     str = "11";
	if (str == "log")      str = "12";
	if (str == "cmd")      str = "13";
	if (str == "cut")      str = "0F";
	if (str == "end")      str = "17";
	if (str == "tab")      str = "09";
	if (str == "cr")       str = "0D";
	if (str == "lf")       str = "0A";

	vector<char> vData = toHex(str);
	pFctSend(refUart, vData.data(), vData.size());

	dInfo("Data moved");
}

void SingleWireScheduling::strUartSend(char *pArgs, char *pBuf, char *pBufEnd, FuncUartSend pFctSend)
{
	if (!pArgs)
	{
		dInfo("No string given");
		return;
	}

	pFctSend(refUart, pArgs, strlen(pArgs));
	dInfo("String moved");
}

