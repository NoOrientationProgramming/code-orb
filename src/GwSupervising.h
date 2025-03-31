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

#ifndef GW_SUPERVISING_H
#define GW_SUPERVISING_H

#include "Processing.h"

class GwSupervising : public Processing
{

public:

	static GwSupervising *create()
	{
		return new dNoThrow GwSupervising;
	}

protected:

	GwSupervising();
	virtual ~GwSupervising() {}

private:

	GwSupervising(const GwSupervising &) = delete;
	GwSupervising &operator=(const GwSupervising &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	bool servicesStart();

	/* member variables */
	//uint32_t mStartMs;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

