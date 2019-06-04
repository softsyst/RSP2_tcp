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
#include "rsp_tcp.h"
#include "mir_sdr.h"
#include "syTwoDimArray.h"

struct gainConfiguration
{
	static void createGainConfigTables();

	const int internalBands = 7;
	// get the band id of the matrix tables
	// the AM(HiZ Port) is not covered by mir_sdr_BandT
	const int bandId[(int)mir_sdr_BAND_L + 2] = { 0,0,0, 1,2,3,4,5,6 };
	int LNAstates[4][7] =    //number of LNAstates depending on rxType and band
	{
		{4,4,4,4,4,4,0},
		{7,10,10,10,10, 9, 0},
		{9,9,9,9,6,6,5},
		{7,10,10,10,10,9,5}
	};

	mir_sdr_BandT band;

	// the internally used band, converted from band
	int myBand;

	gainConfiguration(mir_sdr_BandT band);

	bool calculateGrValues(int flatValue, int rxtype, int& LNAstate, int& gr);
};




