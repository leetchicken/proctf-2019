#include "team.h"
#include "checksystem.h"


Console* Team::AddConsole(IPAddr consoleIp)
{
    std::lock_guard<std::mutex> guard(mutex);
    Console* console = new Console();
    console->ipAddr = consoleIp;
    consoles[consoleIp] = console;
    return console;
}


Console* Team::GetConsole(IPAddr consoleIp)
{
    std::lock_guard<std::mutex> guard(mutex);
    auto iter = consoles.find(consoleIp);
    if(iter == consoles.end())
        return nullptr;
    return iter->second;
}


void Team::AddNotification(Notification* n, IPAddr except)
{
    std::lock_guard<std::mutex> guard(mutex);
    for(auto& iter : consoles)
    {
        if(iter.first == except)
            continue;
        iter.second->AddNotification(n);
    }
}


void Team::Update()
{
    std::lock_guard<std::mutex> guard(mutex);
    for(auto& iter : consoles)
        iter.second->Update();
}


void Team::DumpStats(std::string& out)
{
    std::lock_guard<std::mutex> guard(mutex);

    char buf[512];
    sprintf(buf, "Team%u %s:\n", desc.number, desc.name.c_str());
    out.append(buf);

    sprintf(buf, "  Network: %s\n", inet_ntoa(desc.network));
    out.append(buf);

    sprintf(buf, "  Last time team post notification: %f\n", lastTimeTeamPostNotification);
    out.append(buf);

    sprintf(buf, "  Number of flags: %u\n\n", (uint32_t)flags.size());
    out.append(buf);

    IPAddr hwConsoleIp = GetHwConsoleIp(desc.network);

    for(auto& iter : consoles)
    {
        auto& c = iter.second;

        sprintf(buf, "  Console %s:\n", inet_ntoa(iter.first));
        out.append(buf);

        bool isHw = c->ipAddr == hwConsoleIp;
        sprintf(buf, "    Is HW: %s\n", isHw ? "yes" : "no");
        out.append(buf);

        sprintf(buf, "    Notifications in queue: %u\n", c->GetNotificationsInQueue());
        out.append(buf);

        sprintf(buf, "    Last console notify time: %f\n", c->lastConsoleNotifyTime);
        out.append(buf);

        sprintf(buf, "    Auth key: %x\n\n", c->authKey);
        out.append(buf);
    }
}

void Team::PutFlag(const char* flagId, const char* flag)
{
    flags.insert({flagId, flag});
}

const char* Team::GetFlag(const char* flagId)
{
    auto iter = flags.find(flagId);
    if(iter == flags.end())
        return nullptr;
    return iter->second.c_str();
}