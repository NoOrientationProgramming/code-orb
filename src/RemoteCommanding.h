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

#ifndef REMOTE_COMMANDING_H
#define REMOTE_COMMANDING_H

#include "Processing.h"
#include "TelnetFiltering.h"

struct EntryHelp
{
	std::string id;
	std::string shortcut;
	std::string desc;
	std::string group;
};

class RemoteCommanding : public Processing
{

public:

	static RemoteCommanding *create(SOCKET fd)
	{
		return new dNoThrow RemoteCommanding(fd);
	}

	static void listCommandsUpdate(const std::list<std::string> &listStr);

protected:

	RemoteCommanding(SOCKET fd);
	virtual ~RemoteCommanding() {}

private:

	RemoteCommanding() = delete;
	RemoteCommanding(const RemoteCommanding &) = delete;
	RemoteCommanding &operator=(const RemoteCommanding &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	void processInfo(char *pBuf, char *pBufEnd);

	void promptSend(bool cursor = true, bool preNewLine = false, bool postNewLine = false);

	/* member variables */
	//uint32_t mStartMs;
	SOCKET mFdSocket;
	TelnetFiltering *mpFilt;

	/* static functions */

	/* static variables */
	static std::list<EntryHelp> listCmds;

	/* constants */

};

#endif

