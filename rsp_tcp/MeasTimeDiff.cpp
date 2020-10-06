#include <windows.h>
#include <iostream>
#include <iomanip>
#include ".\meastimediff.h"

///////////////////////////////////////////////////////////////////////////////
void CMeasTimeDiff::formattedTimeOutput(const std::string& s, const double& tim)
{
	std::cout << s << std::setprecision(5) << tim << std::endl;
}
///////////////////////////////////////////////////////////////////////////////
//factor == 1:		Time in seconds
//factor == 1e9		Time in nanoseconds
//factor == 1e3		Time in milliseconds
// etc.
double CMeasTimeDiff::calcTimeDiff(
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1, const double& factor)
{
	LARGE_INTEGER frequency;
	BOOL b = QueryPerformanceFrequency(&frequency);
	if (b ==  FALSE)	//then perf. counting not supported
		return 0.0;

	double time_in_unit = 0.0;
	double oneCount_perTime = 1/(double)frequency.QuadPart * factor;

	LONGLONG delta = count2.QuadPart - count1.QuadPart;
	time_in_unit = (double)delta * oneCount_perTime;

	return time_in_unit;
}
///////////////////////////////////////////////////////////////////////////////
double CMeasTimeDiff::calcTimeDiff_in_ms(
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1)
{
	return calcTimeDiff (count2, count1, 1e3);
}
///////////////////////////////////////////////////////////////////////////////
double CMeasTimeDiff::calcTimeDiff_in_us(
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1)
{
	return calcTimeDiff (count2, count1, 1e6);
}
///////////////////////////////////////////////////////////////////////////////
double CMeasTimeDiff::calcTimeDiff_in_ns(
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1)
{
	return calcTimeDiff (count2, count1, 1e9);
}

