/*
  This file is part of the DSP-Crowd project
  https://www.dsp-crowd.com

  Author(s):
      - Johannes Natter, office@dsp-crowd.com

  File created on 10.10.2022

  Copyright (C) 2022, Johannes Natter
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

