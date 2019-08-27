#pragma once
#include <stdint.h>
#include <arpa/inet.h>

using IPAddr = uint32_t;
using NetworkAddr = uint32_t;
using AuthKey = uint32_t;

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kNetworkMask = 0x00FFFFFF;
static const uint16_t kHttpPort = 80;
static const uint16_t kNotifyPort = 8001;
static const uint16_t kChecksystemPort = 8002;
static const uint32_t kMaxNotificationSize = 1024;
static const uint32_t kMaxUserNameLen = 64;
static const uint32_t kMaxPasswordLen = 32;
static const AuthKey kInvalidAuthKey = ~0u;

enum ECheckErrorCodes
{
    kCheckerOk = 101,
    kCheckerCorrupt = 102,
    kCheckerMumble = 103,
    kCheckerDown = 104,
    kCheckerCheckerError = 110,
};

float GetTime();
NetworkAddr SockaddrToNetworkAddr(in_addr a);
const char* inet_ntoa(IPAddr addr);
IPAddr inet_aton(const char*);
char* ReadFile(const char* fileName, uint32_t& size);
