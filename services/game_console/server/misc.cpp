#include "misc.h"
#include <time.h>
#include <string.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>


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


IPAddr inet_aton(const char* addrStr)
{
    in_addr addr;
    inet_aton(addrStr, &addr);
    return addr.s_addr;
}


char* ReadFile(const char* fileName, uint32_t& size)
{
    FILE* f = fopen(fileName, "r");
    if (!f)
    {
        size = 0;
        return nullptr;
    }

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* fileData = (char*)malloc(size);
    fread(fileData, 1, size, f);
    fclose(f);

    return fileData;
}