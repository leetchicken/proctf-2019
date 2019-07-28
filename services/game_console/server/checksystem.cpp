#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include <string.h>
#include <thread>
#include <mutex>
#include <map>
#include "checksystem.h"

struct HwConsole
{
    int socket = -1;
    IPAddr addr = ~0u;
};

static const uint32_t kScreenSize = 32;
static const uint32_t kBitsForCoord = 5;
static std::mutex GMutex;
static std::map<NetworkAddr, HwConsole> GConsoles;
static std::thread GNetworkThread;


struct Point2D
{
    int32_t x;
    int32_t y;
};


static Point2D GeneratePoint(Point2D& encodedV)
{
    Point2D ret;
    ret.x = rand() % kScreenSize;
    ret.y = rand() % kScreenSize;
    encodedV.x = (rand() << kBitsForCoord) | ret.x;
    encodedV.y = (rand() << kBitsForCoord) | ret.y;
    return ret;
}


static int32_t EdgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) 
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}


static bool RasterizeAndCompare(Point2D* v, uint32_t valToCompare)
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
    uint32_t result = 0;
    for(p.y = minY; p.y <= maxY; p.y++)
    {
        for(p.x = minX; p.x <= maxX; p.x++)
        {
            int32_t w0 = EdgeFunction(v[1], v[2], p);
            int32_t w1 = EdgeFunction(v[2], v[0], p);
            int32_t w2 = EdgeFunction(v[0], v[1], p);

            if((w0 | w1 | w2) >= 0) 
            {
                result += w0;
                result += w1 << std::min(p.x, 31);
                result += w2 << std::min(p.x + p.y, 31);
            }
        }
    }

    return result == valToCompare;
}


bool Check(int sock)
{
    Point2D v[3];
    Point2D encodedV[3];
    for(uint32_t i = 0; i < 3; i++)
        v[i] = GeneratePoint(encodedV[i]);

    int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
    if(doubleTriArea < 0)
    {
        std::swap(v[0], v[1]);
        std::swap(encodedV[0], encodedV[1]);
    }

	int ret = Send(sock, encodedV, sizeof(encodedV));
    if(ret < 0)
        return false; 

    uint32_t result;
	ret = Recv(sock, &result, sizeof(result));
    if(ret <= 0)
        return false;

    if(RasterizeAndCompare(v, result))
    {
        printf("CHECKSYSTEM: OK\n");
        return true;
    }
    else
    {
        printf("CHECKSYSTEM: Does no match\n");
        return false;
    }

    return false;
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

        printf("CHECKSYSTEM: Accepted connection: %s\n", inet_ntoa(clientAddr.sin_addr));

        if(!Check(newSocket))
        {
            close(newSocket);
            printf("CHECKSYSTEM: Is HW Console - no\n");
            continue;
        }
        printf("CHECKSYSTEM: Is HW Console - yes\n");

        NetworkAddr netAddr = SockaddrToNetworkAddr(clientAddr.sin_addr);

        {
            std::lock_guard<std::mutex> guard(GMutex);
            auto iter = GConsoles.find(netAddr);
            if(iter == GConsoles.end())
            {
                printf("CHECKSYSTEM: Unknown network: %s\n", inet_ntoa(netAddr));
                close(newSocket);
                continue;
            }
            if(iter->second.socket >= 0)
            {
                printf("CHECKSYSTEM: close previous connection\n");
                close(iter->second.socket);
            }
            iter->second.socket = newSocket;
            iter->second.addr = clientAddr.sin_addr.s_addr;
        }
    }
}


void InitChecksystem(const std::vector<NetworkAddr>& teamsNet)
{
    for(auto& net : teamsNet)
        GConsoles.insert({net, HwConsole()});
    GNetworkThread = std::thread(NetworkThread);
}


IPAddr GetHwConsoleIp(NetworkAddr teamNet)
{
    std::lock_guard<std::mutex> guard(GMutex);
    auto iter = GConsoles.find(teamNet);
    if(iter == GConsoles.end())
        return ~0u;
    return iter->second.addr;
}


bool Check(NetworkAddr teamNet)
{
    std::lock_guard<std::mutex> guard(GMutex);

    auto iter = GConsoles.find(teamNet);
    if(iter == GConsoles.end())
    {
        printf("CHECKSYSTEM: unkown network passed\n");
        return false;
    }
    int sock = iter->second.socket;

    bool ret = Check(sock);
    if(!ret)
    {
        close(sock);
        iter->second.socket = -1;
        iter->second.addr = ~0u;
    }

    return ret;
}
