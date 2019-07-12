#include "../hw/api.h"

static const uint32_t kGameIconWidth = 172;
static const uint32_t kGameIconHeight = 172;
static const uint32_t kGameIconSize = kGameIconWidth * kGameIconHeight * 4;
static const uint32_t kBackgroundWidth = 480;
static const uint32_t kBackgroundHeight = 272;
static const uint32_t kInfoIconsWidth = 40;
static const uint32_t kInfoIconsHeight = 40;
static const uint32_t kRefreshButtonWidth = 153;
static const uint32_t kRefreshButtonHeight = 32;
static const uint32_t kMaxGameIconsOnScreen = 3;
static const uint32_t kGameIconCacheSize = kMaxGameIconsOnScreen + 1;
static const uint32_t kMaxGamesCount = 256;
static const uint32_t kMaxGameCodeSize = 1024;
static const char* kServerAddr = "192.168.1.1:8000";


struct IconsManager
{
    struct GameIcon
    {
        uint8_t* addr;
    };

    uint8_t* sdramStart;
    API* api;
    GameIcon gameIcons[kGameIconCacheSize];
    uint32_t freeGameIconIndices[kGameIconCacheSize];
    uint32_t freeGameIconIndicesNum;
    uint8_t* background;
    uint8_t* emptyGameIcon;
    uint8_t* loadingIcon;
    uint8_t* networkOffIcon;
    uint8_t* networkOnIcon;
    uint8_t* refreshButton;

    IconsManager(uint8_t* startAddr, API* api_)
    {
        sdramStart = startAddr;
        api = api_;
    }

    uint8_t* Reset()
    {
        uint8_t* curSdram = sdramStart;
        for(uint32_t i = 0; i < kGameIconCacheSize; i++)
        {
            gameIcons[i].addr = curSdram;
            freeGameIconIndices[i] = i;
            curSdram += kGameIconSize;
        }
        freeGameIconIndicesNum = kGameIconCacheSize;

        background = curSdram;
        uint32_t backgroundSize = kBackgroundWidth * kBackgroundHeight * 4;
        curSdram = ReadIcon("/fs/background.bmp", background, backgroundSize);

        emptyGameIcon = curSdram;
        curSdram = ReadIcon("/fs/empty_icon.bmp", emptyGameIcon, kGameIconSize);

        uint32_t infoIconSize = kInfoIconsWidth * kInfoIconsHeight * 4;

        loadingIcon = curSdram;
        curSdram = ReadIcon("/fs/loading.bmp", loadingIcon, infoIconSize);

        networkOffIcon = curSdram;
        curSdram = ReadIcon("/fs/network_off.bmp", networkOffIcon, infoIconSize);

        networkOnIcon = curSdram;
        curSdram = ReadIcon("/fs/network_on.bmp", networkOnIcon, infoIconSize);

        refreshButton = curSdram;
        uint32_t refreshButtonSize = kRefreshButtonWidth * kRefreshButtonHeight * 4;
        curSdram = ReadIcon("/fs/refresh.bmp", refreshButton, refreshButtonSize);

        return curSdram;
    }

    uint32_t AllocateGameIcon()
    {
        if(!freeGameIconIndicesNum)
            return ~0u;
        
        uint32_t ret = freeGameIconIndices[freeGameIconIndicesNum - 1];
        freeGameIconIndicesNum--;
        return ret;
    }

    void FreeGameIcon(uint32_t idx)
    {
        freeGameIconIndices[freeGameIconIndicesNum] = idx;
        freeGameIconIndicesNum++;
    }

    void ClearGameIcons()
    {
        for(uint32_t i = 0; i < kGameIconCacheSize; i++)
            freeGameIconIndices[i] = i;
        freeGameIconIndicesNum = kGameIconCacheSize;
    }

private:
    uint8_t* ReadIcon(const char* fileName, uint8_t* dst, uint32_t size)
    {
        void* f = api->fopen(fileName, "r");
        api->fread(dst, size, f);
        api->fclose(f);
        return dst + size;
    }
};


enum EIconState
{
    kIconInvalid = 0,
    kIconLoading,
    kIconValid,
    kIconValidEmpty,

    kIconStatesCount
};


struct GameDesc
{
    Rect uiRect;
    uint32_t id;
    char name[32];
    uint8_t* iconAddr;
    uint32_t iconIndex;
    EIconState iconState;
    HTTPRequest* iconRequest;

    GameDesc()
        : id(~0u)
    {
        uiRect.y = 60;
        uiRect.width = kGameIconWidth;
        uiRect.height = kGameIconHeight;
        name[0] = '\0';
        ResetIconState();
    }

    void ResetIconState()
    {
        iconAddr = NULL;
        iconIndex = ~0u;
        iconState = kIconInvalid;
        iconRequest = NULL;
    }
};


void FillRect(API* api, const Rect& screenRect, Rect rect, uint32_t color)
{
    rect.ClampByRect(screenRect);
    if(rect.Area())
        api->LCD_FillRect(rect, color);
}


