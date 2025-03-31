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
#include <signal.h>
#endif
#include <iostream>

#include "GwSupervising.h"

using namespace std;

GwSupervising *pApp = NULL;

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
	(void)argc;
	(void)argv;

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
		pApp->treeTick();

		this_thread::sleep_for(chrono::milliseconds(2));

		if (pApp->progress())
			continue;

		break;
	}

	Success success = pApp->success();
	Processing::destroy(pApp);

	Processing::applicationClose();

	return !(success == Positive);
}

