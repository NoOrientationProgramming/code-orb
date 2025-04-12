/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 12.04.2025

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

#ifndef COLOR_TESTING_H
#define COLOR_TESTING_H

#include "Processing.h"

class ColorTesting : public Processing
{

public:

	static ColorTesting *create()
	{
		return new dNoThrow ColorTesting;
	}

protected:

	ColorTesting();
	virtual ~ColorTesting() {}

private:

	ColorTesting(const ColorTesting &) = delete;
	ColorTesting &operator=(const ColorTesting &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	/* member variables */
	//uint32_t mStartMs;
	bool mInternal;
	ColorTesting *mpTest;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

