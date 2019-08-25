#include "team.h"
#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <pugixml.hpp>


static const uint32_t kFlagIdLen = 8;
static const uint32_t kFlagLen = 32;
static const uint32_t kRecordSize = kFlagIdLen + kFlagLen;


void Team::LoadDb()
{
    char filename[256];
    sprintf(filename, "data/%s.dat", name.c_str());

    bool error = false;
    storage = fopen(filename, "r+");
	if(storage)
	{
		fseek(storage, 0, SEEK_END);
		size_t fileSize = ftell(storage);
		fseek(storage, 0, SEEK_SET);

        if((fileSize % kRecordSize) == 0)
        {
            uint32_t recordsNum = fileSize / kRecordSize;
            for(uint32_t i = 0; i < recordsNum; i++)
            {
                char flagId[kFlagIdLen + 1];
                memset(flagId, 0, sizeof(flagId));
				if(fread(flagId, kFlagIdLen, 1, storage) != 1)
                {
                    error = true;
                    printf("Failed to read storage %s\n", filename);
                    break;
                }

                char flag[kFlagLen + 1];
                memset(flag, 0, sizeof(flag));
				if(fread(flag, kFlagLen, 1, storage) != 1)
                {
                    error = true;
                    printf("Failed to read storage %s\n", filename);
                    break;
                }

                flags.insert({flagId, flag});
            }
        }
        else
        {
            printf("Storage corrupted %s\n", filename);
            error = true;
        }

        printf("Storage has been read succefully %s\n", filename);
    }
    else
    {
        printf("Storage does not exists %s\n", filename);
        error = true;
    }

    if(error)
	{
		FILE* c = fopen(filename, "w");
		fclose(c);
		storage = fopen(filename, "r+");
	}
}


void Team::DumpStats(std::string& out, IPAddr hwConsoleIp)
{
    std::lock_guard<std::mutex> guard(mutex);

    char buf[512];
    sprintf(buf, "Team%u '%s':\n", number, name.c_str());
    out.append(buf);

    sprintf(buf, "  Network: %s\n", inet_ntoa(network));
    out.append(buf);

    sprintf(buf, "  Last time team post notification: %f\n", lastTimeTeamPostNotification);
    out.append(buf);

    sprintf(buf, "  Number of flags: %u\n", (uint32_t)flags.size());
    out.append(buf);

    sprintf(buf, "  HW Console IP: %s\n\n", inet_ntoa(hwConsoleIp));
    out.append(buf);
}


void Team::PutFlag(const char* flagId, const char* flag)
{
    std::lock_guard<std::mutex> guard(mutex);

    if(strlen(flagId) != kFlagIdLen || strlen(flag) != kFlagLen)
    {
        printf("CRITICAL ERROR: Invalid length of flag id or flag: %s %s\n", flagId, flag);
        exit(1);
    }

    uint32_t offset = flags.size() * kRecordSize;
    fseek(storage, offset, SEEK_SET);
    fwrite(flagId, kFlagIdLen, 1, storage);
    fwrite(flag, kFlagLen, 1, storage);
    fflush(storage);

    flags.insert({flagId, flag});
}


const char* Team::GetFlag(const char* flagId)
{
    std::lock_guard<std::mutex> guard(mutex);
    auto iter = flags.find(flagId);
    if(iter == flags.end())
        return nullptr;
    return iter->second.c_str();
}


std::unordered_map<NetworkAddr, Team> GTeams;


static bool LoadTeamsDatabase()
{
    pugi::xml_document doc;
    if (!doc.load_file("data/teams.xml")) 
    {
        printf("Failed to teams load database\n");
        return false;
    }

    printf("Teams database:\n");
    pugi::xml_node teamsNode = doc.child("Teams");
    for (pugi::xml_node teamNode = teamsNode.first_child(); teamNode; teamNode = teamNode.next_sibling())
    {
        const char* netStr = teamNode.attribute("net").as_string();
        NetworkAddr net = inet_aton(netStr);

        Team& team = GTeams[net];
        team.number = teamNode.attribute("number").as_uint();
        team.name = teamNode.attribute("name").as_string();
        if(team.name.length() > kMaxUserNameLen)
        {
            printf("  ERROR: too long user name: %s\n", team.name.c_str());
            exit(1);
        }
		team.network = net;
        printf("  %u %s\n", team.number, team.name.c_str());
        printf("    network: %s(%08X)\n", netStr, net);
        team.LoadDb();
    }

    return true;
}


bool TeamsStart()
{
    return LoadTeamsDatabase();
}


Team* FindTeam(in_addr ipAddr, bool showError)
{
    NetworkAddr netAddr = SockaddrToNetworkAddr(ipAddr);
    auto iter = GTeams.find(netAddr);
    if(iter == GTeams.end())
    {
        if(showError)
            printf("  ERROR: Unknown network: %s\n", inet_ntoa(ipAddr));
        return nullptr;
    }
    return &iter->second;
}


void GetTeams(std::vector<Team*>& teams)
{
    teams.reserve(GTeams.size());
    for(auto& iter : GTeams)
        teams.push_back(&iter.second);
}