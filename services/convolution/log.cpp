#include "log.h"
#include <mutex>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>

static std::mutex GMutex;


void Time()
{
    timeval tv;
	gettimeofday(&tv, nullptr);
	// Convert it to local.
	tm platformTime;
	localtime_r(&tv.tv_sec, &platformTime);

    printf("[%02u:%02u:%04u %02u:%02u:%02u:%03u] ", platformTime.tm_mday, platformTime.tm_mon + 1, platformTime.tm_year + 1900,
            platformTime.tm_hour, platformTime.tm_min, platformTime.tm_sec, tv.tv_usec / 1000);
}


void Log(const char* formatStr, ...)
{
    std::lock_guard<std::mutex> guard(GMutex);
    Time();
    va_list args;
    va_start(args, formatStr);
    vprintf(formatStr, args);
    va_end(args);
}
