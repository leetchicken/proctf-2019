#include <string.h>
#include <pugixml.hpp>
#include <arpa/inet.h>
#include <mutex>
#include <thread>
#include <list>
#include "httpserver.h"
#include "png.h"

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kNetworkMask = 0x00FFFFFF;
static const uint32_t kConsoleAddr = 0x06000000;


float GetTime()
{
    timespec tp;
    memset(&tp, 0, sizeof(tp));
    clock_gettime(CLOCK_MONOTONIC, &tp);
    return tp.tv_sec + tp.tv_nsec / 1000000000.0;
}

using IPAddr = uint32_t;
using NetworkAddr = uint32_t;
using AuthKey = uint32_t;

NetworkAddr SockaddrToNetworkAddr(in_addr a)
{
    return a.s_addr & kNetworkMask;
}

const char* inet_ntoa(IPAddr addr)
{
    return inet_ntoa(*(in_addr*)&addr);
}

struct GameDesc
{
    std::string name;
    std::string desc;
};

struct TeamDesc
{
    uint32_t number;
    std::string name;
    std::string networkStr;
    NetworkAddr network;
};

struct Notification
{
    char* notification = nullptr;
    uint32_t notificationLen = 0;

    mutable uint32_t refCount = 0;

    Notification(const char* userName, const char* message)
    {
        uint32_t userNameLen = strlen(userName);
        uint32_t messageLen = strlen(message);

        notificationLen = sizeof(uint32_t) + userNameLen + sizeof(uint32_t) + messageLen;
        notification = (char*)malloc(notificationLen);

        char* ptr = notification;
        memcpy(ptr, &userNameLen, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        memcpy(ptr, userName, userNameLen);
        ptr += userNameLen;

        memcpy(ptr, &messageLen, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        memcpy(ptr, message, messageLen);
    }

    Notification(void* data, uint32_t dataSize)
    {
        notification = (char*)malloc(dataSize);
        memcpy(notification, data, dataSize);
        notificationLen = dataSize;
    }

    ~Notification()
    {
        if(notification)
            free(notification);
        notification = nullptr;
        notificationLen = 0;
    }

    void AddRef() const
    {
        __sync_add_and_fetch(&refCount, 1);
    }

    uint32_t Release() const
    {
        uint32_t count = __sync_sub_and_fetch(&refCount, 1);
        if ((int32_t)count <= 0)
            delete this;
        return count;
    }

    static bool Validate(void* data, uint32_t dataSize)
    {
        if(dataSize < sizeof(uint32_t))
            return false;

        char* ptr = (char*)data;
        uint32_t userNameLen = 0, messageLen = 0;
        memcpy(&userNameLen, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        ptr += userNameLen;

        if(2 * sizeof(uint32_t) + userNameLen > dataSize)
            return false;

        memcpy(&messageLen, ptr, sizeof(uint32_t));
        
        return 2 * sizeof(uint32_t) + userNameLen + messageLen == dataSize;
    }
};

struct Console
{
    IPAddr ipAddr;
    float lastConsoleNotifyTime = 0.0f;
    AuthKey authKey = ~0u;
    uint16_t notifyPort = 0xffff;
    static const uint32_t kNotificationQueueSize = 32;

    void Auth()
    {
        authKey = 0;
        for(uint32_t i = 0; i < 4; i++)
            authKey |= (rand() % 255) << (i * 8);

        notifyPort = 0;
        for(uint32_t i = 0; i < 2; i++)
            notifyPort |= (rand() % 255) << (i * 8);
    }

    bool AddNotification(Notification* n)
    {
        std::lock_guard<std::mutex> guard(mutex);
        if(notifications.size() >= kNotificationQueueSize)
        {
            printf("  Console %s: notification queue overflowed\n", inet_ntoa(ipAddr));
            return false;
        }
        notifications.push_back(n);
        n->AddRef();

        NotifyConsole();

        return true;
    }

    Notification* GetNotification()
    {
        std::lock_guard<std::mutex> guard(mutex);
        if(notifications.empty())
            return nullptr;
        auto* retVal = notifications.front();
        notifications.pop_front();
        return retVal;
    }

    uint32_t GetNotificationsInQueue()
    {
        std::lock_guard<std::mutex> guard(mutex);
        return notifications.size();
    }

    void Update()
    {
        std::lock_guard<std::mutex> guard(mutex);
        float dt = GetTime() - lastConsoleNotifyTime;
        if(!notifications.empty() && dt > 5.0f)
            NotifyConsole();
    }

private:
    std::mutex mutex;
    std::list<Notification*> notifications;

    void NotifyConsole()
    {
        lastConsoleNotifyTime = GetTime();

        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if(sock < 0)
        {
            printf("  Failed to create socket\n");
            return;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ipAddr;
        addr.sin_port = ntohs(notifyPort);
        char randomData[16];
        sendto(sock, randomData, 16, 0, (sockaddr*)&addr, sizeof(addr));
        close(sock);
    }
};

struct Team
{
    TeamDesc desc;
    float lastTimeTeamPostNotification = 0.0f;

    Console* AddConsole(IPAddr consoleIp)
    {
        std::lock_guard<std::mutex> guard(mutex);
        Console* console = new Console();
        console->ipAddr = consoleIp;
        consoles[consoleIp] = console;
        return console;
    }

    Console* GetConsole(IPAddr consoleIp)
    {
        std::lock_guard<std::mutex> guard(mutex);
        auto iter = consoles.find(consoleIp);
        if(iter == consoles.end())
            return nullptr;
        return iter->second;
    }

    void AddNotification(Notification* n, IPAddr except = 0)
    {
        std::lock_guard<std::mutex> guard(mutex);
        for(auto& iter : consoles)
        {
            if(iter.first == except)
                continue;
            iter.second->AddNotification(n);
        }
    }

    void Update()
    {
        std::lock_guard<std::mutex> guard(mutex);
        for(auto& iter : consoles)
            iter.second->Update();
    }

    void DumpStats(std::string& out)
    {
        std::lock_guard<std::mutex> guard(mutex);

        char buf[512];
        sprintf(buf, "Team%u %s:\n", desc.number, desc.name.c_str());
        out.append(buf);

        sprintf(buf, "  Network: %s\n", desc.networkStr.c_str());
        out.append(buf);

        sprintf(buf, "  Last time team post notification: %f\n", lastTimeTeamPostNotification);
        out.append(buf);

        for(auto& iter : consoles)
        {
            auto& c = iter.second;

            sprintf(buf, "  Console %s:\n", inet_ntoa(iter.first));
            out.append(buf);

            sprintf(buf, "    Notifications in queue: %u\n", c->GetNotificationsInQueue());
            out.append(buf);

            sprintf(buf, "    Last console notify time: %f\n", c->lastConsoleNotifyTime);
            out.append(buf);

            sprintf(buf, "    Notify port: %u\n", c->notifyPort);
            out.append(buf);

            sprintf(buf, "    Auth key: %x\n\n", c->authKey);
            out.append(buf);
        }
    }

private:
    std::mutex mutex;
    std::map<IPAddr, Console*> consoles;
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


std::map<uint32_t, GameDesc> GGamesDatabase;
std::map<NetworkAddr, Team> GTeams;
std::mutex GConsolesGuard;
std::map<AuthKey, Console*> GConsoles;


static Team* FindTeam(in_addr ipAddr)
{
    NetworkAddr netAddr = SockaddrToNetworkAddr(ipAddr);
    auto iter = GTeams.find(netAddr);
    if(iter == GTeams.end())
    {
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
    FindInMap(queryString, kAuth, authKey, 16);
    printf("  auth key=%x\n", authKey);

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

        console->Auth();
        printf("  auth key=%x\n", console->authKey);
        printf("  notify port=%u\n", console->notifyPort);

        {
            std::lock_guard<std::mutex> guard(GConsolesGuard);
            if(GConsoles.find(console->authKey) !=  GConsoles.end())
            {
                printf("  CRITICAL ERROR, dublicate console was found, auth key: %x\n", console->authKey);
                exit(1);
            }
            GConsoles[console->authKey] = console;
        }

        struct
        {
            uint32_t authKey;
            uint16_t notifyPort;
        } responseData;
        responseData.authKey = console->authKey;
        responseData.notifyPort = console->notifyPort;

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
        FindInMap(request.queryString, kId, id, 16);
        printf("  id=%x\n", id);

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
    else if(ParseUrl(request.url, 1, "code"))
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        static const std::string kId("id");
        uint32_t id = ~0u;
        FindInMap(request.queryString, kId, id, 16);
        printf("  id=%x\n", id);

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

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if(ParseUrl(request.url, 1, "notification")) 
    {
        if(!CheckAuthority(request.queryString))
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

		*postProcessor = new NotificationProcessor(request);
        return HttpResponse();
    }
    else if(ParseUrl(request.url, 1, "checksystem_notification")) 
    {
		static std::string userNameStr("username");
		static std::string message("notification");

		const char* userName = FindInMap(request.queryString, userNameStr);
		if(!userName)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

		const char* notification = FindInMap(request.queryString, message);
		if(!notification)
			return HttpResponse(MHD_HTTP_BAD_REQUEST);

        Notification* n = new Notification(userName, notification);
        n->AddRef();
        for(auto& iter : GTeams)
        {
            Team& team = iter.second;
            team.AddNotification(n);
        }
        n->Release();

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
        GGamesDatabase.insert({id, desc});
        printf("  %x %s '%s'\n", id, desc.name.c_str(), desc.desc.c_str());
    }

    return true;
}


bool LoadTeamsDatabase()
{
    pugi::xml_document doc;
    if (!doc.load_file("teams.xml")) 
    {
        printf("Failed to teams load database\n");
        return false;
    }

    printf("Teams database:\n");
    pugi::xml_node teamsNode = doc.child("Teams");
    for (pugi::xml_node teamNode = teamsNode.first_child(); teamNode; teamNode = teamNode.next_sibling())
    {
        const char* netStr = teamNode.attribute("net").as_string();
        NetworkAddr net;
        inet_aton(netStr, (in_addr*)&net);

        Team& team = GTeams[net];
        auto& desc = team.desc;
        desc.number = teamNode.attribute("number").as_uint();
        desc.name = teamNode.attribute("name").as_string();
        desc.networkStr = teamNode.attribute("net").as_string();
        desc.network = net;
        printf("  %u %s %s(%08X)\n", desc.number, desc.name.c_str(), desc.networkStr.c_str(), desc.network);
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


int main()
{
    srand(time(nullptr));

    if(!LoadGamesDatabase() || !LoadTeamsDatabase())
        return -1;

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8000);

    std::thread updateThread(UpdateThread);

    while (1)
        sleep(1);

    server.Stop();
    return 0;
}
