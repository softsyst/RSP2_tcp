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

/**
** The interface and the mirics API Library is Copyright SDRplay Ltd.
** For the sdrplay Copyright, see the file 

       SDRplay_RSP_API_Release_Notes_V2.13.pdf

**/

#include <iostream>
#include <stdio.h>
#include "common.h"
#include "rsp_tcp.h"
#include "rsp_cmdLineArgs.h"
#include "devices.h"
#include "sdrGainTable.h"
#include "syTwoDimArray.h"
#ifndef _WIN32
#include <signal.h>
#endif
using namespace std;

// V0.9.8	Sampling rate 2.000 for ADS-B
// V0.9.10	Bitwidth 8 Bit corrected
//          VERBOSE  output removable
//          sleep in Set Frequency removed
string Version = "0.9.11";

map<eErrors, string> returnErrorStrings =
{
	 { E_OK, "OK"}
    ,{ E_PARAMETER, "Starting Parameter Error"}
	,{ E_WRONG_API_VERSION, "Wrong or incompatible sdrplay API Version. Must be at least 2.13 and less than 3."}
	,{ E_NO_DEVICE, "No sdrplay device found."}
	,{ E_DEVICE_INDEX, "Requested Device Index not present: "}
	,{ E_WIN_WSA_STARTUP, "WSAStartup failed with error:  "}
};

#ifdef _WIN32
BOOL CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
		// Handle the CTRL-C signal only. 
	case CTRL_C_EVENT:
		printf("Shutdown by ctrl-c\n\n");
		devices::instance().Stop();
		exit(0);
/*
		// CTRL-CLOSE: confirm that the user wants to exit. 
	case CTRL_CLOSE_EVENT:
		Beep(600, 200);
		printf("Ctrl-Close event\n\n");
		return(TRUE);

		// Pass other signals to the next handler. 
	case CTRL_BREAK_EVENT:
		Beep(900, 200);
		printf("Ctrl-Break event\n\n");
		return FALSE;

	case CTRL_LOGOFF_EVENT:
		Beep(1000, 200);
		printf("Ctrl-Logoff event\n\n");
		return FALSE;

	case CTRL_SHUTDOWN_EVENT:
		Beep(750, 500);
		printf("Ctrl-Shutdown event\n\n");
		return FALSE;
*/
	default:
		return FALSE;
	}
}
#else
static void sighandler(int signum)
{
	printf("Shutdown \n\n");
	//devices::instance().Stop();
	exit(0);
}
#endif


int main(int argc, char* argv[])
{
	//test
	float a = sqrt(2.0f);

	//test end
	rsp_cmdLineArgs* pargs = 0;
	eErrors retCode = E_OK;
	string sError;
#ifdef _WIN32
	WSADATA wsd;
	int result = WSAStartup(MAKEWORD(2, 2), &wsd);
	if (result != 0)
	{
		retCode = E_WIN_WSA_STARTUP;
		sError = returnErrorStrings[retCode] + to_string(result);
		goto exit;
	}
	if (SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandler, TRUE) == FALSE)
		cout << "Error setting ctrl-c handler." << endl;
#else
	struct sigaction sigact, sigign;
	sigact.sa_handler = sighandler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigign.sa_handler = SIG_IGN;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
	sigaction(SIGQUIT, &sigact, NULL);
	sigaction(SIGPIPE, &sigign, NULL);
#endif

	std::cout << "\nRSP_tcp V" + Version << std::endl;
	std::cout << "Copyright (c) Clem Schmidt, qirx.softsyst.com. All rights reserved" << endl;
	std::cout << endl;

	pargs = new rsp_cmdLineArgs(argc, argv);
	if (pargs->parse() != 0)
	{
		pargs->displayUsage();
		retCode = E_PARAMETER;
		sError = returnErrorStrings[retCode];
		goto exit;
	}
	std::cout << "IP Address = " + pargs->Address.sIPAddress << endl;
	std::cout << "Port Number = " + to_string(pargs->Port) << endl;
	std::cout << "Sampling Rate = " + to_string(pargs->SamplingRate) << endl;
	std::cout << "Frequency = " + to_string(pargs->Frequency) << endl;
	std::cout << "Gain = " + to_string(pargs->Gain) << endl;
	std::cout << "BitWidth = " + to_string(pargs->BitWidth) << endl;
	std::cout << "Device Index = " + to_string(pargs->requestedDeviceIndex) << endl;
	std::cout << "Antenna = " + to_string(pargs->Antenna) << endl;
	std::cout << "Verbose = " + to_string(pargs->verbose) << endl;

	cout << "\nStarting sdrplay...\n";

	gainConfiguration::createGainConfigTables();

	if (devices::instance().getDevices())
	{
		float apiVersion = 0.0f;
		mir_sdr_ErrT err = mir_sdr_ApiVersion(&apiVersion);
		cout << "\nmir_sdr_ApiVersion returned with: " << err << endl;
		cout << "sdrplay API Version " << apiVersion << endl << endl;
		
		double epsilon = 0.0001;

		if (apiVersion < 2.13 -epsilon && apiVersion > 2.13 + epsilon)
		{
			retCode = E_WRONG_API_VERSION;
			sError = returnErrorStrings[retCode];
			goto exit;
		}

		int numDevices = devices::instance().getNumberOfDevices();
		cout << numDevices << " Device(s) found\n";
		if (pargs->requestedDeviceIndex + 1 > numDevices)
		{
			retCode = E_DEVICE_INDEX;
			sError = returnErrorStrings[retCode] + to_string( pargs->requestedDeviceIndex);
			goto exit;
		}

		map<string, mir_sdr_device*>::iterator it;
		for (it = devices::instance().mirDevices.begin();
			it != devices::instance().mirDevices.end(); it++)
		{
			mir_sdr_device* pd = it->second;
			cout << "\tSerial: " << pd->serno << endl;
			cout << "\tUSB Id: " << pd->DevNm << endl;
			cout << "\tHardware Version: " << (int)pd->hwVer << endl;
			cout << "\n";
		}
		//err = mir_sdr_DebugEnable(1);
		//cout << "mir_sdr_DebugEnable(1) returned with " << err << endl;
		devices::instance().Start(pargs);
	}
	else
	{
		retCode = E_NO_DEVICE;
		sError = returnErrorStrings[retCode];
		goto exit;
	}
exit:
	if (retCode != 0)
	{
		delete pargs;
		cout << sError << endl;
		cout << "Application cannot continue. \n" << endl;
		//cout << "Please press any character to exit here..." << endl;
		//getchar();
	}
#ifdef _WIN32
	WSACleanup();
#endif
	return retCode;

}