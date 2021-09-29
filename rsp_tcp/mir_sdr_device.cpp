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
//#define TIME_MEAS
#include "mir_sdr_device.h"
#include "sdrGainTable.h"
//#include "controlThread.h"
#include "MeasTimeDiff.h"
#include <iostream>
using namespace std;

static LARGE_INTEGER Count1, Count2;

mir_sdr_device::~mir_sdr_device()
{
	pthread_mutex_destroy(&mutex_rxThreadStarted);
	pthread_cond_destroy(&started_cond);
}

mir_sdr_device::mir_sdr_device() 
{
	thrdRx = 0;
	pthread_mutex_init(&mutex_rxThreadStarted, NULL);
	pthread_cond_init(&started_cond, NULL);

#ifdef TIME_MEAS
	Count1.LowPart = Count2.LowPart = 0;
	Count1.HighPart = Count2.HighPart = 0;
#endif
}

void mir_sdr_device::createCtrlThread(const char* addr, int port)
{
	cout << endl << "Creating ctrl thread..." << endl;
	if (thrdCtrl != 0) // just in case..
	{
		pthread_cancel(*thrdCtrl);
		delete thrdCtrl;
		thrdCtrl = 0;
	}
	ctrlThreadExitFlag = false;
	ctrlThreadData.addr = addr;
	ctrlThreadData.port = port;
	ctrlThreadData.pDoExit = &ctrlThreadExitFlag;
	ctrlThreadData.wait = 500000; //0.5s
	ctrlThreadData.dev = this; 

	thrdCtrl = new pthread_t();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int res = pthread_create(thrdCtrl, &attr, &ctrl_thread_fn, &ctrlThreadData);
	pthread_attr_destroy(&attr);
}



void mir_sdr_device::init(rsp_cmdLineArgs* pargs)
{
	//From the command line
	currentFrequencyHz = pargs->Frequency;
	RequestedGain = pargs->Gain;
	initialSamplingRateHz = pargs->SamplingRate;
	bitWidth = (eBitWidth)pargs->BitWidth;
	antenna = pargs->Antenna;
	amPort = pargs->amPort;


	if (hwVer == 1)
		rxType = RSP1;
	else if (hwVer == 255)
		rxType = RSP1A;
	else if (hwVer == 2)
	{
		rxType = RSP2;
		err = mir_sdr_AmPortSelect(amPort);
		cout << "AM Port Select: " << amPort << endl;
	}
	else if (hwVer == 3)
	{
		rxType = RSPduo;
		err = mir_sdr_AmPortSelect(amPort);
		cout << "AM Port Select: " << amPort << endl;
	}
	else
		rxType = UNKNOWN;

	//if (rxType == RSP1 || rxType == RSP1A)
		flatGr = false;
	//else
	//	flatGr = true;
	//if (rxType == RSP1 || rxType == RSP1A)
		//flatGr = false;
	//else
	//	flatGr = true;
}

void mir_sdr_device::cleanup()
{
	if (thrdRx != 0) // just in case..
	{
		pthread_cancel(*thrdRx);
		delete thrdRx;
		thrdRx = 0;
	}
}


void mir_sdr_device::stop()
{
	mir_sdr_ErrT err;

	if (!started)
	{
		cout << "Already Stopped. Nothing to do here.";
		return;
	}
	ctrlThreadExitFlag = true;
	cleanup();

	//cout << "Stopping, calling mir_sdr_StreamUninit..." << endl;
	//err = mir_sdr_StreamUninit();
	//cout << "mir_sdr_StreamUninit returned with: " << err << endl;
	//if (err == mir_sdr_Success)
	//	cout << "StreamUnInit successful(0)" << endl;
	//else
	//	cout << "StreamUnInit failed (1) with " << err << endl;
	//isStreaming = false;

	//err = mir_sdr_ReleaseDeviceIdx();
	//cout << "\nmir_sdr_ReleaseDeviceIdx returned with: " << err << endl;
	//cout << "DeviceIndex released: " << DeviceIndex << endl;
	//started = false;

	//closesocket(remoteClient);
	//remoteClient = INVALID_SOCKET;
	//cout << "Socket closed\n\n";
}

