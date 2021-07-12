/**
** RSP2_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
** Copyright (C) 2017 Clem Schmidt, softsyst GmbH, http://www.softsyst.com
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
**/

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#else
#include <winsock2.h>
//#include "getopt/getopt.h"
#endif

#ifdef NEED_PTHREADS_WORKARROUND
#define HAVE_STRUCT_TIMESPEC
#endif
//#include <pthread.h>

#include "mir_sdr_device.h"

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")

typedef int socklen_t;

#else
#define closesocket close
#define SOCKADDR struct sockaddr
#define SOCKET int
#define SOCKET_ERROR -1
#endif

#define MAX_LEN  256
#define TX_BUF_LEN (32) //tbd

ctrl_thread_data_t ctrl_thread_data;

void *ctrl_thread_fn(void *arg)
{
	unsigned char reg_values[MAX_LEN];
	unsigned char txbuf[TX_BUF_LEN];
	int r = 1;
	struct timeval tv = { 1,0 };
	struct linger ling = { 1,0 };
	SOCKET listensocket;
	SOCKET controlSocket;
	int haveControlSocket = 0;
	struct sockaddr_in local, remote;
	socklen_t rlen;

	int len, result, total_gain=123;
	fd_set connfds;
	fd_set writefds;
	int bytesleft, bytessent, index;
	int old_gain = 0;

	ctrl_thread_data_t *data = (ctrl_thread_data_t *)arg;

	mir_sdr_device *dev = (mir_sdr_device *)data->dev;
	int port = data->port;
	int wait = data->wait;
	const char *addr = data->addr;
	bool* do_exit = data->pDoExit;
	u_long blockmode = 1;
	int retval;

	memset(reg_values, 0, MAX_LEN);

	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = inet_addr(addr);

	listensocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	setsockopt(listensocket, SOL_SOCKET, SO_REUSEADDR, (char *)&r, sizeof(int));
	setsockopt(listensocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
	retval = bind(listensocket, (struct sockaddr *)&local, sizeof(local));
	if (retval == SOCKET_ERROR)
		goto close;
#ifdef _WIN32
	ioctlsocket(listensocket, FIONBIO, &blockmode);
#else
	r = fcntl(listensocket, F_GETFL, 0);
	r = fcntl(listensocket, F_SETFL, r | O_NONBLOCK);
#endif

	while (1) {
		printf("\n\nlistening on Control port %d...\n", port);
		retval = listen(listensocket, 1);
		if (retval == SOCKET_ERROR)
			goto close;
		while (1) {
			FD_ZERO(&connfds);
			FD_SET(listensocket, &connfds);
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			r = select(listensocket + 1, &connfds, NULL, NULL, &tv);
			if (*do_exit) {
				goto close;
			}
			else if (r) {
				rlen = sizeof(remote);
				controlSocket = accept(listensocket, (struct sockaddr *)&remote, &rlen);
				haveControlSocket = 1;
				break;
			}
			result = 0;// rtlsdr_get_tuner_i2c_register(dev, reg_values, &len, &total_gain);
			total_gain = ((total_gain + 5) / 10)%256;
			if (old_gain != total_gain)
			{
				printf("\ngain = %2d dB\r", total_gain);
				old_gain = total_gain;
			}
		}

		setsockopt(controlSocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));

		printf("\nControl client accepted!\n");
		usleep(5000000);

		while (1) {


			/* @TODO: check if something else has to be transmitted */
			if (false)
				goto sleep;

			len = 0;
			total_gain = 123;
			result = 0;
			mir_sdr_GainValuesT gainVals;
			mir_sdr_ErrT err =  mir_sdr_GetCurrentGain(&gainVals);
			total_gain = (int)(gainVals.curr * 10.0f);
			//result = rtlsdr_get_tuner_i2c_register(dev, reg_values, &len, &total_gain);
			memset(txbuf, 0, TX_BUF_LEN);
			if (result)
				goto sleep;

			//Big Endian / Network Byte Order
			txbuf[0] = 0;// REPORT_I2C_REGS;
			txbuf[1] = ((len + 2) >> 8) & 0xff;
			txbuf[2] = (len + 2) & 0xff;
			txbuf[3] = (total_gain >> 8) & 0xff;
			txbuf[4] = total_gain & 0xff;
			/* now the message contents */
			//memcpy(&txbuf[5], reg_values, len);
			len += 5;

			/* now start (possibly blocking) transmission */
			bytessent = 0;
			bytesleft = len;
			index = 0;
			while (bytesleft > 0) {
				FD_ZERO(&writefds);
				FD_SET(controlSocket, &writefds);
				tv.tv_sec = 1;
				tv.tv_usec = 0;
				r = select(controlSocket + 1, NULL, &writefds, NULL, &tv);
				if (r) {
					bytessent = send(controlSocket, (const char*)&txbuf[index], bytesleft, 0);
					bytesleft -= bytessent;
					index += bytessent;
				}
				if (bytessent == SOCKET_ERROR || *do_exit) {
					goto close;
				}
			}
		sleep:
			usleep(wait);
		}
	close:
		if (haveControlSocket)
			closesocket(controlSocket);
		if (*do_exit)
		{
			closesocket(listensocket);
			printf("Control Thread terminates\n");
			break;
		}
	}
	return 0;
}
