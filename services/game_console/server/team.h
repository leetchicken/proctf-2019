#pragma once
#include <stdint.h>
#include <string>
#include <mutex>
#include <map>
#include "notification.h"
#include "misc.h"


struct TeamDesc
{
    uint32_t number;
    std::string name;
    NetworkAddr network;
};


struct Team
{
    TeamDesc desc;
    float lastTimeTeamPostNotification = 0.0f;

    void LoadDb();

    void DumpStats(std::string& out, IPAddr hwConsoleIp);
    
    void PutFlag(const char* flagId, const char* flag);
    const char* GetFlag(const char* flagId);

private:
    std::mutex mutex;
    FILE* storage = nullptr;
    std::map<std::string, std::string> flags;
};