void mir_sdr_device::writeWelcomeString() const
{
	BYTE buf0[] = "RTL0";
	BYTE* buf = new BYTE[c_welcomeMessageLength];
	memset(buf, 0, c_welcomeMessageLength);
	memcpy(buf, buf0, 4);
	buf[6] = bitWidth;
	buf[7] = rxType+6;	//6:RSP1, 7: RSP1A, 8: RSP2
	buf[11] = gainConfiguration::GAIN_STEPS;
	buf[15] = 0x52; buf[16] = 0x53; buf[17] = 0x50; buf[18] = (int)rxType + 0x30; //"RSP2", interpreted e.g. by qirx
	send(remoteClient, (const char*)buf, c_welcomeMessageLength, 0);
	delete[] buf;
}


void mir_sdr_device::start(SOCKET client)
{
	started = true;

	remoteClient = client;
	writeWelcomeString();

	cout << endl << "Starting..." << endl;


	if (thrdRx != 0) // just in case..
	{
		pthread_cancel(*thrdRx );
		delete thrdRx;
		thrdRx = 0;
	}
	thrdRx = new pthread_t();

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	int res = pthread_create(thrdRx, &attr, &receive, this);
	pthread_attr_destroy(&attr);
}

BYTE* mir_sdr_device::mergeIQ(const short* idata, const short* qdata, int samplesPerPacket, int& buflen)
{
	BYTE* buf = 0;
	buflen = 0;

	if (bitWidth == BITS_16)
	{
		buflen = samplesPerPacket *4;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			buf[j++] = (BYTE)(idata[i] & 0xff);			
			buf[j++] = (BYTE)((idata[i] & 0xff00) >> 8);  

			buf[j++] = (BYTE)(qdata[i] & 0xff);			
			buf[j++] = (BYTE)((qdata[i] & 0xff00) >> 8); 
		}
	}
	else if (bitWidth == BITS_8)
	{
		// assume the 14 Bit ADC values are mapped onto signed 16-Bit values covering the whole range
		buflen = samplesPerPacket * 2;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			//restore the unsigned 14-Bit signal
			int tmpi = (idata[i] >> 2) + 0x2000;
			int tmpq = (qdata[i] >> 2) + 0x2000;

			// cut the six low order bits
			tmpi >>= 6;
			tmpq >>= 6;

			buf[j++] = (BYTE)tmpi;
			buf[j++] = (BYTE)tmpq;
			//buf[j++] = (BYTE)(idata[i] / 64 + 127);
			//buf[j++] = (BYTE)(qdata[i] / 64 + 127);
		}
	}
	//else if (bitWidth == BITS_8)
	//{
	//	// assume the 12 Bit ADC values are mapped onto signed 16-Bit values covering the whole range
	//	buflen = samplesPerPacket * 2;
	//	buf = new BYTE[buflen];
	//	for (int i = 0, j = 0; i < samplesPerPacket; i++)
	//	{
	//		//restore the unsigned 12-Bit signal
	//		int tmpi = (idata[i] >> 4) + 2048;
	//		int tmpq = (qdata[i] >> 4) + 2048;

	//		// cut the four low order bits
	//		tmpi >>= 4;
	//		tmpq >>= 4;

	//		buf[j++] = (BYTE)tmpi;
	//		buf[j++] = (BYTE)tmpq;
	//		//buf[j++] = (BYTE)(idata[i] / 64 + 127);
	//		//buf[j++] = (BYTE)(qdata[i] / 64 + 127);
	//	}
	//}
	else if (bitWidth == BITS_4)
	{
		buflen = samplesPerPacket * 1;
		buf = new BYTE[buflen];
		for (int i = 0, j = 0; i < samplesPerPacket; i++)
		{
			//I-Byte
			byte b = (BYTE)(idata[i] / 64 + 127);
			// Low order nibble
			byte b2 = b >> 4;
			b2 &= 0x0f;
				
			//Q-Byte
			b = (BYTE)(qdata[i] / 64 + 127);
			// High order nibble
			byte b3 = b & 0xf0;

			byte b4 = b2 | b3;
			buf[j++] = b4;
		}
	}
	return buf;
}

