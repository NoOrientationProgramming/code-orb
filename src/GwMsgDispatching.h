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

#ifndef GW_MSG_DISPATCHING_H
#define GW_MSG_DISPATCHING_H

#include "Processing.h"
#include "TcpListening.h"
#include "TcpTransfering.h"
#include "SingleWireControlling.h"
#include "RemoteCommanding.h"
#include "InfoGathering.h"

enum RemotePeerType {
	RemotePeerProc = 0,
	RemotePeerLog,
	RemotePeerCmd,
};

struct RemoteDebuggingPeer
{
	RemotePeerType type;
	std::string typeDesc;
	Processing *pProc;
};

class GwMsgDispatching : public Processing
{

public:

	static GwMsgDispatching *create()
	{
		return new dNoThrow GwMsgDispatching;
	}

protected:

	GwMsgDispatching();
	virtual ~GwMsgDispatching() {}

private:

	GwMsgDispatching(const GwMsgDispatching &) = delete;
	GwMsgDispatching &operator=(const GwMsgDispatching &) = delete;

	/*
	 * Naming of functions:  objectVerb()
	 * Example:              peerAdd()
	 */

	/* member functions */
	Success process();
	Success shutdown();
	void processInfo(char *pBuf, char *pBufEnd);

	void stateOnlineCheckAndPrint();
	void onlinePrint(bool online = true);
	bool servicesStart();
	void peerListUpdate();
	void contentDistribute();
	void contentSend(const std::string &str, RemotePeerType typePeer);
	bool disconnectRequestedCheck(TcpTransfering *pTrans);
	void peerCheck();
	void peerAdd(TcpListening *pListener, enum RemotePeerType peerType, const char *pTypeDesc);

	/* member variables */
	//uint32_t mStartMs;
	uint16_t mPortStart;
	bool mListenLocal;
	TcpListening *mpLstProc;
	TcpListening *mpLstLog;
	TcpListening *mpLstCmd;
	SingleWireControlling *mpCtrl;
	InfoGathering *mpGather;
	bool mCursorHidden;
	bool mDevUartIsOnline;
	bool mTargetIsOnline;
	std::list<struct RemoteDebuggingPeer> mListPeers;

	/* static functions */

	/* static variables */

	/* constants */

};

#endif

