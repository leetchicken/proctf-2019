#include <string.h>
#include <arpa/inet.h>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include "httpserver.h"
#include "checksystem.h"
#include "misc.h"
#include "notification.h"
#include "user.h"
#include "game.h"


static const std::string kU("u");
static const std::string kP("p");
static const std::string kAuth("auth");


class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
    HttpResponse GetMainPage(HttpRequest request);
    HttpResponse GetRegister(HttpRequest request);
    HttpResponse GetAuth(HttpRequest request);
    HttpResponse GetChangePassword(HttpRequest request);
    HttpResponse GetGamesList(HttpRequest request);
    HttpResponse GetGameIcon(HttpRequest request);
    HttpResponse GetGameAsset(HttpRequest request);
    HttpResponse GetGameCode(HttpRequest request);
    HttpResponse GetNotification(HttpRequest request);
    HttpResponse GetChecksystemStatus(HttpRequest request);
    HttpResponse GetChecksystemGetFlag(HttpRequest request);
    HttpResponse GetChecksystemCheck(HttpRequest request);
    HttpResponse GetChecksystemChangePassword(HttpRequest request);
    HttpResponse GetChecksystemNotification(HttpRequest request);

    HttpResponse PostNotification(HttpRequest request, HttpPostProcessor** postProcessor);
    HttpResponse PostChecksystemFlag(HttpRequest request, HttpPostProcessor** postProcessor);
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
    uint32_t m_authKey;

protected:
    virtual void FinalizeRequest();
};


static User* CheckAuthority(const QueryString& queryString)
{
    AuthKey authKey = ~0u;
    if(!FindInMap(queryString, kAuth, authKey, 16))
    {
        printf("  ERROR: Bad request\n");
        return nullptr;
    }

    printf("  auth key: %x\n", authKey);

    if(authKey == kInvalidAuthKey)
        return nullptr;

    auto user = User::Get(authKey);
    if(!user)
    {
        printf("  ERROR: Unauthorized access, auth key: %x\n", authKey);
        return nullptr;
    }

    return user;
}


static HttpResponse BuildChecksystemResponse(int errorCode, const char* data)
{
    printf("  Response: %d %s\n", errorCode, data);

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/json"});
    response.content = (char*)malloc(512);
    memset(response.content, 0, 512);
    sprintf(response.content, "{ \"errorCode\": %d, \"data\": \"%s\" }", errorCode, data);
    response.contentLength = strlen(response.content);
    return response;
}


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if (ParseUrl(request.url, 1, ""))
        return GetMainPage(request);
    else if (ParseUrl(request.url, 1, "register"))
        return GetRegister(request);
    else if (ParseUrl(request.url, 1, "auth"))
        return GetAuth(request);
    else if (ParseUrl(request.url, 1, "change_password"))
        return GetChangePassword(request);
    else if (ParseUrl(request.url, 1, "list"))
        return GetGamesList(request);
    else if(ParseUrl(request.url, 1, "icon"))
        return GetGameIcon(request);
    else if(ParseUrl(request.url, 1, "asset"))
        return GetGameAsset(request);
    else if(ParseUrl(request.url, 1, "code"))
        return GetGameCode(request);
    else if(ParseUrl(request.url, 1, "notification"))
        return GetNotification(request);
    else if(ParseUrl(request.url, 1, "checksystem_status"))
        return GetChecksystemStatus(request);
    else if(ParseUrl(request.url, 1, "checksystem_getflag"))
        return GetChecksystemGetFlag(request);
    else if(ParseUrl(request.url, 1, "checksystem_check"))
        return GetChecksystemCheck(request);
    else if (ParseUrl(request.url, 1, "checksystem_change_password"))
        return GetChecksystemChangePassword(request);
    else if(ParseUrl(request.url, 1, "checksystem_notification"))
        return GetChecksystemNotification(request);

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if(ParseUrl(request.url, 1, "notification"))
        return PostNotification(request, postProcessor);
    else if(ParseUrl(request.url, 1, "checksystem_putflag")) 
        return PostChecksystemFlag(request, postProcessor);

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::GetMainPage(HttpRequest request)
{
    const char* fileName = "";
    auto team = FindTeam(request.clientIp, false);
    if(team)
        fileName = "data/SDK.md.html";
    else
        fileName = "data/jury.html";            

    FILE* f = fopen(fileName, "r");
    if (!f)
    {
        printf( "failed to read '%s'\n", fileName);
        return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);
    }

    fseek(f, 0, SEEK_END);
    size_t fileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* fileData = (char*)malloc(fileSize);
    fread(fileData, 1, fileSize, f);
    fclose(f);

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "text/html"});
    response.content = fileData;
    response.contentLength = fileSize;
    return response;
}