// 2048000 numSamples == 1344
// 1000* 1344 * 1/2048000 = 656.25ms
void streamCallback(short *xi, short *xq, unsigned int firstSampleNum,
	int grChanged, int rfChanged, int fsChanged, unsigned int numSamples,
	unsigned int reset, unsigned int hwRemoved, void *cbContext)
{

	if (hwRemoved)
	{
		cout << " !!! HW removed !!!" << endl;
		return;
	}
	if (reset)
	{
		cout << " !!! reset !!!" << endl;
		return;
	}

	static int count = 0;

	mir_sdr_device* md = (mir_sdr_device*)cbContext;
	try
	{
		if (!md->isStreaming)
		{
			return;
		}
		//In case of a socket error, dont process the data for one second
		if (md->cbksPerSecond-- > 0)
		{
			if (!md->cbkTimerStarted)
			{
				md->cbkTimerStarted = true;
				cout << "Discarding samples for one second\n";
			}
			return;
		}
		else if (md->cbkTimerStarted)
		{
			md->cbkTimerStarted = false;
		} 
		if (md->remoteClient == INVALID_SOCKET)
			return;

#ifdef TIME_MEAS
		count++;
		if (count % 1000 == 0)
		{
			QueryPerformanceCounter(&Count2);
			CMeasTimeDiff::formattedTimeOutput ("1000 sdrplay buffers (ms)", CMeasTimeDiff::calcTimeDiff_in_ms (Count2, Count1));
			QueryPerformanceCounter(&Count1);
		}
#endif
		int buflen = 0;
		BYTE* buf = md->mergeIQ(xi, xq, numSamples, buflen);
		int remaining = buflen;
		int sent = 0;
		while (remaining > 0)
		{
			fd_set writefds;
			struct timeval tv= {1,0};
			FD_ZERO(&writefds);
			FD_SET(md->remoteClient, &writefds);
			int res  = select(md->remoteClient+1, NULL, &writefds, NULL, &tv);
			if (res > 0)
			{
				sent = send(md->remoteClient, (const char*)buf + (buflen - remaining), remaining, 0);
				remaining -= sent;
			}
			else
			{
				md->cbksPerSecond = md->initialSamplingRateHz / numSamples; //1 sec "timer" in the error case. assumed this does not change frequently
				delete[] buf;
				throw msg_exception("socket error " + to_string(errno));
			}
		}
		delete[] buf;
	}
	catch (exception& e)
	{
		cout << "Error in streaming callback :" << e.what() <<  endl;
	}
}

void gainChangeCallback(unsigned int gRdB, unsigned int lnaGRdB, void* cbContext)
{
	// do nothing...
}


int getCommandAndValue(char* rxBuf, int& value)
{
	BYTE valbuf[4];
	int cmd = rxBuf[0];
	memcpy( valbuf, rxBuf + 1,4);

	if (common::isLittleEndian())
		value = valbuf[3] + valbuf[2] * 0x100 + valbuf[1] * 0x10000 + valbuf[0] * 0x1000000;
	else
		value = valbuf[0] + valbuf[1] * 0x100 + valbuf[2] * 0x10000 + valbuf[3] * 0x1000000;
	return cmd;
}


