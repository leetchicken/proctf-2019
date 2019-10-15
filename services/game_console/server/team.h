#pragma once
#include <stdint.h>
#include <string>
#include <mutex>
#include <map>
#include <vector>
#include "notification.h"
#include "misc.h"
#include <arpa/inet.h>


struct Team
{
    uint32_t number;
    std::string name;
    NetworkAddr network;
    float lastTimeTeamPostNotification = 0.0f;
    uint32_t m_usersCount = 0;

    void LoadDb();

    void DumpStats(std::string& out, IPAddr hwConsoleIp);
    
    void PutFlag(const char* flagId, const char* flag);
    const char* GetFlag(const char* flagId);

private:
    std::mutex mutex;
    FILE* storage = nullptr;
    std::map<std::string, std::string> flags;
};


bool TeamsStart();
Team* FindTeam(in_addr ipAddr, bool showError = true);
Team* FindTeam(int idx);
void GetTeams(std::vector<Team*>& teams);