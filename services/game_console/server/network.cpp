#include <string.h>
#include <poll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <unordered_map>
#include "network.h"
#include "misc.h"


void Socket::Close()
{
    state = kStateClosed;
}


int Socket::Send()
{
    void* ptr = &sendBuffer[sendBufferOffset];
    uint32_t remain = sendBufferSize - sendBufferOffset;
    int ret = send(fd, ptr, remain, 0);
    if(ret < 0)
        return ret;
    sendBufferOffset += ret;
    lastTouchTime = GetTime();
    return ret;
}


int Socket::Recv()
{
    void* ptr = &recvBuffer[recvBufferOffset];
    uint32_t remain = recvBufferSize - recvBufferOffset;
    int ret = recv(fd, ptr, remain, 0);
    if(ret <= 0)
        return ret;
    recvBufferOffset += ret;
    lastTouchTime = GetTime();
    return ret;
}


bool NetworkManager(AcceptConnectionCallback acceptConnection, uint16_t port)
{
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0)
    {
        printf("socket failed: %s\n", strerror(errno));
        return false;
    }

    int opt_val = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_aton("0.0.0.0", &addr.sin_addr);
    int ret = bind(listenSock, (sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		printf("bind failed: %s\n", strerror(errno));
		close(listenSock);
		return false;
	}

	ret = listen(listenSock, 128);
    if(ret < 0)
	{
		printf("listen failed: %s\n", strerror(errno));
		close(listenSock);
		return false;
	}

    std::vector<pollfd> pollFds;
    std::unordered_map<int, Socket*> sockets;
    std::vector<int> socketsToRemove;

    pollfd listenSockPollFd;
	listenSockPollFd.fd = listenSock;
	listenSockPollFd.events = POLLIN;
	listenSockPollFd.revents = 0;
	pollFds.push_back(listenSockPollFd);

    while(1)
    {
        socketsToRemove.clear();
        pollFds.clear();
        pollFds.push_back(listenSockPollFd);

        for(auto& iter : sockets)
        {
            auto socket = iter.second;

            if(socket->state == Socket::kStateSending || socket->state == Socket::kStateReceiving)
            {
                if(GetTime() - socket->lastTouchTime > 5.0f)
                {
                    socket->timeoutCallback(socket);
                    continue;
                }
            }

            socket->updateCallback(socket);

            pollfd pollFd;
            pollFd.fd = socket->fd;
            pollFd.events = 0;
            pollFd.revents = 0;

            if(socket->state == Socket::kStateReady)
            {
                pollFd.events |= POLLIN;
                pollFds.push_back(pollFd);
            }
            else if(socket->state == Socket::kStateSending)
            {
                pollFd.events |= POLLOUT;
                pollFds.push_back(pollFd);
            }
            else if(socket->state == Socket::kStateReceiving)
            {
                pollFd.events |= POLLIN;
                pollFds.push_back(pollFd);
            }
            else if(socket->state == Socket::kStateClosed)
                socketsToRemove.push_back(socket->fd);
        }

        for(int fd : socketsToRemove)
        {
            printf("close connection\n");
            close(fd);
            Socket* socket = sockets[fd];
            delete socket;
            sockets.erase(fd);
        }

        int ret = poll(pollFds.data(), pollFds.size(), 1);
		if(ret < 0)
		{
			printf("poll failed %s\n", strerror(errno));
			exit(1);
		}
        if(ret == 0)
            continue;

        if(pollFds[0].revents == POLLIN)
        {
            sockaddr_in clientAddr;
            socklen_t addrLen = sizeof(clientAddr);
            int fd = accept(listenSock, (sockaddr*)&clientAddr, &addrLen);
            if(fd < 0)
                printf("accept failed: %s\n", strerror(errno));
            else
            {
                Socket* socket = new Socket();
                socket->fd = fd;
                socket->lastTouchTime = GetTime();
                if(acceptConnection(socket, clientAddr))
                    sockets[fd] = socket;
                else
                {
                    close(fd);
                    delete socket;
                }
            }
        }

        for(size_t i = 1; i < pollFds.size(); i++)
        {
            auto& pollFd = pollFds[i];
            auto socket = sockets[pollFd.fd];
            if((pollFd.revents & POLLHUP) == POLLHUP)
            {
                socket->closedByPeerCallback(socket);
            }
            else if((pollFd.revents & POLLOUT) == POLLOUT && socket->state == Socket::kStateSending)
            {
                if(socket->Send() < 0)
                    socket->errorCallback(socket);
            }
            else if((pollFd.revents & POLLIN) == POLLIN && socket->state == Socket::kStateReceiving)
            {
                int ret = socket->Recv();
                if(ret == 0)
                    socket->closedByPeerCallback(socket);  
                else if(ret < 0)
                    socket->errorCallback(socket);
            }
            else if((pollFd.revents & POLLIN) == POLLIN && socket->state == Socket::kStateReady)
            {
                // flush data out, we dont expect that, we dont need that, or handle connection close
                uint8_t buf[16];
                int ret = recv(socket->fd, buf, sizeof(buf), 0);
                if(ret == 0)
                    socket->closedByPeerCallback(socket);
                else if(ret < 0)
                    socket->errorCallback(socket);
            }
        }
    }

    return true;
}