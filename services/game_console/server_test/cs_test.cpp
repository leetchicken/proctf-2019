#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "../common.h"


int main(int argc, char* argv[])
{
    if(argc < 3)
        return -1;

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    inet_aton(argv[1], &addr.sin_addr);
    addr.sin_port = htons(atoi(argv[2]));

    int sock = -1;

    while(1)
    {
        if(sock < 0)
        {
            sock = socket(AF_INET, SOCK_STREAM, 0);
            int ret = connect(sock, (sockaddr*)&addr, sizeof(addr));
            if(ret < 0)
            {
                printf("connect: %s\n", strerror(errno));
                close(sock);
                sock = -1;
                sleep(1);
                continue;
            }
        }

        Point2D v[3];
        int ret = recv(sock, &v, sizeof(v), 0);
        if(ret == 0)
        {
            printf("connection closed by peer, reconnect\n");
            close(sock);
            sock = -1;       
            continue;
        }
        if(ret < 0)
        {
            printf("recv: %s\n", strerror(errno));
            close(sock);
            sock = -1;       
            continue;
        }
        printf("recv points\n");

        const uint32_t kMask = (1 << kBitsForCoord) - 1;
        for(uint32_t i = 0; i < 3; i++)
        {
            v[i].x = v[i].x & kMask;
            v[i].y = v[i].y & kMask;
        }

        uint32_t result = Rasterize(v);
        send(sock, &result, sizeof(result), 0);
    }

    return 0;
}