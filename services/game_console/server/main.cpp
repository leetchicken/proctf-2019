#include <string.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "httpserver.h"
#include "png.h"
#include <pugixml.hpp>
#include <map>
#include <string>

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;

struct GameDesc
{
    std::string name;
    std::string desc;
};

std::map<uint32_t, GameDesc> GGamesDatabase;

class RequestHandler : public HttpRequestHandler
{
public:
	RequestHandler() = default;
	virtual ~RequestHandler() = default;

	HttpResponse HandleGet(HttpRequest request);
	HttpResponse HandlePost(HttpRequest request, HttpPostProcessor** postProcessor);

private:
};


HttpResponse RequestHandler::HandleGet(HttpRequest request)
{
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
        FindInMap(request.queryString, kId, id);
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
        FindInMap(request.queryString, kId, id);
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
    return HttpResponse(MHD_HTTP_NOT_FOUND);
}


bool LoadDatabase()
{
    pugi::xml_document doc;
    if (!doc.load_file("data/games.xml")) 
    {
        printf("Failed to load database\n");
        return false;
    }

    printf("Database:\n");
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


int main()
{
    if(!LoadDatabase())
        return -1;

    RequestHandler handler;
    HttpServer server(&handler);

    server.Start(8000);

    while (1)
        sleep(1);

    server.Stop();
    return 0;
}
