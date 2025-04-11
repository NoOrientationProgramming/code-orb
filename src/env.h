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
	bool haveTclap;
	int verbosity;
#if defined(__unix__)
	bool coreDump;
#endif
	uint8_t ctrlManual;
	std::string codeUart;
	std::string deviceUart;
	uint32_t rateRefreshMs;
	uint16_t startPortsOrb;
	uint16_t startPortsTarget;
};

extern Environment env;

#endif