/// <summary>
/// Receive thread, to process commands
/// </summary>
void* receive(void* p)
{ 
	mir_sdr_ErrT err;
	mir_sdr_device* md = (mir_sdr_device*)p;
	cout << "**** receive thread entered.   *****" << endl;
	/*
	pthread_mutex_lock(&md->mutex_rxThreadStarted);
	pthread_cond_signal(&md->started_cond);
	pthread_mutex_unlock(&md->mutex_rxThreadStarted);
	cout << "**** receive thread signalled. *****" << endl;
	*/
	try
	{
		float apiVersion = 0.0f;

		mir_sdr_ErrT errInit;// = md->stream_Uninit();
		err = mir_sdr_ApiVersion(&apiVersion);

		cout << "\nmir_sdr_ApiVersion returned with: " << err << endl;
		cout << "API Version " << apiVersion << endl;

		md->initialSamplingRateHz =  md->samplingConfigs[2].samplingRateHz;
		md->oldDeltaSrHz = 0;
		int smplsPerPacket;

		mir_sdr_SetGrModeT grMode = mir_sdr_USE_RSP_SET_GR;
		if (md->flatGr)
		{
			grMode = mir_sdr_USE_SET_GR;
			md->LNAstate = 0;
		}
		else
		{
			int gain = md->RequestedGain;
			if (!md->RSPGainValuesFromRequestedGain(gain, md->rxType, md->LNAstate, md->gainReduction))
			{
				cout << "\nCannot retrieve LNA state and Gain Reduction from requested gain value " << gain << endl;
				cout << "Program cannot continue" << endl;
				return 0;
			}
			cout << "\n Using LNA State: " << md->LNAstate << endl;
			cout << " Using Gain Reduction: " << md->gainReduction << endl;
		}
		//// for tests
		//errInit = mir_sdr_SetTransferMode(mir_sdr_BULK);
		//cout << "\mir_sdr_SetTransferMode " << mir_sdr_BULK << "  returned with: " << errInit << endl;


	secondChance:
		md->oldDeltaSrHz = 0;
		errInit = mir_sdr_StreamInit(&md->gainReduction,
			md->initialSamplingRateHz / 1e6,
			md->currentFrequencyHz / 1e6,
			md->samplingConfigs[2].bandwidth,
			mir_sdr_IF_Zero,
			md->LNAstate,
			&md->sys,
			grMode,
			&smplsPerPacket,
			streamCallback,
			gainChangeCallback,
			md);

		cout << "\nmir_sdr_StreamInit returned with: " << errInit << endl;
		// disable DC offset and IQ imbalance correction (default is for these to be enabled  this
		// just show how to disable if required)
		//err = mir_sdr_DCoffsetIQimbalanceControl(1, 0);
		//cout << "\nmir_sdr_DCoffsetIQimbalanceControl returned with: " << err << endl;

		if (errInit == mir_sdr_AlreadyInitialised)
		{
			cout << "Already initialized, uninit first" << endl;
			mir_sdr_ErrT errInit = md->stream_Uninit();
			cout << "\stream_Uninit returned with: " << err << endl;
			usleep(1000000);
			goto secondChance;
		}
		if (errInit == mir_sdr_Success || errInit == mir_sdr_AlreadyInitialised)
		{
			cout << "Starting SDR streaming" << endl;
			md->isStreaming = true;
		}
		else
		{
			cout << "API Init failed with error: " << errInit << endl;
			md->isStreaming = false;
		}

		md->setAntenna(md->antenna);

		// configure DC tracking in tuner 
		err = mir_sdr_SetDcMode(4, 1); // select one-shot tuner DC offset correction with speedup 
		cout << "\nmir_sdr_SetDcMode returned with: " << err << endl;

		err = mir_sdr_SetDcTrackTime(63); // with maximum tracking time 
		cout << "\nmir_sdr_SetDcTrackTime returned with: " << err << endl;
		md->setAGC(true);
	}
	catch (const std::exception& )
	{
		cout << " !!!!!!!!! Exception in the initialization !!!!!!!!!!\n";
		return 0;
	}

	try
	{
		char rxBuf[16];
		for (;;)
		{
			const int cmd_length = 5;
			memset(rxBuf, 0, 16);
			int remaining = cmd_length;

			while (remaining > 0)
			{
				int rcvd = recv(md->remoteClient, rxBuf + (cmd_length - remaining), remaining, 0); //read 5 bytes (cmd + value)
				remaining -= rcvd;

				if (rcvd  == 0)
				{
					err = mir_sdr_StreamUninit();
					cout << "mir_sdr_StreamUninit returned with: " << err << endl;
					err = mir_sdr_ReleaseDeviceIdx();
					cout << "mir_sdr_ReleaseDeviceIdx returned with: " << err << endl;
					throw msg_exception("Socket closed");
					//err = mir_sdr_ReleaseDeviceIdx();
				}
				if (rcvd == SOCKET_ERROR)
				{
					err = mir_sdr_StreamUninit();
					cout << "mir_sdr_ReleaseDeviceIdx returned with: " << err << endl;
					err = mir_sdr_ReleaseDeviceIdx();
					cout << "mir_sdr_StreamUninit returned with: " << err << endl;
					throw msg_exception("Socket error");
				}
			}

			int value = 0; // out parameter
			int cmd = getCommandAndValue(rxBuf,  value);

			// The ids of the commands are defined in rtl_tcp, the names had been inserted here
			// for better readability
			switch (cmd)
			{
			case mir_sdr_device::CMD_SET_FREQUENCY: //set frequency
													  //value is freq in Hz
				err = md->setFrequency(value);
				break;

			case (int)mir_sdr_device::CMD_SET_SAMPLINGRATE:
				md->oldDeltaSrHz = 0;
				err = md->setSamplingRate(value);//value is sr in Hz
				break;

			case (int)mir_sdr_device::CMD_SET_FREQUENCYCORRECTION: //value is ppm correction
				md->setFrequencyCorrection(value);
				break;

			case (int)mir_sdr_device::CMD_SET_FREQUENCYCORRECTION_PPM10: //value is ppm*10 correction
				md->setFrequencyCorrection100(value);
				break;

			case (int)mir_sdr_device::CMD_SET_TUNER_GAIN_BY_INDEX:
				//value is gain value between 0 and 100
				err = md->setGain(value);
				break;

			case (int)mir_sdr_device::CMD_SET_AGC_MODE:
				err = md->setAGC(value != 0);
				break;

			case (int)mir_sdr_device::CMD_SET_BIAS_T:
				err = md->setBiasT(value != 0);
				break;

			case (int)mir_sdr_device::CMD_SET_RSP2_ANTENNA_CONTROL:
				md->setAntenna(value);
				break;
			default:
				printf("Unknown Command; 0x%x 0x%x 0x%x 0x%x 0x%x\n",
					rxBuf[0], rxBuf[1], rxBuf[2], rxBuf[3], rxBuf[4]);
				break;
			}
		}
	}
	catch (exception& e)
	{
		cout << "Error in receive :" << e.what() <<  endl;
		//err = mir_sdr_StreamUninit();
		//cout << "mir_sdr_StreamUninit returned with: " << err << endl;
		//if (err == mir_sdr_Success)
		//	cout << "StreamUnInit successful(0)" << endl;
		//else
		//	cout << "StreamUnInit failed (1) with " << err << endl;
	}
	cout << "**** Rx thread terminating. ****" << endl;
	return 0;
}

