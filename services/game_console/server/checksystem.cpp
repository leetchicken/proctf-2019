#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <unistd.h>
#include <utility>
#include <string.h>
#include <thread>
#include <mutex>
#include <map>
#include "checksystem.h"


static const uint32_t kScreenSize = 24;
static std::mutex GMutex;
static std::map<IPAddr, int> GIpToSocket;
static std::thread GNetworkThread;
static std::vector<IPAddr> GConsolesIp;


struct Point2D
{
    int32_t x;
    int32_t y;
};


static Point2D GeneratePoint()
{
    Point2D ret;
    ret.x = rand() % kScreenSize;
    ret.y = rand() % kScreenSize;
    return ret;
}


static int32_t EdgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) 
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}


static bool RasterizeAndCompare(Point2D* v, uint16_t* screenToCompare)
{
    int32_t minX = kScreenSize - 1;
    int32_t minY = kScreenSize - 1;
    int32_t maxX = 0;
    int32_t maxY = 0;

    for(uint32_t vi = 0; vi < 3; vi++)
    {
        if(v[vi].x > maxX) maxX = v[vi].x;
        if(v[vi].y > maxY) maxY = v[vi].y;
        if(v[vi].x < minX) minX = v[vi].x;
        if(v[vi].y < minY) minY = v[vi].y;
    }

    int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
    if(doubleTriArea <= 0)
        return true;

    Point2D p;
    for(p.y = minY; p.y <= maxY; p.y++)
    {
        for(p.x = minX; p.x <= maxX; p.x++)
        {
            int32_t w0 = EdgeFunction(v[1], v[2], p);
            int32_t w1 = EdgeFunction(v[2], v[0], p);
            int32_t w2 = EdgeFunction(v[0], v[1], p);

            if((w0 | w1 | w2) >= 0) 
            {
                uint32_t pixel = w0 + w1 + w2;
                uint16_t pixel1 = screenToCompare[p.y * kScreenSize + p.y];
                if((uint16_t)pixel != pixel1)
                    return false;
                printf("x");
            }
            else
                printf("0");
        }
        printf("\n");
    }

    return true;
}


static int Recv(int sock, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

	pollfd pollFd;
	pollFd.fd = sock;
	pollFd.events = POLLIN;
	pollFd.revents = 0;

    while(remain)
    {
		int ret = poll(&pollFd, 1, 5000);
        if(ret < 0)
        {
            printf("  ERROR: poll failed %s\n", strerror(errno));
            return ret;
        }
		if((pollFd.revents & POLLIN) == 0)
		{
			printf("  ERROR: recv timeout\n");
			return ret;
		}
		
        ret = recv(sock, ptr, remain, 0);
        if(ret == 0)
        {
            printf("  ERROR: connection closed by peer\n");
            return 0;
        }
        if(ret < 0)
        {
            printf("  ERROR: recv failed %s\n", strerror(errno));
            return ret;
        }
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}


static int Send(int sock, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

	pollfd pollFd;
	pollFd.fd = sock;
	pollFd.events = POLLOUT;
	pollFd.revents = 0;

    while(remain)
    {
		int ret = poll(&pollFd, 1, 5000);
        if(ret < 0)
        {
            printf("  ERROR: poll failed %s\n", strerror(errno));
            return ret;
        }
		if((pollFd.revents & POLLOUT) == 0)
		{
			printf("  ERROR: send timeout\n");
			return ret;
		}

        ret = send(sock, ptr, remain, 0);
        if(ret < 0)
        {
            printf("  ERROR: send failed %s\n", strerror(errno));
            return ret;
        }
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}


static void NetworkThread()
{
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0)
    {
        printf("CHECKSYSTEM: socket failed\n");
        return;
    }

    int opt_val = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kChecksystemPort);
	inet_aton("0.0.0.0", &addr.sin_addr);
    int ret = bind(listenSock, (sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		printf("CHECKSYSTEM: bind failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

	ret = listen(listenSock, 128);
    if(ret < 0)
	{
		printf("CHECKSYSTEM: listen failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

    while(1)
    {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int newSocket = accept4(listenSock, (sockaddr*)&clientAddr, &addrLen, SOCK_NONBLOCK);
        if(newSocket < 0)
        {
            printf("CHECKSYSTEM: accept failed: %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        IPAddr ip = clientAddr.sin_addr.s_addr;
        bool isIpValid = false;
        for(auto validIp : GConsolesIp)
        {
            if(validIp != ip)
                continue;
            
            isIpValid = true;
            break;
        }

        if(!isIpValid)
        {
            printf("CHECKSYSTEM: unknown ip: %s\n", inet_ntoa(ip));
            close(newSocket);
            continue;
        }

        {
            std::lock_guard<std::mutex> guard(GMutex);
            printf("CHECKSYSTEM: Accepted connection: %s\n", inet_ntoa(ip));
            auto iter = GIpToSocket.find(ip);
            if(iter != GIpToSocket.end())
            {
                printf("CHECKSYSTEM: close previous connection\n");
                close(iter->second);
                GIpToSocket.erase(iter);
            }
            GIpToSocket.insert({ip, newSocket});
        }
    }
}


void InitChecksystem(const std::vector<IPAddr>& consolesIp)
{
    GConsolesIp = consolesIp;
    GNetworkThread = std::thread(NetworkThread);
}


bool Check(IPAddr ip)
{
    printf("  Checksystem:\n");
    int sock = -1;
    {
        std::lock_guard<std::mutex> guard(GMutex);
        auto iter = GIpToSocket.find(ip);
        if(iter == GIpToSocket.end())
        {
            printf("  ERROR: there is no connection\n");
            return false;
        }
        sock = iter->second;
    }

    Point2D v[3];
    for(uint32_t i = 0; i < 3; i++)
        v[i] = GeneratePoint();

    int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
    if(doubleTriArea < 0)
        std::swap(v[0], v[1]);

	int ret = Send(sock, v, sizeof(v));
    if(ret < 0)
    {
        std::lock_guard<std::mutex> guard(GMutex);
        GIpToSocket.erase(ip);
        close(sock);
        return false;
    }   

    uint16_t screen[kScreenSize * kScreenSize];
	ret = Recv(sock, screen, sizeof(screen));
    if(ret <= 0)
    {
        std::lock_guard<std::mutex> guard(GMutex);
        GIpToSocket.erase(ip);
        close(sock);
        return false;
    }

    if(RasterizeAndCompare(v, screen))
    {
        printf("  OK\n");
        return true;
    }
    else
    {
        printf("  Does no match\n");
        return false;
    }

    return false;
}
