#pragma once
#include <stdint.h>
#include <string>
#include <mutex>
#include <map>
#include "notification.h"
#include "user.h"
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

    User* AddUser(const std::string& name, const std::string& password);
    User* GetUser(const std::string& name);
    User* GetUser(IPAddr ipAddr);
    bool AuthorizeUser(const std::string& name, const std::string& password);

    void AddNotification(Notification* n, const std::string& except = 0);
    void Update();
    void DumpStats(std::string& out);
    
    void PutFlag(const char* flagId, const char* flag);
    const char* GetFlag(const char* flagId);

private:
    std::mutex mutex;
    FILE* storage = nullptr;
    std::map<std::string, User*> users;
    std::map<std::string, std::string> flags;
};