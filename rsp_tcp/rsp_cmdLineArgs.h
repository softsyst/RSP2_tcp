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
#include <map>
#include <string>
#include "mir_sdr.h"
#include "IPAddress.h"
using namespace std;

class rsp_cmdLineArgs
{
private:
	rsp_cmdLineArgs(){}
	int argc;
	char** argv;

	map < char, int> selectors;
	int intValue(int index, string error, int minval, int maxval);
	IPAddress* ipAddValue(int index, string error);

public:
	IPAddress  Address{ 127,0,0,1 };
	int Port = 7890;
	int Frequency = 178352000;

	//This is the requested Gain value, not the GainReduction value
	int Gain = 25;
	int SamplingRate = 2048000;
	int BitWidth = 2; //16 Bit
	mir_sdr_RSPII_AntennaSelectT Antenna = mir_sdr_RSPII_ANTENNA_A;
	int requestedDeviceIndex = 0;
	int verbose = 0;

	rsp_cmdLineArgs(int argc, char** argv);
	int parse();
	virtual ~rsp_cmdLineArgs();
	static void displayUsage();

};

