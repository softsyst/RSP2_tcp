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

#include <iostream>
#include <string>
#include<thread>
#include "devices.h"
#ifndef _WIN32
#include <netdb.h>
#endif

using namespace std;


// Loops endless until Stop
void devices::Start(rsp_cmdLineArgs*  pargs)
{
	this->pargs = pargs;
	try
	{
		listenerAddress = pargs->Address;
		listenerPort = pargs->Port;
		initListener();
		doListen();

		if (getNumberOfDevices() < 1)
			throw msg_exception("No sdrplay devices present.");
	}
	catch (const std::exception& e)
	{
		cout << "Cannot start listener: " << e.what() << endl;
	}
}


// The stop command for the server, not the device
// Intent is to stop all devices and release everything
void devices::Stop()
{

	map<string, mir_sdr_device*>::iterator it;

	for (it = mirDevices.begin(); it != mirDevices.end(); it++)
	{
		mir_sdr_device* pd = it->second;
		//pd->stop();
		delete pd;
	}
	closesocket(listenSocket);
	listenSocket = INVALID_SOCKET;
}

mir_sdr_device* devices::findFreeDevice() 
{
	map<string, mir_sdr_device*>::iterator it;

	for (it = mirDevices.begin(); it != mirDevices.end(); it++)
	{
		mir_sdr_device* pd = it->second;
		if (!pd->started && pd->devAvail)
			return pd;
	}
	return 0;
}

mir_sdr_device* devices::findRequestedDevice(int rqIdx) 
{
	map<string, mir_sdr_device*>::iterator it;

	for (it = mirDevices.begin(); it != mirDevices.end(); it++)
	{
		mir_sdr_device* pd = it->second;
		if (!pd->started && pd->devAvail && pd->DeviceIndex == rqIdx)
			return pd;
	}
	return 0;
}



void devices::doListen()
{
	mir_sdr_ErrT err;
	try
	{
		devices* d = this;
		int maxConnections = 1;
		SOCKET sock = listenSocket;
		int res = listen(sock, maxConnections);
		if (res == SOCKET_ERROR)
			throw msg_exception(common::getSocketErrorString());
			
		while (sock != INVALID_SOCKET)
		{
			currentDevice = 0;
			cout << "Listening to " << listenerAddress.sIPAddress << ":" << to_string(d->listenerPort) << endl;
			socklen_t rlen = sizeof(remote);
			clientSocket = accept(sock, (struct sockaddr *)&remote, &rlen);

			cout << "Client Accepted!\n" << endl;

			if (mirDevices.size() != 0)
			{
				if (!getDevices())
				{
					cout << "getDevices failed. Cannot continue.\n";
					break;
				}
				//mir_sdr_device* pd = findFreeDevice();
				//if (pd == 0)
				//	cout << "No free device available.\n";
				mir_sdr_device* pd = findRequestedDevice(pargs->requestedDeviceIndex);
				if (pd == 0)
					cout << "Requested Device " << pargs->requestedDeviceIndex << " not available.\n";
				else
				{
					currentDevice = pd;
					err = mir_sdr_SetDeviceIdx(pd->DeviceIndex);
					cout << "mir_sdr_SetDeviceIdx " << pd->DeviceIndex << " returned with: " << err << endl;

					if (err == mir_sdr_Success)
					{
						cout << "Device Index" << pd->DeviceIndex << "successfully set" << endl;
						pd->init(pargs);
						pd->start(clientSocket);

						void* status;
						pthread_join(*pd->thrdRx, &status);
						cout << endl << "++++ Rx thread terminated ++++" << endl;
						delete pd->thrdRx;
						pd->thrdRx = 0;
						pd->stop();
					}
				}
			}
		}
	}
	catch (exception& e)
	{
		cout << "Error starting listener: " << e.what();
	}
}

void devices::initListener()
{
	memset(&local, 0, sizeof(local));
	local.sin_family = AF_INET;
	local.sin_port = htons(listenerPort);
	local.sin_addr.s_addr = inet_addr(listenerAddress.sIPAddress.c_str());

	listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSocket == INVALID_SOCKET)
		throw msg_exception("INVALID_SOCKET");
	int r = 1;
	struct linger ling = { 1,0 };

	int res = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&r, sizeof(int));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());

	res = setsockopt(listenSocket, SOL_SOCKET, SO_LINGER, (char *)&ling, sizeof(ling));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());
	
	res = ::bind(listenSocket, (struct sockaddr *)&local, sizeof(local));
	if (res == SOCKET_ERROR)
		throw msg_exception(common::getSocketErrorString().c_str());
}
/// <summary>
/// Collect all sdrplay devices
/// </summary>
/// <returns></returns>
bool devices::getDevices()
{
	const int c_maxDevices = 5;
	mir_sdr_DeviceT mydevices[c_maxDevices] ;
	try
	{
		unsigned int numDevs = 0;
		{
			mir_sdr_ErrT err;
			err = mir_sdr_GetDevices(mydevices, &numDevs, c_maxDevices);
			if (numDevs == 0)
				return false;

			int ierr = (int)err;
			string error = "mir_sdr_device failed with error :" + to_string(ierr);
			cout << "mir_sdr_GetDevices returned with: " << err << endl;
			for (int i = 0; i < (int)numDevs; i++)
			{
				if (err != mir_sdr_Success)
					throw msg_exception(error.c_str());

				mir_sdr_device* pd = new mir_sdr_device();
				if ((mydevices[i].hwVer < 1 || mydevices[i].hwVer > 3) && mydevices[i].hwVer != 255 )
				{
					printf("Unknown Hardware version %d .\n", mydevices[i].hwVer);
					continue;
				}
				pd->devAvail = mydevices[i].devAvail == 1;
				pd->hwVer = mydevices[i].hwVer;
				pd->serno = mydevices[i].SerNo;
				pd->DevNm = mydevices[i].DevNm;
				pd->DeviceIndex = (unsigned int)i;
				if (mirDevices.count(pd->serno) == 0)
					mirDevices.insert (pair<string, mir_sdr_device*>(pd->serno, pd));
				else
				{
					mirDevices[pd->serno]->devAvail = pd->devAvail;
					mirDevices[pd->serno]->hwVer = pd->hwVer;
					mirDevices[pd->serno]->DevNm = pd->DevNm;
					mirDevices[pd->serno]->DeviceIndex = pd->DeviceIndex;
					delete pd;
				}
			}
		}
	}
	catch (exception& e)
	{
		cout << "Error reading devices: " << e.what() << endl;
		return false;
	}
	return true;
}