//value is correction in ppm
mir_sdr_ErrT mir_sdr_device::setFrequencyCorrection(int value)
{
	mir_sdr_ErrT err = mir_sdr_SetPpm((double)value);
	
	cout << "\nmir_sdr_SetPpm returned with: " << err << endl;
	if (err != mir_sdr_Success)
		cout << "PPM setting error: " << err << endl;
	else
		cout << "PPM correction: " << value << endl;
	return err;
}

//value is correction in ppm*100
mir_sdr_ErrT mir_sdr_device::setFrequencyCorrection100(int value)
{
	double valPpm = (double)(value / 100.0);
	mir_sdr_ErrT err = mir_sdr_SetPpm(valPpm);
	
	cout << "\nmir_sdr_SetPpm returned with: " << err << endl;
	if (err != mir_sdr_Success)
	{
		cout << "PPM setting error: " << err << endl;
		return err;
	}
	else
		cout << "PPM correction: " << valPpm << endl;


	// first undo the old samplingrate delta
	if (oldDeltaSrHz != 0)
	{
		err = mir_sdr_SetFs(-oldDeltaSrHz, 0, 0, 0);
		cout << "\nmir_sdr_SetFs returned with: " << err << endl;
		if (err != mir_sdr_Success)
		{
			cout << "Undo old Sampling rate delta setting error: " << err << endl;
		}
		else
			cout << "old Sampling rate correction undone: " << -oldDeltaSrHz << "Hz" << endl << endl;
	}

	// necessary to let the sr settling
	usleep(100000.0f);

	double tmp = initialSamplingRateHz;
	double deltaSrHz =(-1)* initialSamplingRateHz * valPpm / 1e6;
	if (abs(deltaSrHz) <= DBL_EPSILON)
		return err;
	err = mir_sdr_SetFs(deltaSrHz, 0, 0, 0);
	cout << "\nmir_sdr_SetFs returned with: " << err << endl;
	if (err != mir_sdr_Success)
	{
		cout << "Sampling rate setting error: " << err << endl;
	}
	else
		cout << "Sampling rate correction: " << deltaSrHz << "Hz" << endl << endl;
	oldDeltaSrHz = deltaSrHz;

	return err;
}

mir_sdr_ErrT mir_sdr_device::setBiasT(int value)
{
	mir_sdr_ErrT err = mir_sdr_Success;
	switch (rxType)
	{
		case RSP1A:
			err = mir_sdr_rsp1a_BiasT(value);
			break;
		case RSP2:
			err = mir_sdr_RSPII_BiasTControl(value);
			break;
		case RSPduo:
			err = mir_sdr_rspDuo_BiasT(value);
			break;
		default:
			break;
	}
	cout << "\nmir_sdr_xxx_BiasT returned with: " << err << endl;
	if (err != mir_sdr_Success)
		cout << "BiasT setting error: " << err << endl;
	else
		cout << "BiasT setting: " << value << endl;
	return err;
}

