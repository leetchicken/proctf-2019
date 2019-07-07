#include "../hw/api.h"

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kIconSize = kIconWidth * kIconHeight * 4;
static const uint32_t kMaxIconsOnScreen = 3;
static const uint32_t kIconCacheSize = kMaxIconsOnScreen + 1;
static const uint32_t kMaxGamesCount = 256;
static const uint32_t kMaxGameCodeSize = 1024;


struct IconCache
{
    struct Item
    {
        uint8_t* addr;
        uint32_t gameIndex;
    };

    Item cache[kIconCacheSize];
    uint32_t freeItems[kIconCacheSize];
    uint32_t freeItemsNum;
    uint8_t* errorIcon;

    IconCache(uint8_t* startAddr, API* api)
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
        {
            cache[i].addr = startAddr + i * kIconSize;
            cache[i].gameIndex = ~0u;
            freeItems[i] = i;
        }
        freeItemsNum = kIconCacheSize;

        errorIcon = startAddr + kIconCacheSize * kIconSize;
        void* f = api->fopen("/fs/error.bmp", "r");
        api->fread(errorIcon, kIconSize, f);
        api->fclose(f);
    }

    uint32_t Allocate(uint32_t gameIdx)
    {
        if(!freeItemsNum)
            return ~0u;
        
        uint32_t ret = freeItems[freeItemsNum - 1];
        freeItemsNum--;
        cache[ret].gameIndex = gameIdx;
        return ret;
    }

    void Free(uint32_t idx)
    {
        cache[idx].gameIndex = ~0u;
        freeItems[freeItemsNum] = idx;
        freeItemsNum++;
    }

    void Clear()
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
        {
            cache[i].gameIndex = ~0u;
            freeItems[i] = i;
        }
        freeItemsNum = kIconCacheSize;
    }
};


enum EIconState
{
    kIconInvalid = 0,
    kIconLoading,
    kIconValid,

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
    ServerRequest* iconRequest;

    GameDesc()
        : id(~0u)
    {
        uiRect.y = 80;
        uiRect.width = kIconWidth;
        uiRect.height = kIconHeight;
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


enum ERequests
{
    kRequestGameList = 0,
    kRequestIcon,
    kRequestGameCode,
    kReqeustGameAssets,

    kRequestsCount
};


void FillRect(API* api, const Rect& screenRect, Rect rect, uint32_t color)
{
    rect.ClampByRect(screenRect);
    if(rect.Area())
        api->LCD_FillRect(rect, color);
}


void DrawIcon(API* api, const Rect& screenRect, const GameDesc& desc)
{
    Rect rect = desc.uiRect;
    rect.ClampByRect(screenRect);
    if(rect.Area())
    {
        if(desc.iconState == kIconValid)
        {
            uint32_t pitch = kIconWidth * 4;
            uint32_t offset = 0;
            if(desc.uiRect.x < 0)
                offset = -desc.uiRect.x * 4;
            api->LCD_DrawImage(rect, desc.iconAddr + offset, pitch);
        }
        else
        {
            api->LCD_FillRect(rect, 0xFFFFFFFF);
        }        
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


ServerRequest* RequestGamesList(API* api)
{
    ServerRequest* request = api->AllocServerRequest();
    api->strcpy(request->url, "list");
    if(api->SendServerRequest(request))
        return request;
    return NULL;
}


ServerRequest* RequestIcon(API* api, uint32_t gameId, uint8_t* iconAddr)
{
    ServerRequest* request = api->AllocServerRequest();
    api->sprintf(request->url, "icon?id=%u", gameId);
    request->responseData = (void*)iconAddr;
    request->responseDataCapacity = kIconSize;
    if(api->SendServerRequest(request))
        return request;
    return NULL;
}


ServerRequest* RequestGameCode(API* api, uint32_t gameId, uint8_t* codeAddr)
{
    ServerRequest* request = api->AllocServerRequest();
    api->sprintf(request->url, "code?id=%u", gameId);
    request->responseData = (void*)codeAddr;
    request->responseDataCapacity = kMaxGameCodeSize;
    if(api->SendServerRequest(request))
        return request;
    return NULL;
}


int GameMain(API* api)
{
    Rect screenRect;
    api->GetScreenRect(&screenRect);

    TouchScreenState tsState;

    Rect updateRect(20, 20, 40, 40);

    ServerRequest* request = NULL;
    EMainScreenState state = kMainScreenWaitForNetwork;
    uint32_t progressColor = 0xFF0000FF;

    uint8_t* iconsMem = api->GetSDRam();
    IconCache iconCache(iconsMem, api);
    bool evictIcon = false;
    uint32_t iconRequestsInFlight = 0;

    GameDesc* games = (GameDesc*)(iconsMem + kIconSize * (kIconCacheSize + 1));
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
                if(rect.IsPointInside(prevTouchX, prevTouchX))
                {
                    selectedGame = g;
                    break;
                }
            }

            if(selectedGame == ~0u && updateRect.IsPointInside(prevTouchX, prevTouchX))
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
                    games[i].uiRect.x = i * (kIconWidth + 20) + 20;
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

            api->FreeServerRequest(request);
            request = NULL;
        }

        for(uint32_t i = 0; i < gamesCount; i++)
        {
            Rect rect = games[i].uiRect;
            rect.ClampByRect(screenRect);

            if(evictIcon && !rect.Area() && games[i].iconState == kIconValid)
            {
                iconCache.Free(games[i].iconIndex);
                games[i].ResetIconState();
                evictIcon = false;
            }

            if(rect.Area() && games[i].iconState == kIconInvalid)
            {
                uint32_t iconIdx = iconCache.Allocate(i);
                if(iconIdx != ~0u)
                {
                    uint8_t* iconAddr = iconCache.cache[iconIdx].addr;
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
                        iconCache.Free(iconIdx);
                    }
                }
                else
                {
                    evictIcon = true;
                }                
            }

            if(games[i].iconState == kIconLoading && games[i].iconRequest->done)
            {
                ServerRequest* request = games[i].iconRequest;
                if(!games[i].iconRequest->succeed)
                {
                    iconCache.Free(games[i].iconIndex);
                    games[i].ResetIconState();
                    games[i].iconAddr = iconCache.errorIcon;
                }

                api->FreeServerRequest(request);
                games[i].iconRequest = NULL;
                games[i].iconState = kIconValid;
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
            iconCache.Clear();
            request = RequestGamesList(api);
            if(request)
            {
                state = kMainScreenWaitGameList;
                api->printf("Requested games list\n");
            }
        }

        if(state == kMainScreenLoadGameCode && request->done && !iconRequestsInFlight)
        {
            api->FreeServerRequest(request);
            request = NULL;

            if(request->succeed)
            {
                //ScopedRamExecutionLock make_ram_executable;
                TGameMain gameMain;
                gameMain = (TGameMain)&gameCodeMem[1];
                gameMain(api);
            }
            else
            {
                api->printf("Failed to load game code\n");
            }

            gamesCount = 0;
            iconCache.Clear();
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
            api->LCD_Clear(kColorDarkBlue);
            api->LCD_FillRect(updateRect, kColorLightCyan);

            if(state != kMainScreenReady || iconRequestsInFlight)
            {
                api->LCD_FillRect(Rect(440, 20, 40, 40), progressColor);
                uint32_t b = progressColor & 255;
                b = (b + 1) & 255;
                progressColor = 0xFF000000 | ((255 - b) << 8) | b;
            }

            for(uint32_t g = 0; g < gamesCount; g++)
                DrawIcon(api, screenRect, games[g]);
        }
    }
}

