#include "game.h"
#include <string.h>
#include <pugixml.hpp>
#include <dirent.h>
#include <map>
#include <mutex>


std::mutex GMutex;
std::map<uint32_t, GameDesc*> GGamesDatabase;


static bool EnumerateAssets(const char* gameName, std::vector<std::string>& assets)
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


static bool LoadGamesDatabase()
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
        GameDesc* desc = new GameDesc();
        desc->id = gameNode.attribute("id").as_uint();
        desc->name = gameNode.attribute("name").as_string();
        desc->desc = gameNode.attribute("desc").as_string();
        if(!EnumerateAssets(desc->name.c_str(), desc->assets))
        {
            delete desc;
            return false;
        }
        GGamesDatabase.insert({desc->id, desc});
        printf("  %x %s '%s'\n", desc->id, desc->name.c_str(), desc->desc.c_str());
        printf("  assets:\n");
        if(desc->assets.empty())
            printf("   <none>\n");
        for(auto& assetName : desc->assets)
            printf("   %s\n", assetName.c_str());
    }

    return true;
}


static GameDesc* FindGame(uint32_t id)
{
    std::lock_guard<std::mutex> guard(GMutex);
    auto iter = GGamesDatabase.find(id);
    if(iter == GGamesDatabase.end())
    {
        printf("  unknown game\n");
        return nullptr;
    }
    return iter->second;
}


bool GamesStart()
{
    if(!LoadGamesDatabase())
        return false;

    return true;
}


void GetGamesList(std::vector<GameDesc*>& list)
{
    std::lock_guard<std::mutex> guard(GMutex);
    list.reserve(GGamesDatabase.size());
    for(auto& gameIter : GGamesDatabase)
        list.push_back(gameIter.second);
}


EGameErrorCode GetGameIcon(uint32_t gameId, Image& icon)
{
    GameDesc* gameDesc = FindGame(gameId);
    if(!gameDesc)
        return kGameErrorNotFound;

    std::string iconFilePath = "data/";
    iconFilePath.append(gameDesc->name);
    iconFilePath.append("/icon.png");
    printf("  %s\n", iconFilePath.c_str());
    if(!read_png(iconFilePath.c_str(), icon))
    {
        printf("  failed to read icon\n");
        return kGameErrorInternal;
    }

    if(icon.width != kIconWidth || icon.height != kIconHeight)
    {
        printf("  invalid icon size: %ux%u\n", icon.width, icon.height);
        return kGameErrorInternal;
    }

    return kGameErrorOk;
}


EGameErrorCode GetGameAsset(uint32_t gameId, uint32_t assetIndex, Asset& asset)
{
    GameDesc* gameDesc = FindGame(gameId);
    if(!gameDesc)
        return kGameErrorNotFound;

    if(assetIndex >= gameDesc->assets.size())
    {
        printf("  invalid asset index\n");
        return kGameErrorNotFound;
    }

    asset.name = gameDesc->assets[assetIndex];
    std::string assetFilePath = "data/";
    assetFilePath.append(gameDesc->name);
    assetFilePath.append("/");
    assetFilePath.append(asset.name.c_str());
    printf("  %s\n", assetFilePath.c_str());
    if(!read_png(assetFilePath.c_str(), asset.image))
    {
        printf("  failed to read asset\n");
        return kGameErrorInternal;
    }

    char* extension = asset.name.data() + (asset.name.length() - 3);
    strcpy(extension, "bmp");

    return kGameErrorOk;
}


EGameErrorCode GetGameCode(uint32_t gameId, void** code, uint32_t& codeSize)
{
    GameDesc* gameDesc = FindGame(gameId);
    if(!gameDesc)
        return kGameErrorNotFound;

    std::string codeFilePath = "data/";
    codeFilePath.append(gameDesc->name);
    codeFilePath.append("/code.bin");
    printf("  %s\n", codeFilePath.c_str());
    FILE* f = fopen(codeFilePath.c_str(), "r");
    if (!f)
    {
        printf( "failed to read code\n");
        return kGameErrorInternal;
    }

    fseek(f, 0, SEEK_END);
    codeSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    *code = malloc(codeSize);
	fread(*code, 1, codeSize, f);
    fclose(f);

    return kGameErrorOk;
}


static uint32_t GenerateId()
{
    uint32_t id = 0;
    for(uint32_t i = 0; i < 4; i++)
        id |= (rand() % 255) << (i * 8);
    return id;
}


bool AddGame(void* zipFile, uint32_t zipFileSize, const char* zipName)
{
    char buf[512];
    sprintf(buf, "data/%s", zipName);
    FILE* f = fopen(buf, "w");
    fwrite(zipFile, 1, zipFileSize, f);
    fclose(f);

    char gameName[256];
    memset(gameName, 0, sizeof(gameName));
    strncpy(gameName, zipName, strlen(zipName) - 4);
    sprintf(buf, "cd data; mkdir %s; mv %s.zip %s/; cd %s; unzip -o %s.zip", gameName, gameName, gameName, gameName, gameName);
    system(buf);

    uint32_t id;
    while(1)
    {
        id = GenerateId();
        auto iter = GGamesDatabase.find(id);
        if(iter == GGamesDatabase.end())
            break;
    }

    GameDesc* desc = new GameDesc();
    desc->id = id;
    desc->name = gameName;
    desc->desc = gameName;
    if(!EnumerateAssets(desc->name.c_str(), desc->assets))
    {
        delete desc;
        return false;
    }

    pugi::xml_document doc;
    if (!doc.load_file("data/games.xml")) 
    {
        printf("Failed to load games database\n");
        delete desc;
        return false;
    }

    pugi::xml_node gamesNode = doc.child("Games");
    pugi::xml_node gameNode = gamesNode.append_child("Game");
    gameNode.append_attribute("name") = gameName;
    gameNode.append_attribute("desc") = gameName;
    gameNode.append_attribute("id").set_value(id);
    doc.save_file("data/games.xml");

    std::lock_guard<std::mutex> guard(GMutex);
    GGamesDatabase.insert({desc->id, desc});

    printf("  new game '%s' added\n", gameName);

    return true;
}