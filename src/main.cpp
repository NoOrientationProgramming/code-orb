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
#define APP_HAS_TCLAP 1
#else
#define APP_HAS_TCLAP 0
#endif

#if defined(__unix__)
#include <signal.h>
#endif
#include <iostream>
#if APP_HAS_TCLAP
#include <tclap/CmdLine.h>
#endif

#if APP_HAS_TCLAP
#include "TclapOutput.h"
#endif
#include "GwSupervising.h"
#include "LibDspc.h"

#include "env.h"

using namespace std;
#if APP_HAS_TCLAP
using namespace TCLAP;
#endif

#if defined(__unix__)
#define dDeviceUartDefault	"/dev/ttyACM0"
#elif defined(_WIN32)
#define dDeviceUartDefault	"COM1"
#else
#define dDeviceUartDefault	""
#endif

const int cRateRefreshDefaultMs = 500;
const int cRateRefreshMinMs = 10;
const int cRateRefreshMaxMs = 20000;
#define dStartPortsOrbDefault "2000"
#define dStartPortsTargetDefault "3000"
const int cPortMax = 64000;

Environment env;
GwSupervising *pApp = NULL;

#if APP_HAS_TCLAP
class AppHelpOutput : public TclapOutput {};
#endif

/*
Literature
- http://man7.org/linux/man-pages/man7/signal.7.html
  - for enums: kill -l
  - sys/signal.h
  SIGHUP  1     hangup
  SIGINT  2     interrupt
  SIGQUIT 3     quit
  SIGILL  4     illegal instruction (not reset when caught)
  SIGTRAP 5     trace trap (not reset when caught)
  SIGABRT 6     abort()
  SIGPOLL 7     pollable event ([XSR] generated, not supported)
  SIGFPE  8     floating point exception
  SIGKILL 9     kill (cannot be caught or ignored)
- https://www.usna.edu/Users/cs/aviv/classes/ic221/s16/lec/19/lec.html
- http://www.alexonlinux.com/signal-handling-in-linux
*/
void applicationCloseRequest(int signum)
{
	(void)signum;
	cout << endl;
	pApp->unusedSet();
}

int main(int argc, char *argv[])
{
	int res;

	env.verbosity = 0;
	env.coreDump = false;

	env.ctrlManual = 0;
	env.deviceUart = dDeviceUartDefault;
	env.rateRefreshMs = cRateRefreshDefaultMs;

	env.startPortsOrb = stoi(dStartPortsOrbDefault);
	env.startPortsTarget = stoi(dStartPortsTargetDefault);

#if APP_HAS_TCLAP
	CmdLine cmd("Command description message", ' ', appVersion());

	AppHelpOutput aho;
	cmd.setOutput(&aho);

	ValueArg<int> argVerbosity("v", "verbosity", "Verbosity: high => more output", false, 0, "uint8");
	cmd.add(argVerbosity);
	SwitchArg argCoreDump("", "core-dump", "Enable core dumps", false);
	cmd.add(argCoreDump);

	SwitchArg argCtrlManual("", "ctrl-manual", "Use manual control (automatic control disabled)", false);
	cmd.add(argCtrlManual);
	ValueArg<string> argDevUart("d", "device", "Device used for UART communication. Default: " dDeviceUartDefault,
								false, env.deviceUart, "string");
	cmd.add(argDevUart);
	ValueArg<int> argRateRefreshMs("", "refresh-rate", "Refresh rate of process tree in [ms]",
								false, env.rateRefreshMs, "uint16");
	cmd.add(argRateRefreshMs);

	ValueArg<int> argStartPortOrb("", "start-ports-orb", "Start of 3-port interface for CodeOrb. Default: " dStartPortsOrbDefault,
								false, env.startPortsOrb, "uint16");
	cmd.add(argStartPortOrb);
	ValueArg<int> argStartPortTarget("", "start-ports-target", "Start of 3-port interface for the target. Default: " dStartPortsTargetDefault,
								false, env.startPortsTarget, "uint16");
	cmd.add(argStartPortTarget);

	cmd.parse(argc, argv);

	res = argVerbosity.getValue();
	if (res > 0 && res < 6)
		env.verbosity = res;

	levelLogSet(env.verbosity);

	env.ctrlManual = argCtrlManual.getValue() ? 1 : 0;
	env.coreDump = argCoreDump.getValue();
	env.deviceUart = argDevUart.getValue();

	res = argRateRefreshMs.getValue();
	if (res > cRateRefreshMinMs &&
			res <= cRateRefreshMaxMs)
		env.rateRefreshMs = res;

	res = argStartPortOrb.getValue();
	if (res > 0 && res <= cPortMax)
		env.startPortsOrb = res;

	res = argStartPortTarget.getValue();
	if (res > 0 && res <= cPortMax)
		env.startPortsTarget = res;
#endif

#if defined(__unix__)
	/* https://www.gnu.org/software/libc/manual/html_node/Termination-Signals.html */
	signal(SIGINT, applicationCloseRequest);
	signal(SIGTERM, applicationCloseRequest);
#endif
	pApp = GwSupervising::create();
	if (!pApp)
	{
		errLog(-1, "could not create process");
		return 1;
	}

	pApp->procTreeDisplaySet(true);

	while (1)
	{
		for (int i = 0; i < 3; ++i)
			pApp->treeTick();

		this_thread::sleep_for(chrono::milliseconds(15));

		if (pApp->progress())
			continue;

		break;
	}

	Success success = pApp->success();
	Processing::destroy(pApp);

	Processing::applicationClose();

	return !(success == Positive);
}

