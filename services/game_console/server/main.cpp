#include <string.h>
#include <pugixml.hpp>
#include <arpa/inet.h>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <unordered_map>
#include <dirent.h>
#include "httpserver.h"
#include "png.h"
#include "checksystem.h"
#include "misc.h"
#include "notification.h"
#include "console.h"
#include "team.h"

struct GameDesc
{
    std::string name;
    std::string desc;
    std::vector<std::string> assets;
};


class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
};


class NotificationProcessor : public HttpPostProcessor
{
public:
    NotificationProcessor(const HttpRequest& request);
    virtual ~NotificationProcessor();

    int IteratePostData(MHD_ValueKind kind, const char *key, const char *filename, const char *contentType, const char *transferEncoding, const char *data, uint64_t offset, size_t size);
    void IteratePostData(const char* uploadData, size_t uploadDataSize);

    uint32_t m_contentLength = 0;
    char* m_content = nullptr;
    char* m_curContentPtr = nullptr;

    in_addr m_sourceIp;

protected:
    virtual void FinalizeRequest();
};


std::unordered_map<uint32_t, GameDesc> GGamesDatabase;
std::unordered_map<NetworkAddr, Team> GTeams;
std::mutex GConsolesGuard;
std::unordered_map<AuthKey, Console*> GConsoles;


static Team* FindTeam(in_addr ipAddr, bool showError = true)
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


static Console* GetConsole(AuthKey authKey)
{
    std::lock_guard<std::mutex> guard(GConsolesGuard);
    auto iter = GConsoles.find(authKey);
    if(iter == GConsoles.end())
        return nullptr;
    return iter->second;
}


