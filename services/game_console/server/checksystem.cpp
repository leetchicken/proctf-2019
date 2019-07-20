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
#include "checksystem.h"


static const uint32_t kScreenSize = 24;


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


bool RasterizeAndCompare(Point2D* v, uint64_t authKey, uint16_t* screenToCompare)
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


int Recv(int sock, void* data, uint32_t size)
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
            return -1;
        }
		if((pollFd.revents & POLLIN) == 0)
		{
			printf("  ERROR: recv timeout\n");
			return -1;
		}
		
        ret = recv(sock, ptr, remain, 0);
        if(ret < 0)
        {
            printf("  ERROR: recv failed %s\n", strerror(errno));
            return -1;
        }
        remain -= ret;
        ptr += ret;
    }
    
    return 0;
}


int Send(int sock, void* data, uint32_t size)
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
            return -1;
        }
		if((pollFd.revents & POLLOUT) == 0)
		{
			printf("  ERROR: send timeout\n");
			return -1;
		}

        ret = send(sock, ptr, remain, 0);
        if(ret < 0)
        {
            printf("  ERROR: send failed %s\n", strerror(errno));
            return -1;
        }
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

    int sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (sock < 0)
	{
		printf("  ERROR: socket failed\n");
		return false;
	}

    int ret = connect(sock, (struct sockaddr*)&addr, sizeof(addr));
    if(ret < 0 && errno != EINPROGRESS)
	{
		printf("  ERROR: connect failed immediately: %s\n", strerror(errno));
        close(sock);
        return false;
	}

    // wait for connect
	pollfd pollFd;
	pollFd.fd = sock;
	pollFd.events = POLLIN | POLLOUT;
	pollFd.revents = 0;
    while(1)
    {
		ret = poll(&pollFd, 1, 3000);
        if(ret < 0)
        {
            printf("  ERROR: poll failed %s\n", strerror(errno));
            close(sock);
            return false;
        }

		if(pollFd.revents & POLLOUT)
        {
            int optval = -1;
			socklen_t optlen = sizeof(optval);

			if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen) == -1)
			{
                printf("  ERROR: getsockopt failed %s\n", strerror(errno));
                close(sock);
                return false;
            }		

            if(optval < 0)
            {
                printf("  ERROR: connect failed %s\n", strerror(optval));
                close(sock);
                return false;
            }

			break;
        }
		else
		{
			printf("  ERROR: connect timeout\n");
			close(sock);
			return false;
		}
    }


	ret = Send(sock, &authKey, sizeof(authKey));
    if(ret < 0)
    {
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
        close(sock);
        return false;
    }   

    uint16_t screen[kScreenSize * kScreenSize];
	ret = Recv(sock, screen, sizeof(screen));
    if(ret < 0)
    {
        close(sock);
        return false;
    }

    if(RasterizeAndCompare(v, authKey, screen))
    {
        printf("  OK\n");
		close(sock);
        return true;
    }
    else
    {
        printf("  Does no match\n");
		close(sock);
        return false;
    }

    return false;
}
