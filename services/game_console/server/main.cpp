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
#include "user.h"

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
    uint32_t m_authKey;

protected:
    virtual void FinalizeRequest();
};


struct UsersStorageRecord
{
    IPAddr ip;
    AuthKey authKey;
    char userName[kMaxUserNameLen];
    char password[kMaxPasswordLen];
};
static const char* kUsersStorageFileName = "data/users.dat";

std::map<uint32_t, GameDesc> GGamesDatabase;
std::unordered_map<NetworkAddr, Team> GTeams;
std::mutex GUsersGuard;
std::unordered_map<std::string, User*> GUsers;
std::unordered_map<AuthKey, User*> GAuthUsers;


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


static void DumpUsersStorage()
{
    FILE* f = fopen(kUsersStorageFileName, "w");
    if(!f)
    {
        printf("Failed to open users storage\n");
        return;
    }

    for(auto iter : GUsers)
    {
        UsersStorageRecord record;
        User* u = iter.second;
        memset(&record, 0, sizeof(record));
        record.authKey = u->GetAuthKey();
        record.ip = u->GetIPAddr();
        auto& name = u->GetName();
        auto& password = u->GetPassword();
        memcpy(record.userName, name.c_str(), name.length());
        memcpy(record.password, password.c_str(), password.length());
        fwrite(&record, sizeof(record), 1, f);
    }

    fclose(f);
}


static bool AddUser(const std::string& name, const std::string& password, Team* team)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    auto iter = GUsers.find(name);
    if(iter != GUsers.end())
        return false;

    User* user = new User(name, password, team);
    GUsers[name] = user;

    return true;
}


static User* GetUser(const std::string& name)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto iter = GUsers.find(name);
    if(iter == GUsers.end())
        return nullptr;
    return iter->second;
}


static AuthKey AuthorizeUser(const std::string& name, const std::string& password, IPAddr ipAddr)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto usersIter = GUsers.find(name);
    if(usersIter == GUsers.end())
        return kInvalidAuthKey;

    User* user = usersIter->second;    

    if(user->GetPassword() != password)
        return kInvalidAuthKey;

    uint32_t authKey = user->GetAuthKey();
    auto authUsersIter = GAuthUsers.find(authKey);
    if(authUsersIter != GAuthUsers.end())
    {
        printf("  reauth detected, prev auth key: %x\n", authKey);
        User* user2 = authUsersIter->second;
        if(user != user2)
        {
            printf("  CRITICAL ERROR, mismatched user data '%s':%u '%s':%u\n", user->GetName().c_str(), authKey, user2->GetName().c_str(), user2->GetAuthKey());
            exit(1);
        }
        GAuthUsers.erase(authUsersIter);
    }

    authKey = user->GenerateAuthKey();
    const uint32_t kMaxTriesCount = 5;
    uint32_t triesCount = 0;
    while(GAuthUsers.find(authKey) != GAuthUsers.end())
    {
        if(triesCount >= kMaxTriesCount)
        {
            printf("  CRITICAL ERROR, dublicate user was found, auth key: %x\n", authKey);
            exit(1);
        }

        authKey = user->GenerateAuthKey();
        triesCount++;
    }

    printf("  auth key: %x\n", authKey);
    GAuthUsers[authKey] = user;
    user->SetIPAddr(ipAddr);

    DumpUsersStorage();

    return authKey;
}


static void ChangeUserPassword(User* user, const std::string& newPassword)
{
    uint32_t authKey = user->GetAuthKey();
    GAuthUsers.erase(authKey);

    user->ChangePassword(newPassword);

    DumpUsersStorage();
}


static bool ChangeUserPassword(const std::string& userName, const std::string& newPassword)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    auto iter = GUsers.find(userName);
    if(iter == GUsers.end())
        return false;
    
    User* user = iter->second;
    ChangeUserPassword(user, newPassword);

    return true;
}


static HttpResponse ChangeUserPassword(AuthKey authKey, const std::string& newPassword, Team* team)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    if(authKey == kInvalidAuthKey)
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);;

    auto iter = GAuthUsers.find(authKey);
    if(iter == GAuthUsers.end())
        return HttpResponse(MHD_HTTP_UNAUTHORIZED);;
    
    User* user = iter->second;

    if(user->GetTeam() != team)
    {
        printf("  Team '%s' tries to change password for '%s' from '%s' team\n", team->desc.name.c_str(), user->GetName().c_str(), user->GetTeam()->desc.name.c_str());
        return HttpResponse(MHD_HTTP_FORBIDDEN);
    }

    ChangeUserPassword(user, newPassword);

    return HttpResponse(MHD_HTTP_OK);
}


