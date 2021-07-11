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
#include "devices.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct
	{
		mir_sdr_device *dev;
		int port;
		int wait;
		const char *addr;
		bool* pDoExit;
	}
	ctrl_thread_data_t;
	void *ctrl_thread_fn(void *arg);

#ifdef __cplusplus
}
#endif

