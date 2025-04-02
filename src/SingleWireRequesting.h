/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 02.04.2025

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

#ifndef SINGLE_WIRE_REQUESTING_H
#define SINGLE_WIRE_REQUESTING_H

#include <string>

#include "Processing.h"
#include "LibUart.h"

enum SwtContentId
{
	ContentNone = 0x00,
	ContentLog = 0xC0,
	ContentCmd,
	ContentProc,
};

class SingleWireRequesting : public Processing
{

public:

	static SingleWireRequesting *create(RefDeviceUart &refUart)
	{
		return new dNoThrow SingleWireRequesting(refUart);
	}

	void cmdSet(const std::string &str);

protected:

	SingleWireRequesting(RefDeviceUart &refUart);
	virtual ~SingleWireRequesting() {}

private:

	SingleWireRequesting() = delete;
	SingleWireRequesting(const SingleWireRequesting &) = delete;
	SingleWireRequesting &operator=(const SingleWireRequesting &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	/* member variables */
	RefDeviceUart &mRefUart;
	uint32_t mStartMs;
	uint8_t mContentOut;
	std::string mCmd;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

