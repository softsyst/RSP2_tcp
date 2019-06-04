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
#include "common.h"

struct IPAddress
{
	std::string sIPAddress;
	unsigned char b1, b2, b3, b4;
	bool valid;

	IPAddress(const std::string& s);
	IPAddress(BYTE a1, BYTE a2, BYTE a3, BYTE a4);
	IPAddress(const IPAddress& src);
	IPAddress& operator= (const IPAddress& other);
	bool operator==(const IPAddress& other) const;
	bool checkRange(int val) const;
	void parse(const std::string& s);
};
