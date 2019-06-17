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
#ifdef _WIN32
#pragma warning(disable:4996)
#endif
#include <map>
#include "rsp_tcp.h"
#include "common.h"
#include "IPAddress.h"
#include "mir_sdr_device.h"
#include "mir_sdr.h"
#include "rsp_cmdLineArgs.h"

class devices
{

	// Singleton pattern
private:
	devices() {}
	devices(devices const&);				// Don't Implement
	void operator=(devices const&);			// Don't implement
public:
	static devices& instance()
	{
		static devices    instance; // Guaranteed to be destroyed.
									// Instantiated on first use.
		return instance;
	}
	//devices(devices const&)			= delete;	
	//void operator=(devices const&)	= delete;	
	mir_sdr_device* findFreeDevice() ;
	mir_sdr_device* findRequestedDevice(int rqIdx);
	void setDeviceIdle(int rqIdx);
	void Start(rsp_cmdLineArgs*  pargs);
	void Stop();
	void doListen();
	bool getDevices() ;
	int getNumberOfDevices() const { return mirDevices.size(); }

private:
	void initListener();

	mir_sdr_device* currentDevice;
	SOCKET clientSocket = INVALID_SOCKET;
	SOCKET listenSocket = INVALID_SOCKET;
	sockaddr_in local;
	sockaddr_in remote;
	struct addrinfo *result = NULL;
	//struct addrinfo hints;

	// command line arguments
	rsp_cmdLineArgs* pargs;

	// default values, may be overridden by command line arguments
    IPAddress  listenerAddress = IPAddress(0,0,0,0);
	int listenerPort = 7890;

public:

	/// <summary>
	/// The list of all RSP2 devices
	/// Key: Serial Number, Value: Object
	/// </summary>
	map<string, mir_sdr_device*> mirDevices;
};