mir_sdr_ErrT mir_sdr_device::setAntenna(int value)
{
	mir_sdr_ErrT err = mir_sdr_Success;
	switch (rxType)
	{
	case RSP1A:
		err = mir_sdr_HwVerError;
		break;
	case RSP2:
		err = mir_sdr_RSPII_AntennaControl((mir_sdr_RSPII_AntennaSelectT)value);
		break;
	case RSPduo:
		err = mir_sdr_rspDuo_TunerSel((mir_sdr_rspDuo_TunerSelT)(value-4));
		break;
	default:
		break;
	}

	cout << "\nmir_sdr_RSPII_AntennaControl returned with: " << err << endl;
	if (err != mir_sdr_Success)
		cout << "Antenna Control Setting error: " << err << endl;
	else
		cout << "Antenna Control Setting: " << value << endl;
	return err;
}

mir_sdr_ErrT mir_sdr_device::setAGC(bool on)
{
	mir_sdr_ErrT err = mir_sdr_Fail;
	if (on == false)
	{
		err = mir_sdr_AgcControl(mir_sdr_AGC_DISABLE, agcPoint_dBfs, 0, 0, 0, 0, LNAstate);
		if(VERBOSE)
			cout << "\nmir_sdr_AgcControl OFF returned with: " << err << endl;
	}
	else
	{
		// enable AGC with a setPoint of -15dBfs //optimum for DAB
		err = mir_sdr_AgcControl(mir_sdr_AGC_50HZ, agcPoint_dBfs, 0, 0, 0, 0, LNAstate);
		if(VERBOSE)
			cout << "\nmir_sdr_AgcControl 50Hz, " << agcPoint_dBfs << " dBfs returned with: " << err << endl;
	}
	if (err != mir_sdr_Success)
	{
		cout << "SetAGC failed." << endl;
	}

	return err;
}

bool mir_sdr_device::RSPGainValuesFromRequestedGain(int flatValue, int rxtype, int& LNAstate, int& gr)
{
	if (flatGr)
		return false;

	mir_sdr_SetGrModeT grMode = mir_sdr_USE_RSP_SET_GR;

	mir_sdr_BandT band = (mir_sdr_BandT)0;
	int gRdB = 0;
	int gRdBsystem = 0;

	mir_sdr_ErrT err = mir_sdr_GetGrByFreq(currentFrequencyHz / 1e6, &band, &gRdB, LNAstate,
		&gRdBsystem, grMode);

	gainConfiguration gcfg(band);

	if (gcfg.calculateGrValues(flatValue, rxType, LNAstate, gr))
	{
		return true;
	}
	return false;
}

mir_sdr_ErrT mir_sdr_device::setGain(int value)
{
	mir_sdr_ErrT err;
	mir_sdr_SetGrModeT grMode;

	if (flatGr)
	{
		grMode = mir_sdr_USE_SET_GR;
		gainReduction = gainConfiguration::GAIN_STEPS - value;
		err = mir_sdr_SetGr(gainReduction, 1, 0);
	}
	else
	{

		int lnastate = 0;
		int gr = 0;

		if (RSPGainValuesFromRequestedGain(value, rxType, lnastate, gr))
		{
			err = mir_sdr_RSP_SetGr(gr, lnastate, 1, 0);
			gainReduction = gr;
			LNAstate = lnastate;

			if (VERBOSE)
			{
				cout << "RSP_SetGr succeeded with requested value: " << gainConfiguration::GAIN_STEPS - value << endl;
				cout << "LNA State: " << lnastate << endl;
				cout << "Gr value: " << gr << endl;
			}
			return err;
		}
		else
		{
			cout << "RSP_SetGr failed with requested value: " << gainConfiguration::GAIN_STEPS -value << endl;
			return (mir_sdr_ErrT) -1;
		}
	}
	
	if (VERBOSE)
		cout << "\nmir_sdr_SetGr returned with: " << err << endl;

	if (err != mir_sdr_Success)
	{
		cout << "SetGr failed with requested value: " << gainConfiguration::GAIN_STEPS -value << endl;
	}
	else
		cout << "SetGr succeeded with requested value: " << gainConfiguration::GAIN_STEPS -value << endl;

	return err;
}

