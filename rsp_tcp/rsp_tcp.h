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

enum eBitWidth { BITS_4= 0, BITS_8 = 1, BITS_16 = 2 };
enum eErrors
{
	 E_OK = 0
	,E_PARAMETER = -1
	,E_WRONG_API_VERSION = -2
	,E_NO_DEVICE = -3
	,E_DEVICE_INDEX = -4
	,E_WIN_WSA_STARTUP = -10
};

enum eRxType
{
	RSP1 = 0,
	RSP1A,
	RSP2,
	RSPduo,
	UNKNOWN
	//RSP1 = 6,
	//RSP1A = 7,
	//RSP2 = 8,
	//RSPduo = 9
};

