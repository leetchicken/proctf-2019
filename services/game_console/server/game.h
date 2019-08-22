#pragma once
#include <vector>
#include <string>
#include "misc.h"
#include "png.h"


struct GameDesc
{
    uint32_t id;
    std::string name;
    std::string desc;
    std::vector<std::string> assets;
};


struct Asset
{
    Image image;
    std::string name;
};


enum EGameErrorCode
{
    kGameErrorOk = 0,
    kGameErrorNotFound,
    kGameErrorInternal,
};


bool GamesStart();
void GetGamesList(std::vector<GameDesc>& list);
EGameErrorCode GetGameIcon(uint32_t gameId, Image& icon);
EGameErrorCode GetGameAsset(uint32_t gameId, uint32_t assetIndex, Asset& asset);
EGameErrorCode GetGameCode(uint32_t gameId, void** code, uint32_t& codeSize);