#include "misc.h"
#include <time.h>
#include <string.h>


float GetTime()
{
    timespec tp;
    memset(&tp, 0, sizeof(tp));
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}


NetworkAddr SockaddrToNetworkAddr(in_addr a)
{
    return a.s_addr & kNetworkMask;
}

const char* inet_ntoa(IPAddr addr)
{
    return inet_ntoa(*(in_addr*)&addr);
}