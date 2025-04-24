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
#if defined(_WIN32)
#include <winsock2.h>
#endif

#include "LibUart.h"

using namespace std;

uint8_t uartVirtualMode = 0; // swart
uint8_t uartVirtual = 0;
uint8_t uartVirtualMounted = 0;

static uint8_t bufVirtual[31];
static size_t lenWritten = 0;
static uint8_t *pBufVirt = bufVirtual;

/*
 * Literature
 *
 * Linux
 * - https://man7.org/linux/man-pages/man3/tcgetattr.3p.html
 * - https://man7.org/linux/man-pages/man2/write.2.html
 * - https://man7.org/linux/man-pages/man2/read.2.html
 *
 * Windows
 * - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-createfilea
 * - https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getcommstate
 * - https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcommstate
 * - https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-closehandle
 */
Success devUartInit(const string &deviceUart, RefDeviceUart &refUart)
{
	refUart = RefDeviceUartInvalid;

	if (uartVirtual)
		return uartVirtualMounted ? Positive : Pending;

	Success success;
	string fileUart;

	// create reference
#if defined(__unix__)
	fileUart = deviceUart;

	refUart = open(fileUart.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
#else
	fileUart += "\\\\.\\";
	fileUart += deviceUart;

	refUart = CreateFileA(
		fileUart.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
	);
#endif
	if (refUart == INVALID_HANDLE_VALUE)
		return Pending; // no error!

	// configuration
#if defined(__unix__)
	struct termios toOld, toNew;
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

	// Disable flow control
	toNew.c_iflag &= ~(IXON | IXOFF | IXANY);

	// Disable NL to CR translation
	toNew.c_oflag &= ~ONLCR;

	// Disable echo and canonical mode
	toNew.c_lflag &= ~(ECHO | ICANON);

	// Set baud rate to 115200
	cfsetispeed(&toNew, B115200);
	cfsetospeed(&toNew, B115200);

	res = tcsetattr(refUart, TCSANOW, &toNew);
	if (res < 0)
	{
		success = errLog(-1, "could not set terminal options");
		goto errInit;
	}
#else
	DCB dcbSerialParams = {};
	bool ok;

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	ok = GetCommState(refUart, &dcbSerialParams);
	if (!ok)
	{
		success = errLog(-1, "could not get serial port state");
		goto errInit;
	}

	dcbSerialParams.BaudRate = CBR_115200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
	dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
	dcbSerialParams.fOutxCtsFlow = FALSE;
	dcbSerialParams.fOutxDsrFlow = FALSE;
	dcbSerialParams.fDsrSensitivity = FALSE;
	dcbSerialParams.fTXContinueOnXoff = TRUE;
	dcbSerialParams.fOutX = FALSE;
	dcbSerialParams.fInX = FALSE;

	ok = SetCommState(refUart, &dcbSerialParams);
	if (!ok)
	{
		success = errLog(-1, "could not set serial port state");
		goto errInit;
	}
#endif
	return Positive;

errInit:
	if (refUart == RefDeviceUartInvalid)
		return success;
#if defined(__unix__)
	close(refUart);
#else
	CloseHandle(refUart);
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

		size_t lenAttemted = PMIN(lenReq, sizeof(bufVirtual));
		*bufVirtual = 0;

		if (uartVirtualMode) // mode = uart: TX not connected to RX
			return lenAttemted;

		lenWritten = lenAttemted;
		pBufVirt = bufVirtual;

		memcpy(pBufVirt, pBuf, lenWritten);

		return lenWritten;
	}

	if (refUart == RefDeviceUartInvalid)
		return -1;

#if defined(__unix__)
	lenWritten = write(refUart, pBuf, lenReq);
#else
	(void)pBuf;
	(void)lenReq;
#endif
	return lenWritten;
}

ssize_t uartSend(RefDeviceUart refUart, uint8_t ch)
{
	return uartSend(refUart, &ch, sizeof(ch));
}

static int errGet()
{
#ifdef _WIN32
	return WSAGetLastError();
#else
	return errno;
#endif
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
		if (!lenRead)
			return 0;

		memcpy(pBuf, pBufVirt, lenRead);

		lenWritten -= lenRead;
		pBufVirt += lenRead;

		return lenRead;
	}

	if (refUart == RefDeviceUartInvalid)
		return -1;

	ssize_t lenRead;

#if defined(__unix__)
	lenRead = read(refUart, pBuf, lenReq);
	if (!lenRead)
		return -2;
#else
	lenRead = -1;
	return lenRead;
#endif
	if (lenRead < 0)
	{
		int numErr = errGet();
#ifdef _WIN32
		if (numErr == WSAEWOULDBLOCK || numErr == WSAEINPROGRESS)
			return 0; // std case and ok

		if (numErr == WSAECONNRESET)
			return -2;
#else
		if (numErr == EWOULDBLOCK || numErr == EINPROGRESS || numErr == EAGAIN)
			return 0; // std case and ok

		if (numErr == ECONNRESET)
			return -2;
#endif
	}

	return lenRead;
}

ssize_t uartVirtRcv(RefDeviceUart refUart, const void *pBuf, size_t lenReq)
{
	(void)refUart;

	if (!uartVirtual)
		return -1;

	lenWritten = PMIN(lenReq, sizeof(bufVirtual));
	pBufVirt = bufVirtual;

	memcpy(pBufVirt, pBuf, lenWritten);

	return lenWritten;
}

