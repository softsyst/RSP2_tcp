/**
** RSP_tcp - TCP/IP I/Q Data Server for the sdrplay RSP2
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

#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
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
//#include <ws2tcpip.h>
#include <time.h>
#endif
	typedef unsigned char BYTE;

#ifdef _WIN32
#pragma comment(lib, "ws2_32.lib")

	typedef int socklen_t;

#else
#define closesocket close
#define SOCKADDR struct sockaddr
#define SOCKET int
#define INVALID_SOCKET          ((SOCKET)(~0))
#define SOCKET_ERROR -1
#endif


class common
{
private:
	template<typename Out>
	static void split(const std::string &s, char delim, Out result);
public:
	static std::vector<std::string> split(const std::string &s, char delim);
	static bool checkRange(int val, int minval, int maxval);

	static bool isLittleEndian();
	static timespec getRelativeTimeoutValue(int relativeTimeoutSec);
	#ifdef _WIN32
	static int gettimeofday(struct timeval *tv, void* ignored);
	#endif
	
	static std::string getSocketErrorString()
	{
		#ifdef _WIN32
		int err = WSAGetLastError();
			return std::to_string(err);
		#else
			return std::to_string(errno);
		#endif
	}
};

class msg_exception : public std::exception
{
public:
  msg_exception(std::string const &message) : msg_(message) { }
  virtual char const *what() const noexcept { return msg_.c_str(); }

private:
  std::string msg_;
};

class rsptcpException : public msg_exception
{
public :
	rsptcpException(const char* msg) : msg_exception(std::string(msg)) {}
};

