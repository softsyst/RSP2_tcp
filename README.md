# RSP2_tcp
TCP/IP Server for I/Q data delivered by sdrplay RSP devices : RSP1 (untested), RSP1A, RSP2, RSPduo (single tuner)
- Compatible to rtl_tcp,
- Runs on Windows and Linux,
- Delivers 8- and 16- Bit I/Q Data,
- Compatible with SDR# (8-Bit Mode, Source RTL-SDR (TCP))
- Working with sdrplay driver 2.13.
## History
### V0.9.9, October 2020
- Gain steps changed from 99 to 80
- AGC Algorithm revised
- Time Measurements possible with conditional compilation
### V0.9.7, November 2019
- Version necessary for QIRX V2.1.1.6 running RSPs
- BiasT,
- Antenna selection mapped to tuner selection in the RSPduo
- Minor errors in gain table fixed### V0.9.7, November 2019
- Gain settings for the commandline argument cleaned,  
### V0.9.6, June 2019
- Gain settings revised, RSP1(A) should now work much better,  
- RSP2duo newly supported.  

**Credits:**  
- Thanks to Herman Wijnants (http://www.wijnants.info/fmdx) for his extensive tests of the revised gain settings,  
- Thanks to Martien, PD1G for his testing of the program with the RSPduo.
