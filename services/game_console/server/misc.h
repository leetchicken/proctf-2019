#pragma once
#include <stdint.h>
#include <arpa/inet.h>

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kNetworkMask = 0x00FFFFFF;
static const uint32_t kConsoleAddr = 0x06000000;
static const uint16_t kHttpPort = 8000;
static const uint16_t kNotifyPort = 8001;
static const uint16_t kChecksystemPort = 8002;


using IPAddr = uint32_t;
using NetworkAddr = uint32_t;
using AuthKey = uint32_t;

float GetTime();
NetworkAddr SockaddrToNetworkAddr(in_addr a);
const char* inet_ntoa(IPAddr addr);