static Console* CheckAuthority(const QueryString& queryString)
{
    static const std::string kAuth("auth");
    AuthKey authKey = ~0u;
    if(!FindInMap(queryString, kAuth, authKey, 16))
    {
        printf("  ERROR: Bad request\n");
        return nullptr;
    }
    printf("  auth key: %x\n", authKey);

    auto console = GetConsole(authKey);
    if(!console)
    {
        printf("  ERROR: Unauthorized access, auth key: %x\n", authKey);
        return nullptr;
    }

    return console;
}


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if (ParseUrl(request.url, 1, "auth"))
    {
        auto team = FindTeam(request.clientIp);
        if(!team)
            return HttpResponse(MHD_HTTP_FORBIDDEN);

        Console* console = team->GetConsole(request.clientIp.s_addr);
        if(console)
        {
            std::lock_guard<std::mutex> guard(GConsolesGuard);
            printf("  found existing console, auth key: %x\n", console->authKey);
            auto iter = GConsoles.find(console->authKey);
            if(iter == GConsoles.end())
            {
                printf("  CRITICAL ERROR, console was not found\n");
                exit(1);
            }
            GConsoles.erase(iter);
        }
        else
        {
            console = team->AddConsole(request.clientIp.s_addr);
        }

        console->GenerateAuthKey();
        printf("  auth key: %x\n", console->authKey);

        {
            std::lock_guard<std::mutex> guard(GConsolesGuard);
            if(GConsoles.find(console->authKey) != GConsoles.end())
            {
                printf("  CRITICAL ERROR, dublicate console was found, auth key: %x\n", console->authKey);
                exit(1);
            }
            GConsoles[console->authKey] = console;
        }

        struct
        {
            uint32_t authKey;
        } responseData;
        responseData.authKey = console->authKey;

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.content = (char*)malloc(sizeof(responseData));
        response.contentLength = sizeof(responseData);
        memcpy(response.content, &responseData, response.contentLength);
        return response;
    }
    else if (ParseUrl(request.url, 1, "list"))
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        const uint32_t gamesNum = GGamesDatabase.size();
        const uint32_t kResponseCapacity = 1024;
        uint8_t* buf = (uint8_t*)malloc(kResponseCapacity);
        memset(buf, 0, kResponseCapacity);
        uint8_t* ptr = buf;
        memcpy(ptr, (void*)&gamesNum, 4);
        ptr += 4;
        for(auto& gameIter : GGamesDatabase)
        {
            memcpy(ptr, (void*)&gameIter.first, 4);
            ptr += 4;
            uint32_t assetsNum = gameIter.second.assets.size(); 
            memcpy(ptr, (void*)&assetsNum, 4);
            ptr += 4;
            uint32_t len = gameIter.second.name.length() + 1;
            memcpy(ptr, gameIter.second.name.c_str(), len);
            ptr += len;
        }
		
        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.content = (char*)buf;
        response.contentLength = ptr - buf;
        return response;
    }
    else if(ParseUrl(request.url, 1, "icon"))
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        static const std::string kId("id");
        uint32_t id = ~0u;
        if(!FindInMap(request.queryString, kId, id, 16))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        printf("  id: %x\n", id);

        auto iter = GGamesDatabase.find(id);
        if(iter == GGamesDatabase.end())
        {
            printf("  unknown game\n");
            return HttpResponse(MHD_HTTP_NOT_FOUND);
        }

        std::string iconFilePath = "data/";
        iconFilePath.append(iter->second.name);
        iconFilePath.append("/icon.png");
        printf("  %s\n", iconFilePath.c_str());
        Image icon;
        if(!read_png(iconFilePath.c_str(), icon))
        {
            printf("  failed to read icon\n");
            return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        if(icon.width != kIconWidth || icon.height != kIconHeight)
        {
            printf("  invalid icon size: %ux%u\n", icon.width, icon.height);
            return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.contentLength = kIconWidth * kIconHeight * 4;
        response.content = (char*)malloc(response.contentLength);
        ABGR_to_ARGB(icon.abgr, (ARGB*)response.content, icon.width, icon.height);
        return response;
    }
    else if(ParseUrl(request.url, 1, "asset"))
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        static const std::string kId("id");
        static const std::string kIndex("index");

        uint32_t id = ~0u;
        if(!FindInMap(request.queryString, kId, id, 16))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        printf("  id: %x\n", id);

        uint32_t index = ~0u;
        if(!FindInMap(request.queryString, kIndex, index, 10))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        printf("  index: %u\n", index);

        auto iter = GGamesDatabase.find(id);
        if(iter == GGamesDatabase.end())
        {
            printf("  unknown game\n");
            return HttpResponse(MHD_HTTP_NOT_FOUND);
        }

        if(index >= iter->second.assets.size())
        {
            printf("  invalid asset index\n");
            return HttpResponse(MHD_HTTP_NOT_FOUND);
        }

        std::string assetName = iter->second.assets[index];
        std::string assetFilePath = "data/";
        assetFilePath.append(iter->second.name);
        assetFilePath.append("/");
        assetFilePath.append(assetName.c_str());
        printf("  %s\n", assetFilePath.c_str());
        Image asset;
        if(!read_png(assetFilePath.c_str(), asset))
        {
            printf("  failed to read asset\n");
            return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        char* extension = assetName.data() + (assetName.length() - 3);
        strcpy(extension, "bmp");

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.contentLength = assetName.length() + 1 + asset.width * asset.height * 4;
        response.content = (char*)malloc(response.contentLength);
        memset(response.content, 0, response.contentLength);

        char* ptr = response.content;
        memcpy(ptr, assetName.c_str(), assetName.length());
        ptr += assetName.length() + 1;
        ABGR_to_ARGB(asset.abgr, (ARGB*)ptr, asset.width, asset.height);

        return response;
    }
    else if(ParseUrl(request.url, 1, "code"))
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        static const std::string kId("id");
        uint32_t id = ~0u;
        if(!FindInMap(request.queryString, kId, id, 16))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        printf("  id: %x\n", id);

        auto iter = GGamesDatabase.find(id);
        if(iter == GGamesDatabase.end())
        {
            printf("  unknown game\n");
            return HttpResponse(MHD_HTTP_NOT_FOUND);
        }

        std::string codeFilePath = "data/";
        codeFilePath.append(iter->second.name);
        codeFilePath.append("/code.bin");
        printf("  %s\n", codeFilePath.c_str());
        FILE* f = fopen(codeFilePath.c_str(), "r");
	    if (!f)
        {
            printf( "failed to read code\n");
            return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);
        }

        fseek(f, 0, SEEK_END);
        size_t codeFileSize = ftell(f);
        fseek(f, 0, SEEK_SET);
        char* code = (char*)malloc(codeFileSize);
        fread(code, 1, codeFileSize, f);
        fclose(f);

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.content = code;
        response.contentLength = codeFileSize;
        return response;
    }
    else if(ParseUrl(request.url, 1, "notification"))
    {
        auto console = CheckAuthority(request.queryString);
        if(!console)
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        Notification* n = console->GetNotification();
        if(!n)
            return HttpResponse(MHD_HTTP_OK);

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "application/octet-stream"});
        response.content = (char*)malloc(n->notificationLen);
        memcpy(response.content, n->notification, n->notificationLen);
        response.contentLength = n->notificationLen;
        n->Release();
        return response;
    }
    else if(ParseUrl(request.url, 1, "checksystem_status"))
    {
        auto team = FindTeam(request.clientIp, false);
        if(team)
            return HttpResponse(MHD_HTTP_FORBIDDEN);

        std::string data;
        char buf[512];
        sprintf(buf, "Current time: %f\n\n", GetTime());
        data.append(buf);

        for(auto& iter : GTeams)
        {
            auto& team = iter.second;
            team.DumpStats(data);
        }

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        response.headers.insert({"Content-Type", "text/plain"});
        response.content = (char*)malloc(data.size());
        memcpy(response.content, data.c_str(), data.size());
        response.contentLength = data.size();
        return response;
    }
    else if(ParseUrl(request.url, 1, "checksystem_getflag"))
    {
        auto team = FindTeam(request.clientIp, false);
        if(team)
        {
            printf("  ERROR: Forbidden\n");
            return HttpResponse(MHD_HTTP_FORBIDDEN);
        }

        static std::string kAddr("addr");
		static std::string kFlagId("id");
		static std::string kFlag("flag");

        const char* addrStr = FindInMap(request.queryString, kAddr);
		if(!addrStr)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

		const char* flagId = FindInMap(request.queryString, kFlagId);
		if(!flagId)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

		in_addr addr;
        inet_aton(addrStr, (in_addr*)&addr);
        team = FindTeam(addr);
        if(!team)
        {
            printf("  ERROR: Bad request\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        auto& flag = team->GetFlag(flagId);
        printf("  Flag '%s' with id '%s' for team '%s'\n", flag.c_str(), flagId, team->desc.name.c_str());

        IPAddr consoleAddr = GetHwConsoleIp(team->desc.network);
        if(consoleAddr == ~0u)
        {
            printf("  ERROR: team does not have registerd hw console\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        if(!Check(team->desc.network))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);

		Console* console = team->GetConsole(consoleAddr);
        if(!console)
        {
            printf("  ERROR: hw console is not registered\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        if(console->GetNotificationsInQueue() >= Console::kNotificationQueueSize)
        {
            printf("  Notifications queue is overflowed\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        HttpResponse response;
        response.code = MHD_HTTP_OK;
        if(flag.size())
        {
            response.headers.insert({"Content-Type", "text/plain"});
            response.content = (char*)malloc(flag.size());
            memcpy(response.content, flag.c_str(), flag.size());
            response.contentLength = flag.size();
        }
        return response;
    }

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if(ParseUrl(request.url, 1, "notification")) 
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        uint32_t contentLength;
        static const std::string kContentLength("content-length");
        FindInMap(request.headers, kContentLength, contentLength);
        if(contentLength == 0 || contentLength > kMaxNotificationSize)
            return HttpResponse(MHD_HTTP_BAD_REQUEST);

		*postProcessor = new NotificationProcessor(request);
        return HttpResponse();
    }
    else if(ParseUrl(request.url, 1, "checksystem_notification")) 
    {
		static std::string kMessage("message");

		const char* message = FindInMap(request.queryString, kMessage);
		if(!message)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

        Notification* n = new Notification("Hackerdom", message);
        n->AddRef();
        for(auto& iter : GTeams)
        {
            Team& team = iter.second;
            team.AddNotification(n);
        }
        n->Release();

        return HttpResponse(MHD_HTTP_OK);
    }
    else if(ParseUrl(request.url, 1, "checksystem_putflag")) 
    {
        auto team = FindTeam(request.clientIp, false);
        if(team)
        {
            printf(" ERROR: Forbidden\n");
            return HttpResponse(MHD_HTTP_FORBIDDEN);
        }

        static std::string kAddr("addr");
		static std::string kFlagId("id");
		static std::string kFlag("flag");

        const char* addrStr = FindInMap(request.queryString, kAddr);
		if(!addrStr)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

		const char* flagId = FindInMap(request.queryString, kFlagId);
		if(!flagId)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

		const char* flag = FindInMap(request.queryString, kFlag);
		if(!flag)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

        char message[512];
        sprintf(message, "Here is your flag, keep it safe\n%s", flag);

        in_addr addr;
        inet_aton(addrStr, (in_addr*)&addr);
        team = FindTeam(addr);
        if(!team)
        {
            printf("  ERROR: Bad request\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        printf("  Flag '%s' with id '%s' for team '%s'\n", flag, flagId, team->desc.name.c_str());

        team->PutFlag(flagId, flag);
        Notification* n = new Notification("Hackerdom", message);
        team->AddNotification(n);

        return HttpResponse(MHD_HTTP_OK);
    }

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


NotificationProcessor::NotificationProcessor(const HttpRequest& request)
    : HttpPostProcessor(request)
{
    static const std::string kContentLength("content-length");
    FindInMap(request.headers, kContentLength, m_contentLength);

    if(m_contentLength)
        m_content = (char*)malloc(m_contentLength);
    m_curContentPtr = m_content;

    m_sourceIp = request.clientIp;
}


NotificationProcessor::~NotificationProcessor() 
{
    if(m_content)
        free(m_content);
}


void NotificationProcessor::FinalizeRequest() 
{
    if(!m_contentLength)
    {
        Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
		printf("  Bad request\n");
        return;
    }

    auto team = FindTeam(m_sourceIp);
    if(!team)
    {
        Complete(HttpResponse(MHD_HTTP_FORBIDDEN));
        return;
    }
    auto& teamDesc = team->desc;

    printf("  notification from Team %u %s\n", teamDesc.number, teamDesc.name.c_str());

    float curTime = GetTime();
    if(curTime - team->lastTimeTeamPostNotification < 1.0f)
    {
        team->lastTimeTeamPostNotification = curTime;
        printf("  too fast\n");

        const char* kTooFast = "Bad boy/girl, its too fast";
        char* tooFast = (char*)malloc(strlen(kTooFast) + 1);
        strcpy(tooFast, kTooFast);
        Complete(HttpResponse(MHD_HTTP_FORBIDDEN, tooFast, strlen(kTooFast), Headers()));

        return;
    }
    team->lastTimeTeamPostNotification = curTime;

    if(!Notification::Validate(m_content, m_contentLength))
    {
        printf("  corrupted notification\n");
        Complete(HttpResponse(MHD_HTTP_BAD_REQUEST));
        return;
    }

    Notification* notification = new Notification(m_content, m_contentLength);
    notification->AddRef();
    for(auto& iter : GTeams)
    {
        Team& team = iter.second;
        team.AddNotification(notification, m_sourceIp.s_addr);
    }
    notification->Release();

    Complete(HttpResponse(MHD_HTTP_OK));
}


int NotificationProcessor::IteratePostData( MHD_ValueKind kind, const char *key, const char *filename, const char *contentType,
                                            const char *transferEncoding, const char *data, uint64_t offset, size_t size ) 
{
    return MHD_YES;
}


void NotificationProcessor::IteratePostData(const char* uploadData, size_t uploadDataSize)
{
    if(uploadDataSize)
    {
        memcpy(m_curContentPtr, uploadData, uploadDataSize);
        m_curContentPtr += uploadDataSize;
    }
}


bool EnumerateAssets(const char* gameName, std::vector<std::string>& assets)
{
    char dirName[256];
    sprintf(dirName, "data/%s", gameName);

    DIR *dir;
    dirent *ent;
    if ((dir = opendir(dirName)) != NULL) 
    {
        while ((ent = readdir(dir)) != NULL) 
        {
            if(ent->d_type != DT_REG)
                continue;

            if(strcmp(ent->d_name, "icon.png") == 0)
                continue;

            int len = strlen(ent->d_name);
            if(len < 4) // less than ".png"
                continue;

            const char* ext = ent->d_name + (len - 4);
            if(strcmp(ext, ".png") != 0)
                continue;

            assets.push_back(ent->d_name);
        }
        closedir(dir);
    } 
    else 
    {
        printf("Failed to open directory %s\n", dirName);
        return false;
    }

    return true;
}


bool LoadGamesDatabase()
{
    pugi::xml_document doc;
    if (!doc.load_file("data/games.xml")) 
    {
        printf("Failed to load games database\n");
        return false;
    }

    printf("Games database:\n");
    pugi::xml_node gamesNode = doc.child("Games");
    for (pugi::xml_node gameNode = gamesNode.first_child(); gameNode; gameNode = gameNode.next_sibling())
    {
        uint32_t id = gameNode.attribute("id").as_uint();
        GameDesc desc;
        desc.name = gameNode.attribute("name").as_string();
        desc.desc = gameNode.attribute("desc").as_string();
        if(!EnumerateAssets(desc.name.c_str(), desc.assets))
            return false;
        GGamesDatabase.insert({id, desc});
        printf("  %x %s '%s'\n", id, desc.name.c_str(), desc.desc.c_str());
        printf("  assets:\n");
        if(desc.assets.empty())
            printf("   <none>\n");
        for(auto& assetName : desc.assets)
            printf("   %s\n", assetName.c_str());
    }

    return true;
}


bool LoadTeamsDatabase()
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
        auto& desc = team.desc;
        desc.number = teamNode.attribute("number").as_uint();
        desc.name = teamNode.attribute("name").as_string();
		desc.network = net;
        printf("  %u %s\n", desc.number, desc.name.c_str());
        printf("    network: %s(%08X)\n", netStr, net);
    }

    return true;
}


void UpdateThread()
{
    while(1)
    {
        for(auto& iter : GTeams)
        {
            Team& team = iter.second;
            team.Update();
        }
        sleep(1);
    }
}


void NetworkThread()
{
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0)
    {
        printf("NOTIFY: socket failed\n");
        return;
    }

    int opt_val = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kNotifyPort);
	inet_aton("0.0.0.0", &addr.sin_addr);
    int ret = bind(listenSock, (sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		printf("NOTIFY: bind failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

	ret = listen(listenSock, 128);
    if(ret < 0)
	{
		printf("NOTIFY: listen failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

    while(1)
    {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int newSocket = accept4(listenSock, (sockaddr*)&clientAddr, &addrLen, SOCK_NONBLOCK);
        if(newSocket < 0)
        {
            printf("NOTIFY: accept failed: %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        printf("NOTIFY: Connection from %s\n", inet_ntoa(clientAddr.sin_addr));

        Team* team = FindTeam(clientAddr.sin_addr);
        if(!team)
        {
            close(newSocket);
            continue;
        }

        Console* console = team->GetConsole(clientAddr.sin_addr.s_addr);
        if(!console)
        {
            printf("NOTIFY: Unregistered console\n");
            close(newSocket);
            continue;
        }

        console->SetNotifySocket(newSocket);
    }
}


int main()
{
    srand(time(nullptr));

    if(!LoadGamesDatabase() || !LoadTeamsDatabase())
        return -1;

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(kHttpPort);

    std::thread updateThread(UpdateThread);
    std::thread networkThread(NetworkThread);

    std::vector<NetworkAddr> teamsNet;
    for(auto& iter : GTeams)
        teamsNet.push_back(iter.second.desc.network);
    InitChecksystem(teamsNet);

    while (1)
        sleep(1);

    server.Stop();
    return 0;
}
