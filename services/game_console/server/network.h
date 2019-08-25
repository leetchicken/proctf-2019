#pragma once
#include <functional>
#include <netinet/in.h>


enum EUserErrorCodes
{
    kUserErrorOk = 0,
    kUserErrorTooLong,
    kUserErrorAlreadyExists,
    kUserErrorInvalidCredentials,
    kUserErrorInvalidAuthKey,
    kUserErrorUnauthorized,
    kUserErrorForbidden,
};


struct Socket
{
    using Callback = std::function<void(Socket* socket)>;

    enum EState
    {
        kStateClosed = 0,
        kStateReady,
        kStateSending,
        kStateReceiving,
    };

    EState state = kStateClosed;
    int fd = -1;
    uint8_t sendBuffer[32];
    uint32_t sendBufferSize = 0;
    uint32_t sendBufferOffset = 0;
    uint8_t recvBuffer[32];
    uint32_t recvBufferSize = 0;
    uint32_t recvBufferOffset = 0;
    float lastTouchTime = 0.0f;

    void* userData = nullptr;
    Callback updateCallback;
    Callback timeoutCallback;
    Callback closedByPeerCallback;
    Callback errorCallback;

    void Close();
    int Send();
    int Recv();
};


using AcceptConnectionCallback = std::function<bool (Socket* socket, const sockaddr_in& clientAddr)>;
bool NetworkManager(AcceptConnectionCallback acceptConnection, uint16_t port);