#pragma once
#include <string>

class CMeasTimeDiff
{
public:
	static double calcTimeDiff(
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1, const double& factor);
	static double calcTimeDiff_in_ms( 
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1);
	static double calcTimeDiff_in_us( 
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1);
	static double calcTimeDiff_in_ns( 
							   const LARGE_INTEGER& count2,
							   const LARGE_INTEGER& count1);
	static void formattedTimeOutput(const std::string& s, const double& tim);
};