mir_sdr_ErrT mir_sdr_device::setSamplingRate(int requestedSrHz)
{
	mir_sdr_ErrT err = stream_Uninit();
	if (err != mir_sdr_Success)
		return err;

	int ix = getSamplingConfigurationTableIndex(requestedSrHz);
	if (ix >= 0)
	{
		err = stream_InitForSamplingRate(ix);
		return err;
	}
	else
	{

	}
		return mir_sdr_Fail;
}

mir_sdr_ErrT mir_sdr_device::setFrequency(int valueHz)
{
	mir_sdr_ErrT err = mir_sdr_SetRf((double)valueHz, 1, 0);
	if (VERBOSE)
		cout << "\nmir_sdr_SetRf returned with: " << err << endl;

	switch (err)
	{
	case mir_sdr_Success:
		break;
	case mir_sdr_RfUpdateError:
		//sleep(0.5f);//wait for the old command to settle
		err = mir_sdr_SetRf((double)valueHz, 1, 0);
		if (VERBOSE)
		{
			cout << "\nmir_sdr_SetRf returned with: " << err << endl;
			cout << "Frequency setting result: " << err << endl;
		}

		if (err == mir_sdr_OutOfRange || mir_sdr_RfUpdateError)
			err = reinit_Frequency(valueHz);
		break;

	case mir_sdr_OutOfRange:
		err = reinit_Frequency(valueHz);
		break;
	default:
		break;
	}
	if (err != mir_sdr_Success)
	{
		cout << "Frequency setting error: " << err << endl;
		cout << "Requested Frequency was: " << +valueHz << endl;
	}
	else
	{
		currentFrequencyHz = valueHz;
		if (VERBOSE)
			cout << "Frequency set to (Hz): " << valueHz << endl;
	}
	return err;
}

mir_sdr_ErrT mir_sdr_device::reinit_Frequency(int valueHz)
{
	mir_sdr_ErrT err = mir_sdr_Fail;

	int samplesPerPacket;

	mir_sdr_SetGrModeT grMode;
	if (flatGr)
		grMode = mir_sdr_USE_SET_GR;
	else
		grMode = mir_sdr_USE_RSP_SET_GR;


	oldDeltaSrHz = 0;

	err = mir_sdr_Reinit(
		&gainReduction,
		initialSamplingRateHz / 1e6,
		(double)valueHz / 1e6,
		(mir_sdr_Bw_MHzT)0,//mir_sdr_Bw_MHzT.mir_sdr_BW_1_536,
		(mir_sdr_If_kHzT)0,//mir_sdr_If_kHzT.mir_sdr_IF_Zero,
		(mir_sdr_LoModeT)0,//mir_sdr_LoModeT.mir_sdr_LO_Undefined,
		LNAstate,
		&sys,
		grMode,
		&samplesPerPacket,
		mir_sdr_CHANGE_RF_FREQ);
	if (VERBOSE)
		cout << "\nmir_sdr_Reinit returned with: " << err << endl;
	if (err != mir_sdr_Success)
	{
		cout << "Requested Frequency (Hz) was: " << valueHz << endl;
	}
	else
	{
		if (VERBOSE)
			cout << "Frequency set to (Hz): " << valueHz << endl;
		currentFrequencyHz = valueHz;
	}
	return err;
}

//mir_sdr_ErrT mir_sdr_device::stream_InitForSamplingRate(int srHz)
//{
//	mir_sdr_Bw_MHzT bandwidth = srHz;
//	int reqSamplingRateHz = samplingConfigs[ix].samplingRateHz;
//	int deviceSamplingRateHz = samplingConfigs[ix].deviceSamplingRateHz;
//	unsigned int decimationFactor = samplingConfigs[ix].decimationFactor;
//	unsigned int  doDecimation = samplingConfigs[ix].doDecimation ? 1 : 0;
//	return stream_InitForSamplingRate(bandwidth, reqSamplingRateHz, deviceSamplingRateHz, decimationFactor, doDecimation);
//}
mir_sdr_ErrT mir_sdr_device::stream_InitForSamplingRate(int sampleConfigsTableIndex)
{
	oldDeltaSrHz = 0;
	int ix = sampleConfigsTableIndex;

	mir_sdr_Bw_MHzT bandwidth = samplingConfigs[ix].bandwidth;
	int reqSamplingRateHz = samplingConfigs[ix].samplingRateHz;
	int deviceSamplingRateHz = samplingConfigs[ix].deviceSamplingRateHz;
	unsigned int decimationFactor = samplingConfigs[ix].decimationFactor;
	unsigned int  doDecimation = samplingConfigs[ix].doDecimation ? 1 : 0;
	return stream_InitForSamplingRate(bandwidth, reqSamplingRateHz, deviceSamplingRateHz, decimationFactor, doDecimation);
}

