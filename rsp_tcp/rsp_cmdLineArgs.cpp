//https://devblogs.microsoft.com/cppblog/standards-version-switches-in-the-compiler/#c++14
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
#include "IPAddress.h"
#include "rsp_cmdLineArgs.h"
#include "common.h"
#include <string>


rsp_cmdLineArgs::rsp_cmdLineArgs(int argc, char** argv)
{
	this->argc = argc;
	this->argv = argv;
	for (int i = 0; i < argc; i++)
	{
		string s = argv[i];
		int len = s.length();
		if (len == 2 && s[0] == '-')
		{
			selectors[s[1]] = i;
		}
	}
}


rsp_cmdLineArgs::~rsp_cmdLineArgs()
{
}

int rsp_cmdLineArgs::intValue(int index, string error, int minval, int maxval)
{
	int val;
	if (argc > index + 1)
	{
		string s = argv[index + 1];
		try
		{
			val = std::stoi(s);
			if (!common::checkRange(val, minval, maxval))
				throw msg_exception("Out of Range Error");
			return val;
		}
		catch (exception& )
		{
			std::cout << error << val << endl << endl;
		}
	}
	else
		std::cout << "Missing Argument" << endl << endl;
	return -1;
}

IPAddress* rsp_cmdLineArgs::ipAddValue( int index, string error)
{
	IPAddress*  ipadd = 0;
	if (argc > index + 1)
	{
		string s = argv[index + 1];
		ipadd = new IPAddress(s);
		if (ipadd->sIPAddress == s) // then parse was ok
			return ipadd;
		else
			std::cout << error << s << endl << endl;
	}
	else
		std::cout << "Missing Argument" << endl << endl;
	return 0;
}

void rsp_cmdLineArgs::displayUsage()
{
	cout << "Usage: \t[-a listen address, default is 127.0.0.1]" << endl;
	cout << "\t[-p listen port, default is 7890]" << endl;
	cout << "\t[-f frequency [Hz], default is 178352000Hz]" << endl;
	cout << "\t[-s sampling rate [Hz], allowed values are 512000, 1024000, 2048000, 4096000, 8192000, default is 2048000]" << endl;
	cout << "\t[-g gain value, initial value betwee 20 and 80, default is 25]" << endl;
	cout << "\t[-W bit width, value of 1 means 8 bit, value of 2 means 16 bit, default is 16 bit]" << endl;
	cout << "\t[-d device index, value counts from 0 to number of devices -1, default is 0]" << endl;
	cout << "\t[-T antenna, value of 1 means Antenna A, value of 2 means Antenna B, default is Antenn A]" << endl;
	cout << "\t[-v verbose mode, outputs many values on the command line window]" << endl;
	cout << "\t[-H High Impedance antenna port on RSP2 or RSPduo, 1 on, 0 off]" << endl;
}



int rsp_cmdLineArgs::parse()
{
	map<char, int>::iterator it;
	if (argc == 2 && argv[1][0] == '?')
		goto exit;

	for (it = selectors.begin(); it != selectors.end(); it++)
	{
		IPAddress* ipa = 0;
		switch (it->first) //key
		{
		case 's':
			SamplingRate = intValue(it->second, "Invalid Sampling Rate ", 512000, 8192000);
			if (SamplingRate == -1)
				goto exit;
			break;
		case 'p':
			Port = intValue(it->second, "Invalid Port Number ", 0, 0xffff);
			if (Port == -1)
				goto exit;
			break;
		case 'f':
			Frequency = intValue(it->second, "Invalid Frequency ", 0, 0x7fffffff);
			if (Frequency == -1)
				goto exit;
			break;
		case 'g':
			Gain = intValue(it->second, "Invalid Gain ", 0, 99);
			if (Gain == -1)
				goto exit;
			break;
		case 'W':
			BitWidth = intValue(it->second, "Invalid Bit Width ", 0, 2);
			if (BitWidth == -1)
				goto exit;
			break;
		case 'a':
			ipa = ipAddValue(it->second, "Invalid IP Address ");
			if (ipa == 0)
				goto exit;
			Address = *ipa;
			break;
		case 'v':
			verbose = intValue(it->second, "Invalid verbose value ", 0, 1);
			if (verbose == -1)
				goto exit;
			break;
		case 'H':
			amPort = intValue(it->second, "Invalid Antenna Value ", 0, 1);
			break;
		case 'T':
			Antenna = static_cast<mir_sdr_RSPII_AntennaSelectT>(intValue(it->second, "Invalid Antenna Value ", 1, 2) + 4);
			break;
		case 'd':
			requestedDeviceIndex = intValue(it->second, "Invalid Device Index requested  ", 0, 8);
			if (requestedDeviceIndex == -1)
				goto exit;
			break;
		case 'h':
			goto exit;
		case '?':
			goto exit;
		default:
			cout << "Invalid argument -" + it->second;
			goto exit;

		}
	}
	return 0;
	exit:
		return -1;
}