static User* GetUser(AuthKey authKey)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto iter = GAuthUsers.find(authKey);
    if(iter == GAuthUsers.end())
        return nullptr;
    return iter->second;
}


static AuthKey GetAuthKeyFromQueryString(const QueryString& queryString)
{
    static const std::string kAuth("auth");
    AuthKey authKey = ~0u;
    if(!FindInMap(queryString, kAuth, authKey, 16))
    {
        printf("  ERROR: Bad request\n");
        return kInvalidAuthKey;
    }

    return authKey;
}


static User* CheckAuthority(const QueryString& queryString)
{
    AuthKey authKey = GetAuthKeyFromQueryString(queryString);

    printf("  auth key: %x\n", authKey);

    if(authKey == kInvalidAuthKey)
        return nullptr;

    auto user = GetUser(authKey);
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
    sprintf(response.content,
            "{ \"errorCode\": %d, \"data\": \"%s\" }", errorCode, data);
    response.contentLength = strlen(response.content);
    return response;
}


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    static const std::string kU("u");
    static const std::string kP("p");

    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if (ParseUrl(request.url, 1, ""))
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
    else if (ParseUrl(request.url, 1, "register"))
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

        if(userName.length() > kMaxUserNameLen || password.length() > kMaxPasswordLen)
        {
            printf("  too long\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        if(!AddUser(userName, password, team))
            return HttpResponse(MHD_HTTP_ALREADY_REPORTED);

        printf("  new user\n");

        return HttpResponse(MHD_HTTP_OK);
    }
    else if (ParseUrl(request.url, 1, "auth"))
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

        AuthKey authKey = AuthorizeUser(userName, password, request.clientIp.s_addr);
        if(authKey == kInvalidAuthKey)
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
    else if (ParseUrl(request.url, 1, "change_password"))
    {
        auto team = FindTeam(request.clientIp);
        if(!team)
            return HttpResponse(MHD_HTTP_FORBIDDEN);

        AuthKey authKey = GetAuthKeyFromQueryString(request.queryString);

        std::string password;
        if(!FindInMap(request.queryString, kP, password))
            return HttpResponse(MHD_HTTP_BAD_REQUEST);

        printf("  auth key: %x\n", authKey);
        printf("  new password: %s\n", password.c_str());

        return ChangeUserPassword(authKey, password, team);
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
        auto user = CheckAuthority(request.queryString);
        if(!user)
            return HttpResponse(MHD_HTTP_UNAUTHORIZED);

        Notification* n = nullptr;
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            n = user->GetNotification();
        }

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
        {
            printf(" ERROR: Forbidden\n");
            return HttpResponse(MHD_HTTP_FORBIDDEN);
        }

        std::string data;
        char buf[512];
        sprintf(buf, "Current time: %f\n\n", GetTime());
        data.append(buf);

        {
            for(auto& teamIter : GTeams)
            {
                auto& team = teamIter.second;
                IPAddr hwConsoleIp = GetHwConsoleIp(team.desc.network);

                team.DumpStats(data, hwConsoleIp);

                std::lock_guard<std::mutex> guard(GUsersGuard);
                for(auto& userIter : GUsers)
                {
                    auto user = userIter.second;
                    if(user->GetTeam() != &team)
                        continue;
                    
                    sprintf(buf, "  User '%s':\n", user->GetName().c_str());
                    data.append(buf);

                    user->DumpStats(data, hwConsoleIp);
                }
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
        printf("  team:    %s\n", team->desc.name.c_str());

        const char* flag = team->GetFlag(flagId);
        if(!flag)
            return BuildChecksystemResponse(kCheckerCheckerError, "Failed to found flag");
        printf("  flag:    %s\n", flag);

        return BuildChecksystemResponse(kCheckerOk, flag);
    }
    else if(ParseUrl(request.url, 1, "checksystem_check"))
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
        printf("  team:    %s\n", team->desc.name.c_str());

        User* hwConsoleUser = GetUser(team->desc.name);
        if(!hwConsoleUser)
            return BuildChecksystemResponse(kCheckerMumble, "HW Console user is not registered");

        IPAddr hwConsoleAddr = GetHwConsoleIp(team->desc.network);
        if(hwConsoleAddr == ~0u)
            return BuildChecksystemResponse(kCheckerDown, "There is no hardware console");

        if(hwConsoleUser->GetIPAddr() != hwConsoleAddr)
            return BuildChecksystemResponse(kCheckerMumble, "HW Console user is used not on the hw console");

        if(hwConsoleUser->GetNotificationsInQueue() >= User::kNotificationQueueSize)
            return BuildChecksystemResponse(kCheckerMumble, "Notifications queue is overflowed");

        if(!Check(team->desc.network))
            return BuildChecksystemResponse(kCheckerMumble, "Check failed");

        return BuildChecksystemResponse(kCheckerOk, "OK");
    }
    else if (ParseUrl(request.url, 1, "checksystem_change_password"))
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

        if(!ChangeUserPassword(userName, password))
        {
            printf("  Password change failed\n");
            return HttpResponse(MHD_HTTP_BAD_REQUEST);
        }

        printf("  Password changed\n");

        return HttpResponse(MHD_HTTP_OK);
    }
    else if(ParseUrl(request.url, 1, "checksystem_notification")) 
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
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            for(auto& iter : GUsers)
            {
                User* user = iter.second;
                user->AddNotification(n);
            }
        }
        n->Release();

        return HttpResponse(MHD_HTTP_OK);
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
        printf("  team:    %s\n", team->desc.name.c_str());

        User* hwConsoleUser = GetUser(team->desc.name);
        if(!hwConsoleUser)
            return BuildChecksystemResponse(kCheckerMumble, "HW Console user is not registered");

        team->PutFlag(flagId, flag);

        char message[512];
        sprintf(message, "Here is your flag, keep it safe\n%s", flag);
        Notification* n = new Notification("Hackerdom", message);
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            hwConsoleUser->AddNotification(n);
        }

        return BuildChecksystemResponse(kCheckerOk, "OK");
    }

    return HttpResponse(MHD_HTTP_NOT_FOUND);
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
    auto& teamDesc = team->desc;

    User* sourceUser = GetUser(m_authKey);

    printf("  notification from user '%s' team %u '%s'\n", sourceUser->GetName().c_str(), teamDesc.number, teamDesc.name.c_str());

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
    {
        std::lock_guard<std::mutex> guard(GUsersGuard);
        for(auto& iter : GUsers)
        {
            User* user = iter.second;
            if(user == sourceUser)
                continue;
            user->AddNotification(notification);
        }
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
        if(desc.name.length() > kMaxUserNameLen)
        {
            printf("  ERROR: too long user name: %s\n", desc.name.c_str());
            exit(1);
        }
		desc.network = net;
        printf("  %u %s\n", desc.number, desc.name.c_str());
        printf("    network: %s(%08X)\n", netStr, net);
        team.LoadDb();
    }

    return true;
}