mir_sdr_ErrT mir_sdr_device::stream_InitForSamplingRate(
	mir_sdr_Bw_MHzT bandwidth,
	int reqSamplingRateHz,
	int deviceSamplingRateHz,
	unsigned int decimationFactor,
	unsigned int doDecimation
	)
{
	//int ix = sampleConfigsTableIndex;

	//mir_sdr_Bw_MHzT bandwidth = samplingConfigs[ix].bandwidth;
	//int reqSamplingRateHz = samplingConfigs[ix].samplingRateHz;
	//int deviceSamplingRateHz = samplingConfigs[ix].deviceSamplingRateHz;
	//unsigned int decimationFactor = samplingConfigs[ix].decimationFactor;
	//unsigned int  doDecimation = samplingConfigs[ix].doDecimation ? 1 : 0;

	oldDeltaSrHz = 0;
	mir_sdr_ErrT err = mir_sdr_Fail;

	int samplesPerPacket;

	mir_sdr_SetGrModeT grMode;
	if (flatGr)
		grMode = mir_sdr_USE_SET_GR;
	else
		grMode = mir_sdr_USE_RSP_SET_GR;

	err = mir_sdr_StreamInit(
		&gainReduction,
		(double)deviceSamplingRateHz / 1e6,
		currentFrequencyHz / 1e6,
		bandwidth,
		mir_sdr_IF_Zero,
		LNAstate,
		&sys,
		grMode,
		&samplesPerPacket,
		streamCallback,
		gainChangeCallback,
		this);
	if (VERBOSE)
		cout << "\nmir_sdr_StreamInit returned with: " << err << endl;
	if (err != mir_sdr_Success)
	{
		cout << "Sampling Rate setting error: " << err << endl;
		cout << "Requested Sampling Rate was: " << reqSamplingRateHz << endl;
	}
	else
	{
		cout << "Sampling Rate set to (Hz): " << deviceSamplingRateHz << endl;
		initialSamplingRateHz =  deviceSamplingRateHz;

		if (doDecimation == 1)
		{
			err = mir_sdr_DecimateControl(doDecimation, decimationFactor, 0);
			cout << "mir_sdr_DecimateControl returned with: " << err << endl;
			if (err != mir_sdr_Success)
			{
				cout << "Requested Decimation Factor  was: " << decimationFactor << endl;
			}
			else
			{
				cout << "Decimation Factor set to  " << decimationFactor << endl;
			}

		}
		else
			cout << "No Decimation applied\n";
	}
	return err;
}

mir_sdr_ErrT mir_sdr_device::stream_Uninit()
{
	mir_sdr_ErrT err = mir_sdr_Fail;
	int cnt = 0;
	while (err != mir_sdr_Success)
	{
		cnt++;
		try {
			err = mir_sdr_StreamUninit();
		}
		catch (...)
		{
			cout << "\nmir_sdr_StreamUninit returned with: " << err << endl;
			err = mir_sdr_Fail;
		}
		if (err == mir_sdr_Success)
			break;
		usleep(100000.0f);
		if (cnt == 5)
			break;
	}
	if (err != mir_sdr_Success)
	{
		cout << "StreamUninit failed with: " << err << endl;
	}
	return err;
}

/// <summary>
/// Gets the config table index for a requested sampling rate
/// </summary>
/// <param name="requestedSrHz">Requested sampling rate in Hz</param>
/// <returns>Index into the samplingConfigs table, -1 if not found</returns>
int mir_sdr_device::getSamplingConfigurationTableIndex(int requestedSrHz)
{
	for (int i = 0; i < c_numSamplingConfigs; i++)
	{
		samplingConfiguration sc = samplingConfigs[i];
		if (requestedSrHz == sc.samplingRateHz)
		{
			return i;
		}
	}
	printf("Sampling Rate: %d; Should be %d or %d or %d or %d or %d\n", requestedSrHz, samplingConfigs[0].samplingRateHz,
		samplingConfigs[1].samplingRateHz, samplingConfigs[2].samplingRateHz, samplingConfigs[3].samplingRateHz, samplingConfigs[4].samplingRateHz);
	printf("Sampling Rate: %d Hz will be tried\n", requestedSrHz);

	return -1;
}



