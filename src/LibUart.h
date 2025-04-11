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

#ifndef LIB_UART_H
#define LIB_UART_H

#include <cinttypes>
#include <string>

#include "Processing.h"

#if defined(__unix__)
typedef int RefDeviceUart;
#define RefDeviceUartInvalid -1
#elif defined(_WIN32)
typedef int RefDeviceUart;
#define RefDeviceUartInvalid -1
#else
typedef int RefDeviceUart;
#define RefDeviceUartInvalid -1
#endif

extern uint8_t uartVirtualMode;
extern uint8_t uartVirtual;
extern uint8_t uartVirtualMounted;

Success devUartInit(const std::string &deviceUart, RefDeviceUart &refUart);
void devUartDeInit(RefDeviceUart &refUart);

ssize_t uartSend(RefDeviceUart refUart, const void *pBuf, size_t lenReq);
ssize_t uartSend(RefDeviceUart refUart, uint8_t ch);
ssize_t uartRead(RefDeviceUart refUart, void *pBuf, size_t lenReq);
ssize_t uartVirtRcv(RefDeviceUart refUart, const void *pBuf, size_t lenReq);

#endif

