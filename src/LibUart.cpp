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

#if defined(_WIN32)
#include <winsock2.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
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
 * - https://learn.microsoft.com/sr-cyrl-rs/windows/win32/api/winbase/nf-winbase-setcommtimeouts
 */
Success devUartInit(const string &deviceUart, RefDeviceUart &refUart)
{
	refUart = RefDeviceUartInvalid;

	if (uartVirtual)
		return uartVirtualMounted ? Positive : Pending;

	Success success;
	string fileUart;

	// create reference
#if defined(_WIN32)
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
#else
	fileUart = deviceUart;

	refUart = open(fileUart.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
#endif
	if (refUart == RefDeviceUartInvalid)
		return Pending; // no error!

	// configuration
#if defined(_WIN32)
	DCB dcbSerialParams = {};
	COMMTIMEOUTS timeouts = {};
	BOOL ok;

	dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

	ok = GetCommState(refUart, &dcbSerialParams);
	if (!ok)
	{
		//wrnLog("could not get serial port state");

		success = -1;
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
		//wrnLog("could not set serial port state");

		success = -1;
		goto errInit;
	}

	// set non-blocking
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutConstant = 0;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	ok = SetCommTimeouts(refUart, &timeouts);
	if (!ok)
	{
		//wrnLog("could not set serial port timeouts");

		success = -1;
		goto errInit;
	}

	GetCommState(refUart, &dcbSerialParams);
	dbgLog("Baud %lu, ByteSize %d, StopBits %d, Parity %d",
		dcbSerialParams.BaudRate,
		dcbSerialParams.ByteSize,
		dcbSerialParams.StopBits,
		dcbSerialParams.Parity);

	GetCommTimeouts(refUart, &timeouts);
	dbgLog("ReadIntervalTimeout          %lu ms", timeouts.ReadIntervalTimeout);
	dbgLog("ReadTotalTimeoutMultiplier   %lu ms/Byte", timeouts.ReadTotalTimeoutMultiplier);
	dbgLog("ReadTotalTimeoutConstant     %lu ms", timeouts.ReadTotalTimeoutConstant);
	dbgLog("WriteTotalTimeoutMultiplier  %lu ms/Byte", timeouts.WriteTotalTimeoutMultiplier);
	dbgLog("WriteTotalTimeoutConstant    %lu ms", timeouts.WriteTotalTimeoutConstant);
#else
	struct termios toOld, toNew;
	int res;

	res = tcgetattr(refUart, &toOld);
	if (res < 0)
	{
		//wrnLog("could not get terminal options");

		success = -1;
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
		//wrnLog("could not set terminal options");

		success = -1;
		goto errInit;
	}
#endif
	return Positive;

errInit:
	if (refUart == RefDeviceUartInvalid)
		return success;
#if defined(_WIN32)
	CloseHandle(refUart);
#else
	close(refUart);
#endif
	return success;
}

void devUartDeInit(RefDeviceUart &refUart)
{
	if (refUart == RefDeviceUartInvalid)
		return;

#if defined(_WIN32)
	CloseHandle(refUart);
#else
	close(refUart);
#endif
	refUart = RefDeviceUartInvalid;
}

/*
 * Literature
 *
 * Linux
 * - https://man7.org/linux/man-pages/man2/write.2.html
 *
 * Windows
 * - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-writefile
 */
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

#if defined(_WIN32)
	DWORD lenWrittenWin;
	BOOL ok;

	lenWritten = 0;

	ok = WriteFile(refUart, pBuf, (DWORD)lenReq, &lenWrittenWin, NULL);
	if (!ok)
		return -1;

	lenWritten = (size_t)lenWrittenWin;
#else
	lenWritten = write(refUart, pBuf, lenReq);
#endif
	return lenWritten;
}

ssize_t uartSend(RefDeviceUart refUart, uint8_t ch)
{
	return uartSend(refUart, &ch, sizeof(ch));
}

/*
 * Literature
 *
 * Linux
 * - https://man7.org/linux/man-pages/man2/read.2.html
 *
 * Windows
 * - https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-readfile
 */
ssize_t uartRead(RefDeviceUart refUart, void *pBuf, size_t lenReq)
{
	if (!lenReq)
		return -1;

	if (uartVirtual)
	{
		if (!uartVirtualMounted)
			return -1;

		size_t lenReadVirt;

		lenReadVirt = PMIN(lenReq, lenWritten);
		if (!lenReadVirt)
			return 0;

		memcpy(pBuf, pBufVirt, lenReadVirt);

		lenWritten -= lenReadVirt;
		pBufVirt += lenReadVirt;

		return lenReadVirt;
	}

	if (refUart == RefDeviceUartInvalid)
		return -1;

	ssize_t lenRead;

#if defined(_WIN32)
	DWORD lenReadWin;
	BOOL ok;

	ok = ReadFile(refUart, pBuf, (DWORD)lenReq, &lenReadWin, NULL);
	if (!ok)
	{
		int numErr = errno;

		if (numErr == ERROR_IO_PENDING ||
				numErr == WSAEWOULDBLOCK ||
				numErr == WSAEINPROGRESS)
			return 0; // std case and ok

		return -2;
	}

	lenRead = (ssize_t)lenReadWin;
#else
	lenRead = read(refUart, pBuf, lenReq);
	if (lenRead < 0)
	{
		int numErr = errno;

		if (numErr == EWOULDBLOCK ||
				numErr == EINPROGRESS ||
				numErr == EAGAIN)
			return 0; // std case and ok

		return -2;
	}

	if (!lenRead)
		return -2;
#endif
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

