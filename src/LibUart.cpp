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
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

#include "LibUart.h"

using namespace std;

static uint8_t uartVirtual = 0;
static uint8_t uartVirtualMounted = 0;

static uint8_t bufVirtual[31];
static uint8_t *pBufVirt = bufVirtual;
static size_t lenWritten = 0;

/*
 * Literature
 * - https://man7.org/linux/man-pages/man3/tcgetattr.3p.html
 * - https://man7.org/linux/man-pages/man2/write.2.html
 * - https://man7.org/linux/man-pages/man2/read.2.html
 */
Success devUartInit(const string &deviceUart, RefDeviceUart &refUart)
{
	if (uartVirtual)
		return uartVirtualMounted ? Positive : Pending;

#if defined(__unix__)
	refUart = open(deviceUart.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (refUart < 0)
		return Pending; // no error!

	struct termios toOld, toNew;
	Success success;
	int res;

	res = tcgetattr(refUart, &toOld);
	if (res < 0)
	{
		success = errLog(-1, "could not get terminal options");
		goto errInit;
	}

	toNew = toOld;

	// Disable CR to NL translation
	toNew.c_iflag &= ~ICRNL;

	// Disable NL to CR translation
	toNew.c_oflag &= ~ONLCR;

	// Disable echo and canonical mode
	toNew.c_lflag &= ~(ECHO | ICANON);

	// Clear baud rate and set extended baud rate
	toNew.c_cflag &= ~CBAUD;
	toNew.c_cflag |= CBAUDEX;

	// Set baud rate to 115200
	cfsetispeed(&toNew, B115200);
	cfsetospeed(&toNew, B115200);

	res = tcsetattr(refUart, TCSANOW, &toNew);
	if (res < 0)
	{
		(void)tcsetattr(refUart, TCSANOW, &toOld); // Restore old settings on failure

		success = errLog(-1, "could not set terminal options");
		goto errInit;
	}
#else
	(void)deviceUart;
	(void)refUart;
#endif
	return Positive;

errInit:
#if defined(__unix__)
	close(refUart);
#endif
	return success;
}

void devUartDeInit(RefDeviceUart &refUart)
{
	if (refUart == RefDeviceUartInvalid)
		return;

#if defined(__unix__)
	close(refUart);
#endif
	refUart = RefDeviceUartInvalid;
}

ssize_t uartSend(RefDeviceUart refUart, const void *pBuf, size_t lenReq)
{
	if (!lenReq)
		return -1;

	if (uartVirtual)
	{
		if (!uartVirtualMounted)
			return -1;

		lenWritten = PMIN(lenReq, sizeof(bufVirtual));
		pBufVirt = bufVirtual;

		memcpy(pBufVirt, pBuf, lenWritten);

		return lenWritten;
	}

#if defined(__unix__)
	lenWritten = write(refUart, pBuf, lenReq);
#else
	(void)pBuf;
	(void)lenReq;
#endif
	return lenWritten;
}

ssize_t uartSend(RefDeviceUart refUart, const string &str)
{
	return uartSend(refUart, str.data(), str.size());
}

ssize_t uartSend(RefDeviceUart refUart, uint8_t ch)
{
	return uartSend(refUart, &ch, sizeof(ch));
}

ssize_t uartRead(RefDeviceUart refUart, void *pBuf, size_t lenReq)
{
	if (!lenReq)
		return -1;

	if (uartVirtual)
	{
		if (!uartVirtualMounted)
			return -1;

		size_t lenRead;

		lenRead = PMIN(lenReq, lenWritten);
		memcpy(pBuf, pBufVirt, lenRead);

		lenWritten -= lenRead;
		pBufVirt += lenRead;

		return lenRead;
	}

	return read(refUart, pBuf, lenReq);
}

void uartVirtualSet(uint8_t enabled)
{
	uartVirtual = enabled;
}

void uartVirtualMountedSet(uint8_t mounted)
{
	uartVirtualMounted = mounted;
}

