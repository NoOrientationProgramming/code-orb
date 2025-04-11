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

#if defined(__unix__)
#include <unistd.h>
#include <signal.h>
#endif

#include "GwSupervising.h"
#include "SystemDebugging.h"
#include "LibFilesys.h"

#include "env.h"

#define dForEach_ProcState(gen) \
		gen(StStart) \
		gen(StMain) \

#define dGenProcStateEnum(s) s,
dProcessStateEnum(ProcState);

#if 0
#define dGenProcStateString(s) #s,
dProcessStateStr(ProcState);
#endif

using namespace std;

#if defined(__unix__)
static Processing *pTreeRoot = NULL;
static char nameApp[16];
static char nameFileProc[64];
static char buffProcTree[2*1024*1024];
static bool procTreeSaveInProgress = false;
#endif

GwSupervising::GwSupervising()
	: Processing("GwSupervising")
	//, mStartMs(0)
	, mpApp(NULL)
{
	mState = StStart;
}

/* member functions */

Success GwSupervising::process()
{
	//uint32_t curTimeMs = millis();
	//uint32_t diffMs = curTimeMs - mStartMs;
	//Success success;
	bool ok;
#if 0
	dStateTrace;
#endif
	switch (mState)
	{
	case StStart:

		ok = servicesStart();
		if (!ok)
			return procErrLog(-1, "could not start services");

		mState = StMain;

		break;
	case StMain:

		break;
	default:
		break;
	}

	return Pending;
}

#if defined(__unix__)
static void procTreeSave()
{
	time_t now;
	FILE *pFile;
	int res;
	size_t lenReq, lenDone;
	string strTime;

	if (procTreeSaveInProgress)
		return;
	procTreeSaveInProgress = true;

	now = time(NULL);

	if (!pTreeRoot)
	{
		wrnLog("process tree root not defined");
		return;
	}

	*buffProcTree = 0;

	pTreeRoot->processTreeStr(
		buffProcTree,
		buffProcTree + sizeof(buffProcTree),
		true,
		true);

	strTime = to_string(now);
	res = snprintf(nameFileProc, sizeof(nameFileProc),
			"%s_%s_tree-proc.txt",
			strTime.c_str(),
			nameApp);
	if (res < 0)
	{
		wrnLog("could not create process tree file name");
		return;
	}

	pFile = fopen(nameFileProc, "w");
	if (!pFile)
	{
		wrnLog("could not open process tree file");
		return;
	}

	lenReq = strlen(buffProcTree);
	lenDone = fwrite(buffProcTree, 1, lenReq, pFile);

	fclose(pFile);

	if (lenDone != lenReq)
	{
		wrnLog("error writing to process tree file");
		return;
	}
}

// - https://man7.org/linux/man-pages/man5/core.5.html
// - https://man7.org/linux/man-pages/man7/signal.7.html
// - https://man7.org/linux/man-pages/man3/abort.3.html
void coreDumpRequest(int signum)
{
	wrnLog("Got signal: %d", signum);

	if (signum != SIGABRT)
	{
		wrnLog("Requesting core dump");
		abort();

		return;
	}

	wrnLog("Creating process tree file");
	procTreeSave();
}
#endif

bool GwSupervising::servicesStart()
{
#if defined(__unix__)
	bool ok;

	if (env.coreDump)
	{
		procWrnLog("enable core dumps");

		signal(SIGABRT, coreDumpRequest);

		ok = coreDumpsEnable(coreDumpRequest);
		if (!ok)
			procWrnLog("could not enable core dumps");
	}
#endif
	SystemDebugging *pDbg;

	pDbg = SystemDebugging::create(this);
	if (!pDbg)
	{
		procWrnLog("could not create process");
		return false;
	}

	pDbg->listenLocalSet();
	pDbg->portStartSet(env.startPortsOrb);

	pDbg->procTreeDisplaySet(false);
	start(pDbg);

	mpApp = GwMsgDispatching::create();
	if (!mpApp)
	{
		procWrnLog("could not create process");
		return false;
	}

	start(mpApp);

	return true;
}

void GwSupervising::processInfo(char *pBuf, char *pBufEnd)
{
	(void)pBuf;
	(void)pBufEnd;
#if 0
	dInfo("State\t\t\t%s\n", ProcStateString[mState]);
#endif
}

/* static functions */

