/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 10.10.2022

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

#ifndef TELNET_FILTERING_H
#define TELNET_FILTERING_H

#include "Processing.h"
#include "KeyFiltering.h"
#include "TcpTransfering.h"

class TelnetFiltering : public KeyFiltering
{

public:

	static TelnetFiltering *create(int fd)
	{
		return new (std::nothrow) TelnetFiltering(fd);
	}

	void titleSet(const std::string &title);

	ssize_t read(void *pBuf, size_t len);
	ssize_t send(const void *pData, size_t len);

protected:

	TelnetFiltering(int fd);
	virtual ~TelnetFiltering() {}

private:

	TelnetFiltering(const TelnetFiltering &) = delete;
	TelnetFiltering &operator=(const TelnetFiltering &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	Success dataProcess();
	Success keyGet(uint8_t key);
	void keyPrintCommit(char32_t key,
				bool modShift = false,
				bool modAlt = false,
				bool modCtrl = false);
	void keyCtrlCommit(CtrlKeyUser key,
				bool modShift = false,
				bool modAlt = false,
				bool modCtrl = false);
	void statesKeySet(uint32_t state, uint32_t stateRet);

	/* member variables */
	uint32_t mStateKey;
	uint32_t mStateKeyRet;
	int mSocketFd;
	TcpTransfering *mpConn;
#if CONFIG_PROC_HAVE_DRIVERS
	std::mutex mMtxConn;
#endif
	std::string mTitle;
	std::string mFragmentUtf;
	uint8_t mCntFragment;
	bool mModShift;
	bool mModAlt;
	bool mModCtrl;
	size_t mNumCommited;
	KeyUser mLast;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