HttpResponse RequestHandler::GetRegister(HttpRequest request)
{
    auto team = FindTeam(request.clientIp);
    if(!team)
    {
        printf(" ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    std::string userName, password;
    if(!FindInMap(request.queryString, kU, userName))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    if(!FindInMap(request.queryString, kP, password))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  user name: %s\n", userName.c_str());
    printf("  password:  %s\n", password.c_str());

    auto ret = User::Add(userName, password, team);

    if(ret == kUserErrorTooLong)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    else if(ret == kUserErrorAlreadyExists)
        return HttpResponse(MHD_HTTP_ALREADY_REPORTED);

    return HttpResponse(MHD_HTTP_OK);
}


HttpResponse RequestHandler::GetAuth(HttpRequest request)
{
    auto team = FindTeam(request.clientIp);
    if(!team)
        return HttpResponse(MHD_HTTP_FORBIDDEN);

    std::string userName, password;
    if(!FindInMap(request.queryString, kU, userName))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    if(!FindInMap(request.queryString, kP, password))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  user name: %s\n", userName.c_str());
    printf("  password:  %s\n", password.c_str());

    AuthKey authKey;
    auto ret = User::Authorize(userName, password, request.clientIp.s_addr, authKey);
    if(ret == kUserErrorInvalidCredentials)
    {
        printf("  invalid credentials\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    struct
    {
        uint32_t authKey;
    } responseData;
    responseData.authKey = authKey;

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/octet-stream"});
    response.content = (char*)malloc(sizeof(responseData));
    response.contentLength = sizeof(responseData);
    memcpy(response.content, &responseData, response.contentLength);
    return response;
}


HttpResponse RequestHandler::GetChangePassword(HttpRequest request)
{
    auto team = FindTeam(request.clientIp);
    if(!team)
        return HttpResponse(MHD_HTTP_FORBIDDEN);

    AuthKey authKey = kInvalidAuthKey;
    if(!FindInMap(request.queryString, kAuth, authKey, 16))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    std::string password;
    if(!FindInMap(request.queryString, kP, password))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);

    printf("  auth key: %x\n", authKey);
    printf("  new password: %s\n", password.c_str());

    auto ret = User::ChangePassword(authKey, password, team);
    if(ret == kUserErrorInvalidAuthKey)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    else if(ret == kUserErrorUnauthorized)
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);
    else if(ret == kUserErrorForbidden)
        return HttpResponse(MHD_HTTP_FORBIDDEN);

    return HttpResponse(MHD_HTTP_OK);
}


HttpResponse RequestHandler::GetGamesList(HttpRequest request)
{
    if(!CheckAuthority(request.queryString))
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);

    std::vector<GameDesc> gamesList;
    ::GetGamesList(gamesList);

    const uint32_t gamesNum = gamesList.size();
    const uint32_t kResponseCapacity = 1024;
    uint8_t* buf = (uint8_t*)malloc(kResponseCapacity);
    memset(buf, 0, kResponseCapacity);
    uint8_t* ptr = buf;
    memcpy(ptr, (void*)&gamesNum, 4);
    ptr += 4;
    for(auto& gameDesc : gamesList)
    {
        memcpy(ptr, (void*)&gameDesc.id, 4);
        ptr += 4;
        uint32_t assetsNum = gameDesc.assets.size(); 
        memcpy(ptr, (void*)&assetsNum, 4);
        ptr += 4;
        uint32_t len = gameDesc.name.length() + 1;
        memcpy(ptr, gameDesc.name.c_str(), len);
        ptr += len;
    }
    
    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/octet-stream"});
    response.content = (char*)buf;
    response.contentLength = ptr - buf;
    return response;
}


HttpResponse RequestHandler::GetGameIcon(HttpRequest request)
{
    if(!CheckAuthority(request.queryString))
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);

    static const std::string kId("id");
    uint32_t id = ~0u;
    if(!FindInMap(request.queryString, kId, id, 16))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  id: %x\n", id);

    Image icon;
    auto ret = ::GetGameIcon(id, icon);
    if(ret == kGameErrorNotFound)
        return HttpResponse(MHD_HTTP_NOT_FOUND);
    else if(ret == kGameErrorInternal)
        return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/octet-stream"});
    response.contentLength = kIconWidth * kIconHeight * 4;
    response.content = (char*)malloc(response.contentLength);
    ABGR_to_ARGB(icon.abgr, (ARGB*)response.content, icon.width, icon.height);
    return response;
}


