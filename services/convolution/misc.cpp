#include "misc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


uint64_t GetTime()
{
	timespec tp;
	memset(&tp, 0, sizeof(tp));
	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec * 1000000000llu + tp.tv_nsec;
}


char* ReadFile(const char* fileName, uint32_t& size)
{
    size = 0;
    FILE* f = fopen(fileName, "r");
    if (!f)
        return nullptr;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* fileData = (char*)malloc(size);
    if(fread(fileData, 1, size, f) != size)
    {
        free(fileData);
        fclose(f);
        return nullptr;    
    }
    fclose(f);

    return fileData;
}


Timer::Timer()
{
	m_startTime = GetTime();
}


double Timer::GetDurationNanosec() const
{
	return GetTime() - m_startTime;
}


double Timer::GetDurationMillisec() const
{
	return (GetTime() - m_startTime) * 0.000001;
}
