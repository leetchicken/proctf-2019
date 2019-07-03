#include "api.h"

static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kIconSize = kIconWidth * kIconHeight * 4;
static const uint32_t kMaxIconsOnScreen = 3;
static const uint32_t kIconCacheSize = kMaxIconsOnScreen + 1;
static const uint32_t kMaxGamesCount = 256;
static const uint32_t kMaxGameCodeSize = 1024;
Rect GScreenRect;


struct IconCache
{
    struct Item
    {
        uint8_t* addr;
        uint32_t gameIndex;
    };

    Item cache[kIconCacheSize];

    IconCache(uint8_t* startAddr)
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
        {
            cache[i].addr = startAddr + i * kIconSize;
            cache[i].gameIndex = ~0u;
        }
    }

    void Clear()
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
            cache[i].gameIndex = ~0u;
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
    EIconState iconState;

    GameDesc()
        : id(~0u), iconAddr(NULL), iconState(kIconInvalid)
    {
        name[0] = '\0';
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


void FillRect(API* api, Rect rect, uint32_t color)
{
    rect.ClampByRect(GScreenRect);
    if(rect.Area())
        api->LCD_FillRect(rect, color);
}


void DrawIcon(API* api, const GameDesc& desc)
{
    Rect rect = desc.uiRect;
    rect.ClampByRect(GScreenRect);
    if(rect.Area())
    {
        if(desc.iconState == kIconValid)
        {
            uint32_t pitch = kIconWidth * 4;
            api->LCD_DrawImage(rect, desc.iconAddr, pitch);
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
    kMainScreenLoadIcons,
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
    api->GetScreenRect(&GScreenRect);

    TouchScreenState tsState;

    Rect updateRect(20, 20, 40, 40);

    ServerRequest* request = NULL;
    EMainScreenState state = kMainScreenWaitForNetwork;
    uint32_t loadingIconForGameIdx = ~0u;
    uint32_t progressColor = 0xFF0000FF;

    uint8_t* iconsMem = api->GetSDRam();
    IconCache iconCache(iconsMem);

    GameDesc* games = (GameDesc*)(iconsMem + kIconSize * kIconCacheSize);
    uint32_t gamesCount = 0;

    uint8_t* gameCodeMem = (uint8_t*)api->Malloc(kMaxGameCodeSize);

    while(1)
    {
        api->SwapFramebuffer();
        api->GetTouchScreenState(&tsState);

        if(state == kMainScreenWaitForNetwork && api->GetNetwokConnectionStatus() == kNetwokConnectionStatusGlobalUp)
        {
            api->printf("Connected to the network: '%s'\n", api->GetIPAddress());
            request = RequestGamesList(api);
            if(request)
                state = kMainScreenWaitGameList;
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
                    api->memcpy(&desc.id, response, 4);
                    response += 4;
                    uint32_t strLen = api->strlen((char*)response);
                    api->memcpy(desc.name, response, strLen + 1);
                    response += strLen + 1;
                }

                uint32_t curCacheIdx = 0;
                for(uint32_t i = 0; i < gamesCount; i++)
                {
                    games[i].uiRect.x = i * (kIconWidth + 20) + 20;
                    games[i].uiRect.y = 80;
                    games[i].uiRect.width = kIconWidth;
                    games[i].uiRect.height = kIconHeight;

                    Rect rect = games[i].uiRect;
                    rect.ClampByRect(GScreenRect);
                    if(rect.Area() && curCacheIdx < kIconCacheSize)
                    {
                        games[i].iconAddr = iconCache.cache[curCacheIdx].addr;
                        games[i].iconState = kIconInvalid;
                        iconCache.cache[curCacheIdx].gameIndex = i;
                        curCacheIdx++;
                    }
                    else
                    {
                        games[i].iconAddr = NULL;
                        games[i].iconState = kIconInvalid;
                    }
                }

                state = kMainScreenLoadIcons;
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

        if(state == kMainScreenLoadIcons)
        {
            if(loadingIconForGameIdx != ~0u && request->done)
            {
                api->FreeServerRequest(request);
                request = NULL;
                games[loadingIconForGameIdx].iconState = kIconValid;
                loadingIconForGameIdx = ~0u;
            }

            if(loadingIconForGameIdx == ~0u)
            {
                for(uint32_t i = 0; i < gamesCount; i++)
                {
                    if(games[i].iconAddr && games[i].iconState == kIconInvalid && !request)
                    {
                        request = RequestIcon(api, games[i].id, games[i].iconAddr);
                        if(request)
                        {
                            games[i].iconState = kIconLoading;
                            loadingIconForGameIdx = i;
                        }
                        break;
                    }
                }
            }

            // TODO error handling is missing
            if(loadingIconForGameIdx == ~0u)
                state = kMainScreenReady;
        }

        uint32_t selectedGame = ~0u;
        for (uint8_t i = 0; i < tsState.touchDetected; i++)
        {
            /*if(tsState.touchEventId[i] != TOUCH_EVENT_PRESS_DOWN)
                continue;*/

            for(uint32_t g = 0; g < gamesCount; g++)
            {
                Rect& rect = games[g].uiRect;
                if(rect.IsPointInside(tsState.touchX[i], tsState.touchY[i]))
                {
                    selectedGame = g;
                    break;
                }
            }
            
            if(selectedGame != ~0u)
            	break;

            if(state == kMainScreenReady && updateRect.IsPointInside(tsState.touchX[i], tsState.touchY[i]))
            {
                gamesCount = 0;
                iconCache.Clear();
                request = RequestGamesList(api);
                if(request)
                {
                    state = kMainScreenWaitGameList;
                    api->printf("Requested games list\n");
                }
                break;
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

        if(state == kMainScreenLoadGameCode && request->done)
        {
            if(request->succeed)
            {
                api->FreeServerRequest(request);
                request = NULL;
                //ScopedRamExecutionLock make_ram_executable;
                TGameMain gameMain;
                gameMain = (TGameMain)&gameCodeMem[1];
                gameMain(api);
            }
            else
            {
                api->printf("Failed to load game code\n");
                api->FreeServerRequest(request);
                request = NULL;
                gamesCount = 0;
                iconCache.Clear();
                request = RequestGamesList(api);
                if(request)
                {
                    state = kMainScreenWaitGameList;
                    api->printf("Requested games list\n");
                }
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

            if(state != kMainScreenReady)
            {
                api->LCD_FillRect(Rect(440, 20, 40, 40), progressColor);
                uint32_t b = progressColor & 255;
                b = (b + 1) & 255;
                progressColor = 0xFF000000 | ((255 - b) << 8) | b;
            }

            for(uint32_t g = 0; g < gamesCount; g++)
                DrawIcon(api, games[g]);
        }
    }
}