HttpResponse RequestHandler::GetGameAsset(HttpRequest request)
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

    Asset asset;
    auto ret = ::GetGameAsset(id, index, asset);
    if(ret == kGameErrorNotFound)
        return HttpResponse(MHD_HTTP_NOT_FOUND);
    else if(ret == kGameErrorInternal)
        return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/octet-stream"});
    response.contentLength = asset.name.length() + 1 + asset.image.width * asset.image.height * 4;
    response.content = (char*)malloc(response.contentLength);
    memset(response.content, 0, response.contentLength);

    char* ptr = response.content;
    memcpy(ptr, asset.name.c_str(), asset.name.length());
    ptr += asset.name.length() + 1;
    ABGR_to_ARGB(asset.image.abgr, (ARGB*)ptr, asset.image.width, asset.image.height);

    return response;
}


HttpResponse RequestHandler::GetGameCode(HttpRequest request)
{
    if(!CheckAuthority(request.queryString))
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);

    static const std::string kId("id");
    uint32_t id = ~0u;
    if(!FindInMap(request.queryString, kId, id, 16))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  id: %x\n", id);

    void* code = nullptr;
    uint32_t codeSize = 0;
    auto ret = ::GetGameCode(id, &code, codeSize);
    if(ret == kGameErrorNotFound)
        return HttpResponse(MHD_HTTP_NOT_FOUND);
    else if(ret == kGameErrorInternal)
        return HttpResponse(MHD_HTTP_INTERNAL_SERVER_ERROR);

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "application/octet-stream"});
    response.content = (char*)code;
    response.contentLength = codeSize;
    return response;
}


HttpResponse RequestHandler::GetNotification(HttpRequest request)
{
    auto user = CheckAuthority(request.queryString);
    if(!user)
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);

    Notification* n = user->GetNotification();
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


