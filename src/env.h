/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 31.03.2018

  Copyright (C) 2018, Johannes Natter
*/

#ifndef ENV_H
#define ENV_H

#include <string>

/*
 * ##################################
 * Environment contains variables for
 *          !! IPC ONLY !!
 * ##################################
 */

struct Environment
{
	int verbosity;
	bool coreDumps;
	bool noAuto;
	std::string deviceUart;
};

extern Environment env;

#endif

