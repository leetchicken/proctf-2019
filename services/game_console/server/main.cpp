#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "httpserver.h"
#include "png.h"
#include <pugixml.hpp>
#include <map>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;

struct GameDesc
{
    std::string name;
    std::string desc;
};

struct TeamDesc
{
    std::string name;
    std::string networkStr;
    uint32_t network;
};

std::map<uint32_t, GameDesc> GGamesDatabase;
std::map<uint32_t, TeamDesc> GTeamsDatabase;


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

protected:
    virtual void FinalizeRequest();
};


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if (ParseUrl(request.url, 1, "list"))
    {
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

    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


HttpResponse RequestHandler::HandlePost(HttpRequest request, HttpPostProcessor** postProcessor)
{
    printf("  IP: %s\n", inet_ntoa(request.clientIp));
    if(ParseUrl(request.url, 1, "notification")) 
    {
		*postProcessor = new NotificationProcessor(request);
        return HttpResponse();
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
        uint32_t number = teamNode.attribute("number").as_uint();
        TeamDesc desc;
        desc.name = teamNode.attribute("name").as_string();
        desc.networkStr = teamNode.attribute("net").as_string();
        inet_aton(desc.networkStr.c_str(), (in_addr*)&desc.network);
        GTeamsDatabase.insert({number, desc});
        printf("  %u %s %s(%08X)\n", number, desc.name.c_str(), desc.networkStr.c_str(), desc.network);
    }

    return true;
}


int main()
{
    if(!LoadGamesDatabase() || !LoadTeamsDatabase())
        return -1;

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8000);

    while (1)
        sleep(1);

    server.Stop();
    return 0;
}
