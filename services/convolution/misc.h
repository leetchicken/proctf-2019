#pragma once
#include <stdint.h>

uint64_t GetTime();
char* ReadFile(const char* fileName, uint32_t& size);


struct Timer
{
	Timer();
	double GetDurationNanosec() const;
	double GetDurationMillisec() const;

	double m_startTime = 0.0;
};
