#pragma once
#include <stdint.h>
#include <string>
#include <mutex>
#include <map>
#include "notification.h"
#include "console.h"
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

    Console* AddConsole(IPAddr consoleIp);
    Console* GetConsole(IPAddr consoleIp);
    void AddNotification(Notification* n, IPAddr except = 0);
    void Update();
    void DumpStats(std::string& out);
    
    void PutFlag(const char* flagId, const char* flag);
    const char* GetFlag(const char* flagId);

private:
    std::mutex mutex;
    std::map<IPAddr, Console*> consoles;
    std::map<std::string, std::string> flags;
};