void DrawIcon(API* api, const Rect& screenRect, const IconsManager& iconMan, const GameDesc& desc)
{
    Rect rect = desc.uiRect;
    rect.ClampByRect(screenRect);
    if(rect.Area())
    {
        uint8_t* iconAddr = iconMan.emptyGameIcon;
        if(desc.iconState == kIconValid)
            iconAddr = desc.iconAddr;

        uint32_t pitch = kGameIconWidth * 4;
        uint32_t offset = 0;
        if(desc.uiRect.x < 0)
            offset = -desc.uiRect.x * 4;
        api->LCD_DrawImageWithBlend(rect, iconAddr + offset, pitch);
    }
}


enum EMainScreenState
{
    kMainScreenReady = 0,
    kMainScreenWaitForNetwork,
    kMainScreenWaitGameList,
    kMainScreenLoadGameCode,

    kMainScreenStatesCount
};


HTTPRequest* RequestGamesList(API* api)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s/list", kServerAddr);
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


HTTPRequest* RequestIcon(API* api, uint32_t gameId, uint8_t* iconAddr)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s/icon?id=%x", kServerAddr, gameId);
    request->responseData = (void*)iconAddr;
    request->responseDataCapacity = kGameIconSize;
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


HTTPRequest* RequestGameCode(API* api, uint32_t gameId, uint8_t* codeAddr)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s/code?id=%x", kServerAddr, gameId);
    request->responseData = (void*)codeAddr;
    request->responseDataCapacity = kMaxGameCodeSize;
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


