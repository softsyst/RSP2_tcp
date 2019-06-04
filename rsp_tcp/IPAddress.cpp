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

#include "IPAddress.h"
#include <iostream>
using namespace std;

IPAddress::IPAddress(const string& s)
{
	try
	{
		parse(s);
		sIPAddress = s;
		valid = true;
	}
	catch (const std::exception& e)
	{
		cout << e.what() << endl << endl;
		b1 = b2 = b3 = 0;
		valid = false;
	}
}
IPAddress::IPAddress(BYTE a1, BYTE a2, BYTE a3, BYTE a4)
{
	valid = false;
	if (checkRange(a1) && checkRange(a2) && checkRange(a3) && checkRange(a4))
	{
		b1 = a1; b2 = a2; b3 = a3; b4 = a4;
		sIPAddress = to_string(b1) + "." + to_string(b2) + "." + to_string(b3) + "." + to_string(b4);
		valid = true;
	}
}

IPAddress::IPAddress(const IPAddress& src)
{
	sIPAddress = src.sIPAddress;
	b1 = src.b1; b2 = src.b2; b3 = src.b3; b4 = src.b4;
}

bool IPAddress::operator==(const IPAddress& other) const
{
	if (this == &other)
		return true;
	if (this->b1 == other.b1 && this->b2 == other.b2 && this->b3 == other.b3 && this->b4 == other.b4 &&
		this->sIPAddress == other.sIPAddress)
		return true;
	return false;
}

IPAddress& IPAddress::operator= (const IPAddress& other)
{
	if ( *this == other)
		return *this;
	sIPAddress = other.sIPAddress;
	b1 = other.b1; b2 = other.b2; b3 = other.b3; b4 = other.b4;
	return *this;
}

bool IPAddress::checkRange(int val) const
{
	return common::checkRange(val, 0, 255);
}

void IPAddress::parse(const string& s)
{
	{
		vector <string> parts = common::split(s, '.');
		if (parts.size() != 4)
			throw rsptcpException("IPAddress parsing error.");

		int val = stoi(parts[0]);
		if (checkRange(val))
			b1 = (unsigned char)val;
		else
			throw rsptcpException("IPAddress out of range error.");

		val = stoi(parts[1]);
		if (checkRange(val))
			b2 = (unsigned char)val;
		else
			throw rsptcpException("IPAddress out of range error.");

		val = stoi(parts[2]);
		if (checkRange(val))
			b3 = (unsigned char)val;
		else
			throw rsptcpException("IPAddress out of range error.");

		val = stoi(parts[3]);
		if (checkRange(val))
			b4 = (unsigned char)val;
		else
			throw rsptcpException("IPAddress range error.");
	}
}