void ReadUsersStorage()
{
    const uint32_t kRecordSize = sizeof(UsersStorageRecord);

    FILE* storage = fopen(kUsersStorageFileName, "r");
	if(storage)
	{
		fseek(storage, 0, SEEK_END);
		size_t fileSize = ftell(storage);
		fseek(storage, 0, SEEK_SET);

        bool error = false;
        if((fileSize % kRecordSize) == 0)
        {
            uint32_t recordsNum = fileSize / kRecordSize;
            for(uint32_t i = 0; i < recordsNum; i++)
            {
                UsersStorageRecord record;
				if(fread(&record, kRecordSize, 1, storage) != 1)
                {
                    error = true;
                    printf("Failed to read consoles storage\n");
                    break;
                }

                in_addr addr;
                addr.s_addr = record.ip;
                Team* team = FindTeam(addr);
                if(!team)
                    continue;

                User* user = new User(record.userName, record.password, team);
                GUsers[record.userName] = user;
                user->SetAuthKey(record.authKey);
                user->SetIPAddr(record.ip);
                GAuthUsers[record.authKey] = user;
            }
        }
        else
        {
            printf("Consoles storage is corrupted\n");
        }

        if(!error)
            printf("Consoles storage has been read succefully\n");

        fclose(storage);
    }
}


void UpdateThread()
{
    while(1)
    {
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            for(auto& iter : GUsers)
            {
                User* user = iter.second;
                user->Update();
            }
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

        printf("NOTIFY: Accepted connection from: %s\n", inet_ntoa(clientAddr.sin_addr));

        Team* team = FindTeam(clientAddr.sin_addr);
        if(!team)
        {
            printf("NOTIFY: unknown network\n");
            close(newSocket);
            continue;
        }

        std::lock_guard<std::mutex> guard(GUsersGuard);
        IPAddr ipAddr = clientAddr.sin_addr.s_addr;
        User* user = nullptr;
        for(auto& iter : GUsers)
        {
            if(iter.second->GetIPAddr() == ipAddr)
            {
                user = iter.second;
                break;
            }
        }

        if(!user)
        {
            printf("NOTIFY: Unknown ip address\n");
            close(newSocket);
            continue;
        }

        printf("NOTIFY: OK\n");
        user->SetNotifySocket(newSocket);
    }
}


int main()
{
    srand(time(nullptr));

    if(!LoadGamesDatabase() || !LoadTeamsDatabase())
        return -1;

    ReadUsersStorage();

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