HttpResponse RequestHandler::GetChecksystemStatus(HttpRequest request)
{
    auto team = FindTeam(request.clientIp, false);
    if(team)
    {
        printf(" ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    std::string data;
    char buf[512];
    sprintf(buf, "Current time: %f\n\n", GetTime());
    data.append(buf);

    std::vector<Team*> teams;
    GetTeams(teams);
    std::vector<User*> users;
    User::GetUsers(users);
    for(auto team : teams)
    {
        IPAddr hwConsoleIp = GetHwConsoleIp(team->network);

        team->DumpStats(data, hwConsoleIp);

        for(auto user : users)
        {
            if(user->GetTeam() != team)
                continue;
            
            sprintf(buf, "  User '%s':\n", user->GetName().c_str());
            data.append(buf);

            user->DumpStats(data, hwConsoleIp);
        }
    }

    HttpResponse response;
    response.code = MHD_HTTP_OK;
    response.headers.insert({"Content-Type", "text/plain"});
    response.content = (char*)malloc(data.size());
    memcpy(response.content, data.c_str(), data.size());
    response.contentLength = data.size();
    return response;
}


HttpResponse RequestHandler::GetChecksystemGetFlag(HttpRequest request)
{
    auto team = FindTeam(request.clientIp, false);
    if(team)
    {
        printf("  ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    static std::string kAddr("addr");
    static std::string kFlagId("id");

    const char* addrStr = FindInMap(request.queryString, kAddr);
    if(!addrStr)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  addr:    %s\n", addrStr);

    const char* flagId = FindInMap(request.queryString, kFlagId);
    if(!flagId)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  flag id: %s\n", flagId);

    in_addr addr;
    inet_aton(addrStr, (in_addr*)&addr);
    team = FindTeam(addr);
    if(!team)
        return BuildChecksystemResponse(kCheckerCheckerError, "Failed to found team by IP");
    printf("  team:    %s\n", team->name.c_str());

    const char* flag = team->GetFlag(flagId);
    if(!flag)
        return BuildChecksystemResponse(kCheckerCheckerError, "Failed to found flag");
    printf("  flag:    %s\n", flag);

    return BuildChecksystemResponse(kCheckerOk, flag);
}


HttpResponse RequestHandler::GetChecksystemCheck(HttpRequest request)
{
    auto team = FindTeam(request.clientIp, false);
    if(team)
    {
        printf("  ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    static std::string kAddr("addr");

    const char* addrStr = FindInMap(request.queryString, kAddr);
    if(!addrStr)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  addr:    %s\n", addrStr);

    in_addr addr;
    inet_aton(addrStr, (in_addr*)&addr);
    team = FindTeam(addr);
    if(!team)
        return BuildChecksystemResponse(kCheckerCheckerError, "Failed to found team by IP");
    printf("  team:    %s\n", team->name.c_str());

    User* hwConsoleUser = User::Get(team->name);
    if(!hwConsoleUser)
        return BuildChecksystemResponse(kCheckerMumble, "HW Console user is not registered");

    IPAddr hwConsoleAddr = GetHwConsoleIp(team->network);
    if(hwConsoleAddr == ~0u)
        return BuildChecksystemResponse(kCheckerDown, "There is no hardware console");

    if(hwConsoleUser->GetIPAddr() != hwConsoleAddr)
        return BuildChecksystemResponse(kCheckerMumble, "HW Console user is used not on the hw console");

    if(hwConsoleUser->GetNotificationsInQueue() >= User::kNotificationQueueSize)
        return BuildChecksystemResponse(kCheckerMumble, "Notifications queue is overflowed");

    if(!Check(team->network))
        return BuildChecksystemResponse(kCheckerMumble, "Check failed");

    return BuildChecksystemResponse(kCheckerOk, "OK");
}


HttpResponse RequestHandler::GetChecksystemChangePassword(HttpRequest request)
{
    auto team = FindTeam(request.clientIp, false);
    if(team)
    {
        printf(" ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    std::string userName, password;
    if(!FindInMap(request.queryString, kU, userName))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    if(!FindInMap(request.queryString, kP, password))
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  user name: %s\n", userName.c_str());
    printf("  password:  %s\n", password.c_str());

    if(User::ChangePassword(userName, password) == kUserErrorInvalidCredentials)
    {
        printf("  Password change failed, invalid user name\n");
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    }

    printf("  Password changed\n");

    return HttpResponse(MHD_HTTP_OK);
}


HttpResponse RequestHandler::GetChecksystemNotification(HttpRequest request)
{
    auto team = FindTeam(request.clientIp, false);
    if(team)
    {
        printf(" ERROR: Forbidden\n");
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    static std::string kMessage("message");

    const char* message = FindInMap(request.queryString, kMessage);
    if(!message)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);

    Notification* n = new Notification("Hackerdom", message);
    n->AddRef();
    User::BroadcastNotification(n, nullptr);
    n->Release();

    return HttpResponse(MHD_HTTP_OK);
}


HttpResponse RequestHandler::PostNotification(HttpRequest request, HttpPostProcessor** postProcessor)
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


HttpResponse RequestHandler::PostChecksystemFlag(HttpRequest request, HttpPostProcessor** postProcessor)
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
    printf("  addr:    %s\n", addrStr);

    const char* flagId = FindInMap(request.queryString, kFlagId);
    if(!flagId)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  flag id: %s\n", flagId);

    const char* flag = FindInMap(request.queryString, kFlag);
    if(!flag)
        return HttpResponse(MHD_HTTP_BAD_REQUEST);
    printf("  flag:    %s\n", flag);

    in_addr addr;
    inet_aton(addrStr, (in_addr*)&addr);
    team = FindTeam(addr);
    if(!team)
        return BuildChecksystemResponse(kCheckerCheckerError, "Failed to found team by IP");
    printf("  team:    %s\n", team->name.c_str());

    User* hwConsoleUser = User::Get(team->name);
    if(!hwConsoleUser)
        return BuildChecksystemResponse(kCheckerMumble, "HW Console user is not registered");

    team->PutFlag(flagId, flag);

    char message[512];
    sprintf(message, "Here is your flag, keep it safe\n%s", flag);
    Notification* n = new Notification("Hackerdom", message);
    hwConsoleUser->AddNotification(n);

    return BuildChecksystemResponse(kCheckerOk, "OK");
}


NotificationProcessor::NotificationProcessor(const HttpRequest& request)
    : HttpPostProcessor(request)
{
    static const std::string kContentLength("content-length");
    static const std::string kAuth("auth");
    FindInMap(request.headers, kContentLength, m_contentLength);
    FindInMap(request.queryString, kAuth, m_authKey, 16);

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

    User* sourceUser = User::Get(m_authKey);

    printf("  notification from user '%s' team %u '%s'\n", sourceUser->GetName().c_str(), team->number, team->name.c_str());

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

    Notification* n = new Notification(m_content, m_contentLength);
    n->AddRef();
    User::BroadcastNotification(n, sourceUser);
    n->Release();

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


int main()
{
    srand(time(nullptr));

    if(!GamesStart())
        return -1;

    if(!TeamsStart())
        return -1;

    User::Start();

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(kHttpPort);

    std::vector<NetworkAddr> teamsNet;
    std::vector<Team*> teams;
    GetTeams(teams);
    for(auto team : teams)
        teamsNet.push_back(team->network);
    InitChecksystem(teamsNet);

    while (1)
        sleep(1);

    server.Stop();
    return 0;
}
