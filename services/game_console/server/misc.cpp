#include "misc.h"
#include <time.h>
#include <string.h>
#include <poll.h>
#include <stdio.h>
#include <errno.h>


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


int Recv(int sock, void* data, uint32_t size, uint32_t timeout)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

	pollfd pollFd;
	pollFd.fd = sock;
	pollFd.events = POLLIN;
	pollFd.revents = 0;

    while(remain)
    {
		int ret = poll(&pollFd, 1, timeout);
        if(ret < 0)
        {
            printf("poll failed %s\n", strerror(errno));
            return -1;
        }
        if((pollFd.revents & POLLHUP) == POLLHUP)
		{
			printf("connection closed by peer\n");
            return 0;
		}
		if((pollFd.revents & POLLIN) == 0)
		{
			printf("recv timeout\n");
			return -1;
		}
		
        ret = recv(sock, ptr, remain, 0);
        if(ret == 0)
        {
            printf("connection closed by peer\n");
            return 0;
        }
        if(ret < 0)
        {
            printf("recv failed %s\n", strerror(errno));
            return ret;
        }
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}


int Send(int sock, const void* data, uint32_t size, uint32_t timeout)
{
    const uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

	pollfd pollFd;
	pollFd.fd = sock;
	pollFd.events = POLLOUT;
	pollFd.revents = 0;

    while(remain)
    {
		int ret = poll(&pollFd, 1, timeout);
        if(ret < 0)
        {
            printf("poll failed %s\n", strerror(errno));
            return ret;
        }
		if((pollFd.revents & POLLOUT) == 0)
		{
			printf("send timeout\n");
			return -1;
		}

        ret = send(sock, ptr, remain, 0);
        if(ret < 0)
        {
            printf("send failed %s\n", strerror(errno));
            return ret;
        }
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}