#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <map>
#include "checksystem.h"
#include "network.h"
#include "../common.h"
#include "log.h"


struct HwConsole
{
    HwConsole() = default;

    std::mutex mutex;
    Socket* socket = nullptr;
    IPAddr addr = ~0u;
    bool checkInProgress = false;
    uint32_t rasterResult = 0;
    bool checkResult = false;

    void StartCheck();
    void Reset();
};


static std::map<NetworkAddr, HwConsole> GConsoles;
static std::thread GNetworkThread;


static Point2D GeneratePoint(Point2D& encodedV)
{
    Point2D ret;
    ret.x = rand() % kScreenSize;
    ret.y = rand() % kScreenSize;
    encodedV.x = (rand() << kBitsForCoord) | ret.x;
    encodedV.y = (rand() << kBitsForCoord) | ret.y;
    return ret;
}


void HwConsole::StartCheck()
{
    std::lock_guard<std::mutex> guard(mutex);

    if(checkInProgress || !socket)
        return;

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

    socket->sendBufferSize = sizeof(encodedV);
    socket->sendBufferOffset = 0;
    memcpy(socket->sendBuffer, encodedV, sizeof(encodedV));
    socket->state = Socket::kStateSending;
    socket->lastTouchTime = GetTime();

    rasterResult = Rasterize(v);
    checkResult = false;
    checkInProgress = true;
}


void HwConsole::Reset()
{
    socket->Close();
    socket = nullptr;
    addr = ~0u;
    checkResult = false;
    checkInProgress = false;
}


static bool AcceptConnection(Socket* socket, const sockaddr_in& clientAddr)
{
    Log("CHECKSYSTEM: Accepted connection from: %s\n", inet_ntoa(clientAddr.sin_addr));

    NetworkAddr netAddr = SockaddrToNetworkAddr(clientAddr.sin_addr);
    auto iter = GConsoles.find(netAddr);
    if(iter == GConsoles.end())
    {
        Log("CHECKSYSTEM: unknown network, close connection\n");
        return false;
    }
    
    HwConsole* console = &iter->second;
    {
        std::lock_guard<std::mutex> guard(console->mutex);
        if(console->socket)
        {
            Log("CHECKSYSTEM: close previous connection\n");
            console->socket->Close();
        }

        console->socket = socket;
        console->addr = clientAddr.sin_addr.s_addr;
        console->checkInProgress = false;
        console->checkResult = false;
    }
    socket->userData = console;

    console->StartCheck();

    socket->updateCallback = [](Socket* socket){
        if(socket->state == Socket::kStateSending)
        {
            if(socket->sendBufferOffset == socket->sendBufferSize)
            {
                socket->state = Socket::kStateReceiving;
                socket->recvBufferSize = sizeof(uint32_t);
                socket->recvBufferOffset = 0;
            }
        }
        else if(socket->state == Socket::kStateReceiving)
        {
            if(socket->recvBufferOffset == socket->recvBufferSize)
            {
                uint32_t result;
                memcpy(&result, socket->recvBuffer, sizeof(uint32_t));
                HwConsole* console = (HwConsole*)socket->userData;

                std::lock_guard<std::mutex> guard(console->mutex);

                if(console->rasterResult == result)
                {
                    Log("CHECKSYSTEM: OK\n");
                    socket->state = Socket::kStateReady;
                    console->checkResult = true;
                }
                else
                {
                    Log("CHECKSYSTEM: Does no match\n");
                    console->Reset();
                }

                console->checkInProgress = false;
            }
        }
    };

    socket->timeoutCallback = [](Socket* socket){
        Log("CHECKSYSTEM: socket timeout\n");
        HwConsole* console = (HwConsole*)socket->userData;
        std::lock_guard<std::mutex> guard(console->mutex);
        console->Reset();
    };
    socket->closedByPeerCallback = [](Socket* socket){
        Log("CHECKSYSTEM: connection closed by peer\n");
        HwConsole* console = (HwConsole*)socket->userData;
        std::lock_guard<std::mutex> guard(console->mutex);
        console->Reset();
    };
    socket->errorCallback = [](Socket* socket){
        Log("CHECKSYSTEM: socket error: %s\n", strerror(errno));
        HwConsole* console = (HwConsole*)socket->userData;
        std::lock_guard<std::mutex> guard(console->mutex);
        console->Reset();
    };

    return true;
}


void InitChecksystem(const std::vector<NetworkAddr>& teamsNet)
{
    for(auto& net : teamsNet)
        GConsoles[net];
    GNetworkThread = std::thread(NetworkManager, AcceptConnection, kChecksystemPort);
}


IPAddr GetHwConsoleIp(NetworkAddr teamNet)
{
    auto iter = GConsoles.find(teamNet);
    if(iter == GConsoles.end())
        return ~0u;
    return iter->second.addr;
}


bool Check(NetworkAddr teamNet)
{
    auto iter = GConsoles.find(teamNet);
    if(iter == GConsoles.end())
    {
        Log("CHECKSYSTEM: unkown network passed\n");
        return false;
    }

    HwConsole* console = &iter->second;
    console->StartCheck();
    while(1)
    {
        {
            std::lock_guard<std::mutex> guard(console->mutex);
            if(!console->checkInProgress)
                return console->checkResult;
        }

        usleep(1000);
    }

    return false;
}