int GameMain(API* api)
{
    Rect screenRect;
    api->GetScreenRect(&screenRect);

    TouchScreenState tsState;

    Rect backgroundRect(0, 0, kBackgroundWidth, kBackgroundHeight);
    Rect networkRect(10, 10, kInfoIconsWidth, kInfoIconsHeight);
    Rect loadingRect(430, 10, kInfoIconsWidth, kInfoIconsHeight);
    Rect refreshRect(163, 238, kRefreshButtonWidth, kRefreshButtonHeight);

    HTTPRequest* request = NULL;
    EMainScreenState state = kMainScreenWaitForNetwork;

    uint8_t* curSdram = api->GetSDRam();
    IconsManager iconCache(curSdram, api);
    curSdram = iconCache.Reset();
    bool evictIcon = false;
    uint32_t iconRequestsInFlight = 0;

    GameDesc* games = (GameDesc*)curSdram;
    uint32_t gamesCount = 0;

    uint8_t* gameCodeMem = (uint8_t*)api->Malloc(kMaxGameCodeSize);

    bool touchOnPrevFrame = false;
    uint16_t prevTouchX = 0, prevTouchY = 0;
    float pressDownTime = 0.0f;

    while(1)
    {
        api->SwapFramebuffer();
        api->GetTouchScreenState(&tsState);

        uint32_t selectedGame = ~0u;
        bool updatePressed = false;

        bool pressDown = false, pressUp = false;
        // press down detection
        if(!touchOnPrevFrame && (tsState.touchDetected == 1))
        {
            pressDownTime = api->time();
            pressDown = true;
        }
        // press up detection
        if(touchOnPrevFrame && !tsState.touchDetected)
            pressUp = true;

        if(pressUp && api->time() - pressDownTime < 0.1f)
        {
            for(uint32_t g = 0; g < gamesCount; g++)
            {
                Rect& rect = games[g].uiRect;
                if(rect.IsPointInside(prevTouchX, prevTouchY))
                {
                    selectedGame = g;
                    break;
                }
            }

            if(selectedGame == ~0u && refreshRect.IsPointInside(prevTouchX, prevTouchY))
                updatePressed = true;
        }
        else if(touchOnPrevFrame && (tsState.touchDetected == 1))
        {
            int vecX = (int)tsState.touchX[0] - (int)prevTouchX;
            for(uint32_t g = 0; g < gamesCount; g++)
                games[g].uiRect.x += vecX;
        }

        touchOnPrevFrame = false;
        if(tsState.touchDetected == 1)
        {
            prevTouchX = tsState.touchX[0];
            prevTouchY = tsState.touchY[0];
            touchOnPrevFrame = true;
        }

        if(state == kMainScreenWaitForNetwork && api->GetNetwokConnectionStatus() == kNetwokConnectionStatusGlobalUp)
        {
            api->printf("Connected to the network: '%s'\n", api->GetIPAddress());
            request = RequestGamesList(api);
            if(request)
                state = kMainScreenWaitGameList;
            else
                state = kMainScreenReady;
        }

        if(state == kMainScreenWaitGameList && request->done)
        {
            if(request->succeed)
            {
                uint8_t* response = (uint8_t*)request->responseData;
                api->memcpy(&gamesCount, response, 4);
                response += 4;
                for(uint32_t g = 0; g < gamesCount; g++)
                {
                    GameDesc& desc = games[g];
                    desc = GameDesc();
                    api->memcpy(&desc.id, response, 4);
                    response += 4;
                    uint32_t strLen = api->strlen((char*)response);
                    api->memcpy(desc.name, response, strLen + 1);
                    response += strLen + 1;
                }

                for(uint32_t i = 0; i < gamesCount; i++)
                {
                    games[i].uiRect.x = i * (kGameIconWidth + 20) + 20;
                    games[i].ResetIconState();
                }

                state = kMainScreenReady;
                api->printf("Received games list\n");
            }
            else
            {
                api->printf("Error occured while loading games list\n");
                state = kMainScreenReady;
            }            

            api->FreeHTTPRequest(request);
            request = NULL;
        }

        for(uint32_t i = 0; i < gamesCount; i++)
        {
            Rect rect = games[i].uiRect;
            rect.ClampByRect(screenRect);

            if(evictIcon && !rect.Area() && games[i].iconState == kIconValid)
            {
                iconCache.FreeGameIcon(games[i].iconIndex);
                games[i].ResetIconState();
                evictIcon = false;
            }

            if(rect.Area() && games[i].iconState == kIconInvalid)
            {
                uint32_t iconIdx = iconCache.AllocateGameIcon();
                if(iconIdx != ~0u)
                {
                    uint8_t* iconAddr = iconCache.gameIcons[iconIdx].addr;
                    request = RequestIcon(api, games[i].id, iconAddr);
                    if(request)
                    {
                        games[i].iconAddr = iconAddr;
                        games[i].iconIndex = iconIdx;
                        games[i].iconState = kIconLoading;
                        games[i].iconRequest = request;
                        iconRequestsInFlight++;
                    }
                    else
                    {
                        iconCache.FreeGameIcon(iconIdx);
                    }
                }
                else
                {
                    evictIcon = true;
                }                
            }

            if(games[i].iconState == kIconLoading && games[i].iconRequest->done)
            {
                HTTPRequest* request = games[i].iconRequest;
                if(games[i].iconRequest->succeed)
                {
                    games[i].iconState = kIconValid;
                }
                else
                {
                    iconCache.FreeGameIcon(games[i].iconIndex);
                    games[i].ResetIconState();
                    games[i].iconState = kIconValidEmpty;
                }

                api->FreeHTTPRequest(request);
                games[i].iconRequest = NULL;
                iconRequestsInFlight--;
            }
        }

        if(state == kMainScreenReady && selectedGame != ~0u)
        {
            request = RequestGameCode(api, games[selectedGame].id, gameCodeMem);
            if(request)
            {
                state = kMainScreenLoadGameCode;
                api->printf("Requested game code: %x\n", games[selectedGame].id);
            }
        }

        if(state == kMainScreenReady && updatePressed && !iconRequestsInFlight)
        {
            gamesCount = 0;
            iconCache.ClearGameIcons();
            request = RequestGamesList(api);
            if(request)
            {
                state = kMainScreenWaitGameList;
                api->printf("Requested games list\n");
            }
        }

        if(state == kMainScreenLoadGameCode && request->done && !iconRequestsInFlight)
        {
            api->FreeHTTPRequest(request);
            request = NULL;

            if(request->succeed)
            {
                //ScopedRamExecutionLock make_ram_executable;
                uint32_t baseAddr = 0;
                api->memcpy(&baseAddr, gameCodeMem, 4);
                uint8_t* gameMainAddr = gameCodeMem + baseAddr + 4;
                TGameMain gameMain;
                gameMain = (TGameMain)&gameMainAddr[1];
                gameMain(api);
                iconCache.Reset();
            }
            else
            {
                api->printf("Failed to load game code\n");
            }

            gamesCount = 0;
            iconCache.ClearGameIcons();
            request = RequestGamesList(api);
            if(request)
            {
                state = kMainScreenWaitGameList;
                api->printf("Requested games list\n");
            }
            else
            {
                state = kMainScreenReady;
            }
        }

        // rendering
        if(state == kMainScreenLoadGameCode)
        {
            api->LCD_Clear(0x00000000);
            api->LCD_SetBackColor(0x00000000);
            api->LCD_SetTextColor(0xffffffff);
            api->LCD_DisplayStringAt(0, 100, "Loading...", kTextAlignCenter);
        }
        else
        {
            api->LCD_DrawImage(backgroundRect, iconCache.background, kBackgroundWidth * 4);
            uint8_t* networkIcon = api->GetNetwokConnectionStatus() == kNetwokConnectionStatusGlobalUp ? iconCache.networkOnIcon : iconCache.networkOffIcon;
            api->LCD_DrawImageWithBlend(networkRect, networkIcon, kInfoIconsWidth * 4);
            api->LCD_DrawImageWithBlend(refreshRect, iconCache.refreshButton, kRefreshButtonWidth * 4);

            if(state != kMainScreenReady || iconRequestsInFlight)
                api->LCD_DrawImageWithBlend(loadingRect, iconCache.loadingIcon, kInfoIconsWidth * 4);

            for(uint32_t g = 0; g < gamesCount; g++)
                DrawIcon(api, screenRect, iconCache, games[g]);
        }
    }
}

