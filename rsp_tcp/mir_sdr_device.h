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
#include "common.h"
#include "IPAddress.h"
#include "mir_sdr.h"
#include "rsp_cmdLineArgs.h"
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>
#ifdef _WIN32
#define sleep(n) Sleep(n*1000)
#define usleep(n) Sleep(n/1000)
#else
#include <string.h> //memset etc.
#include <unistd.h>
#endif
using namespace std;

void* receive(void* md);
void streamCallback(short *xi, short *xq, unsigned int firstSampleNum,
	int grChanged, int rfChanged, int fsChanged, unsigned int numSamples,
	unsigned int reset, unsigned int hwRemoved, void *cbContext);

void gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void* cbContext);

struct samplingConfiguration
{
	const int samplingRateHz;
	const int deviceSamplingRateHz;
	const mir_sdr_Bw_MHzT bandwidth;
	const int decimationFactor;
	const bool doDecimation;

	samplingConfiguration(int srHz, int devSrHz, mir_sdr_Bw_MHzT bw, int decimFact, bool doDecim)
	  : samplingRateHz(srHz), 
		deviceSamplingRateHz(devSrHz),
		bandwidth(bw),
		decimationFactor(decimFact),
		doDecimation(doDecim)
	{
	}
};


class mir_sdr_device
{
public:
	mir_sdr_device();
	~mir_sdr_device();

private:
	int getSamplingConfigurationTableIndex(int requestedSrHz);
	void writeWelcomeString() const;
	void cleanup();

	friend void* receive(void* p);
	friend void streamCallback(short *xi, short *xq, unsigned int firstSampleNum,
		int grChanged, int rfChanged, int fsChanged, unsigned int numSamples,
		unsigned int reset, unsigned int hwRemoved, void *cbContext);

public:
	void init(rsp_cmdLineArgs* pargs);
	void start(SOCKET client);
	void stop();

	pthread_mutex_t mutex_rxThreadStarted;
	pthread_cond_t started_cond = PTHREAD_COND_INITIALIZER;
	pthread_t* thrdRx;

	/// <summary>
	/// API: Device Enumeration Structure
	/// </summary>
	string serno;		// serial number
	string DevNm;		// device string (USB)
	BYTE hwVer;			// HW version
	bool devAvail;		// true if available
	bool isStreaming;
	bool flatGr;		// true: dont use the gain reduction tables
	int LNAstate;		// Calculated from the RequestedGain

	int RequestedGain; // the gain requested from the user, NOT the gain reduction used by the RSP
	bool started = false;
	unsigned int DeviceIndex;
	//The socket of the remote app
	SOCKET remoteClient;



private:

	const int c_welcomeMessageLength = 100;
	BYTE* mergeIQ(const short* idata, const short* qdata, int samplesPerPacket, int& buflen);
	mir_sdr_ErrT setFrequencyCorrection(int value);
	mir_sdr_ErrT setBiasT(int value);
	mir_sdr_ErrT setAntenna(int value);
	mir_sdr_ErrT setAGC(bool on);
	mir_sdr_ErrT setGain(int value);
	mir_sdr_ErrT setSamplingRate(int requestedSrHz);
	mir_sdr_ErrT setFrequency(int valueHz);
	mir_sdr_ErrT reinit_Frequency(int valueHz);
	mir_sdr_ErrT stream_InitForSamplingRate(int sampleConfigsTableIndex);
	mir_sdr_ErrT mir_sdr_device::stream_InitForSamplingRate(
		mir_sdr_Bw_MHzT bandwidth,
		int reqSamplingRateHz,
		int deviceSamplingRateHz,
		unsigned int decimationFactor,
		unsigned int doDecimation
	);
		mir_sdr_ErrT stream_Uninit();
	bool RSPGainValuesFromRequestedGain(int flatValue, int rxtype, int& LNAstate, int& gr);

	//Reference: rtl_tcp.c fct command_worker, line 277
	//Copied from QIRX
	enum eRTLCommands
	{
		CMD_SET_FREQUENCY = 1
		, CMD_SET_SAMPLINGRATE = 2
		, CMD_SET_GAIN_MODE = 3               //int manual
		, CMD_SET_GAIN = 4                    //int gain, tenth dB; fct tuner_r82xx.c fct r82xx_set_gain
		, CMD_SET_FREQUENCYCORRECTION = 5     //int ppm
		, CMD_SET_IF_GAIN = 6                 //int stage, int gain
		, CMD_SET_AGC_MODE = 8                //int on
		, CMD_SET_DIRECT_SAMPLING = 9         //int on
		, CMD_SET_OFFSET_TUNING = 10          //int on
		, CMD_SET_TUNER_GAIN_BY_INDEX = 13
		, CMD_SET_BIAS_T = 14				  //int on
		, CMD_SET_RSP2_ANTENNA_CONTROL = 33   //int Antenna Select
	};

	// This server is able to stream native 16-bit data (of "short" type)
	// or - for comaptibility with some apps, 8-bit data, 
	// where ( 8-bit Byte) =  ( 16-bit short /64) + 127
	eBitWidth bitWidth = BITS_16;

	// Reasonable number of possible bandwidth/sampling rate combinations
	const int c_numSamplingConfigs = 10;
	samplingConfiguration samplingConfigs[10] = {
		samplingConfiguration(512000, 2048000, mir_sdr_BW_0_300, 4, true),
		samplingConfiguration(1024000, 2048000, mir_sdr_BW_0_600, 2, true),
		samplingConfiguration(2048000, 2048000, mir_sdr_BW_1_536, 1, false),
		samplingConfiguration(4096000, 4096000, mir_sdr_BW_5_000, 1, false),
		samplingConfiguration(8192000, 8192000, mir_sdr_BW_8_000, 1, false),
		samplingConfiguration(3000000, 3000000, mir_sdr_BW_1_536, 1, false),
		samplingConfiguration(4000000, 4000000, mir_sdr_BW_1_536, 1, false),
		samplingConfiguration(2400000, 2400000, mir_sdr_BW_1_536, 1, false),
		samplingConfiguration(2500000, 2500000, mir_sdr_BW_1_536, 1, false),
		samplingConfiguration(2000000, 8000000, mir_sdr_BW_8_000, 4, true)
	};


	//QIRX internal type for the RSP2
	eRxType rxType;// 7 for RSP1, 8 for RSP2

	//Generic API error type
	mir_sdr_ErrT err;

	int sys = 40;
	int agcPoint_dBfs = -25;

	// currently commanded values
	int currentFrequencyHz;
	int gainReduction;				// Calculated from the RequestedGain
	double currentSamplingRateHz;
	int antenna = 5;
	
	//Callbacks per second, for a "timer" to discard samples 
	//to prevent overrun in the device in the error case
	int cbksPerSecond = 0;
	bool cbkTimerStarted = false;
};

