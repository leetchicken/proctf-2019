#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>
#include "checksystem.h"


static const uint32_t kScreenSize = 32;


struct Point2D
{
    int32_t x;
    int32_t y;
};


Point2D GeneratePoint()
{
    Point2D ret;
    ret.x = rand() % kScreenSize;
    ret.y = rand() % kScreenSize;
    return ret;
}


int32_t EdgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) 
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}


bool RasterizeAndCompare(Point2D* v, uint64_t authKey, uint32_t* screenToCompare)
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
                pixel = pixel ^ (uint32_t)authKey;
                uint32_t pixel1 = screenToCompare[p.y * kScreenSize + p.y];
                if(pixel != pixel1)
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


int Recv(int sock, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

    while(remain)
    {
        int ret = recv(sock, ptr, remain, 0);
        if(ret < 0)
            return -1;
        remain -= ret;
        ptr += ret;
    }
    
    return 0;
}


int Send(int sock, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

    while(remain)
    {
        int ret = send(sock, ptr, remain, 0);
        if(ret < 0)
            return -1;
        remain -= ret;
        ptr += ret;
    }
    
    return 0;
}


bool Check(in_addr ip, uint16_t port, uint64_t authKey)
{
    printf("  Checksystem:\n");
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr = ip;
    addr.sin_port = htons(port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		printf("  ERROR: socket failed\n");
		return false;
	}

	if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
	{
		printf("  ERROR: connect failed\n");
        close(sock);
        return false;
	}

    int ret = Send(sock, &authKey, sizeof(authKey));
    if(ret < 0)
    {
        printf("  ERROR: send failed\n");
        close(sock);
        return false;
    }

    Point2D v[3];
    for(uint32_t i = 0; i < 3; i++)
        v[i] = GeneratePoint();

    int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
    if(doubleTriArea < 0)
        std::swap(v[0], v[1]);

    ret = Send(sock, v, sizeof(v));
    if(ret < 0)
    {
        printf("  ERROR: send failed\n");
        close(sock);
        return false;
    }   

    uint32_t screen[kScreenSize * kScreenSize];
    ret = Recv(sock, screen, sizeof(screen));
    if(ret < 0)
    {
        printf("  ERROR: send failed\n");
        close(sock);
        return false;
    }

    if(RasterizeAndCompare(v, authKey, screen))
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