#include "../hw/api.h"
#include "constants.h"
#include "notifications.h"
#include "icons_manager.h"


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
    kMainScreenWaitAuthKey,
    kMainScreenWaitForNetwork,
    kMainScreenWaitGameList,
    kMainScreenLoadGameCode,
    kMainScreenWaitForGameFinish,

    kMainScreenStatesCount
};


HTTPRequest* RequestAuthKey(API* api)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/auth", kServerIP, kServerPort);
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


bool ParseAuthResponse(API* api, HTTPRequest* r, uint32_t& authKey, uint16_t& notifyPort)
{
    if(!r->succeed)
    {
        authKey = ~0u;
        return false;
    }

    char* ptr = (char*)r->responseData;
    api->memcpy(&authKey, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    api->memcpy(&notifyPort, ptr, sizeof(uint16_t));

    return true;
}


HTTPRequest* RequestGamesList(API* api, uint32_t authKey)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/list?auth=%x", kServerIP, kServerPort, authKey);
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


HTTPRequest* RequestIcon(API* api, uint32_t authKey, uint32_t gameId, uint8_t* iconAddr)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/icon?auth=%x&id=%x", kServerIP, kServerPort, authKey, gameId);
    request->responseData = (void*)iconAddr;
    request->responseDataCapacity = kGameIconSize;
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


HTTPRequest* RequestGameCode(API* api, uint32_t authKey, uint32_t gameId, uint8_t* codeAddr)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/code?auth=%x&id=%x", kServerIP, kServerPort, authKey, gameId);
    request->responseData = (void*)codeAddr;
    request->responseDataCapacity = kMaxGameCodeSize;
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


struct Context
{
    API* api;
    Rect screenRect;
    TouchScreenState tsState;

    Rect backgroundRect;
    Rect networkRect;
    Rect loadingRect;
    Rect refreshRect;

    HTTPRequest* request;
    EMainScreenState state;
    uint32_t authKey;

    uint8_t* curSdram;
    IconsManager iconCache;
    bool evictIcon;
    uint32_t iconRequestsInFlight;

    GameDesc* games;
    uint32_t gamesCount;

    uint8_t* gameCodeMem;
    uint32_t curGame;
    TGameUpdate gameUpdate;
    void* gameCtx;

    bool touchOnPrevFrame;
    uint16_t prevTouchX;
    uint16_t prevTouchY;
    float pressDownTime;

    NotificationsCtx notificationsCtx;

    float timer;

    Context(API* api, uint8_t* sdram)
        : api(api)
    {
        api->GetScreenRect(&screenRect);

        backgroundRect = Rect(0, 0, kBackgroundWidth, kBackgroundHeight);
        networkRect = Rect(10, 10, kInfoIconsWidth, kInfoIconsHeight);
        loadingRect = Rect(430, 10, kInfoIconsWidth, kInfoIconsHeight);
        refreshRect = Rect(163, 238, kRefreshButtonWidth, kRefreshButtonHeight);

        request = NULL;
        state = kMainScreenWaitForNetwork;
        authKey = ~0u;

        curSdram = iconCache.Init(api, sdram);
        evictIcon = false;
        iconRequestsInFlight = 0;

        games = (GameDesc*)curSdram;
        gamesCount = 0;
        curSdram += sizeof(GameDesc) * kMaxGamesCount;

        gameCodeMem = (uint8_t*)api->Malloc(kMaxGameCodeSize);
        curGame = ~0u;
        gameUpdate = NULL;
        gameCtx = NULL;

        touchOnPrevFrame = false;
        prevTouchX = 0;
        prevTouchY = 0;
        pressDownTime = 0.0f;

        notificationsCtx.Init(api);

        timer = api->time();
    }

    bool Update();
};


inline void* operator new(size_t, void* __p)
{ return __p; }


void* GameInit(API* api, uint8_t* sdram)
{
    void* mem = api->Malloc(sizeof(Context));
    Context* ctx = new(mem) Context(api, sdram);
    return ctx;
}


bool GameUpdate(API* api, void* ctxVoid)
{
    Context* ctx = (Context*)ctxVoid;
    return ctx->Update();
}


bool Context::Update()
{
    notificationsCtx.Update();

    float curTime = api->time();
    float dt = curTime - timer;
    timer = curTime;

    if(state == kMainScreenWaitForGameFinish)
    {
        if(gameUpdate(api, gameCtx))
        {
            notificationsCtx.Render(dt);
            return true;
        }

        state = kMainScreenReady;
        curGame = ~0u;
    }

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
        request = RequestAuthKey(api);
        if(request)
            state = kMainScreenWaitAuthKey;
        else
            state = kMainScreenReady;
    }

    if(state == kMainScreenWaitAuthKey && request->done)
    {
        uint16_t notifyPort = 0;
        if(ParseAuthResponse(api, request, authKey, notifyPort))
        {
            api->FreeHTTPRequest(request);
            notificationsCtx.SetNotifyPort(notifyPort);
            notificationsCtx.SetAuthKey(authKey);
            request = RequestGamesList(api, authKey);
            state = kMainScreenWaitGameList;
        }
        else
        {
            state = kMainScreenReady;
            api->FreeHTTPRequest(request);
            request = NULL;
        }
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
                request = RequestIcon(api, authKey, games[i].id, iconAddr);
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
        request = RequestGameCode(api, authKey, games[selectedGame].id, gameCodeMem);
        if(request)
        {
            state = kMainScreenLoadGameCode;
            api->printf("Requested game code: %x\n", games[selectedGame].id);
            curGame = selectedGame;
        }
    }

    if(state == kMainScreenReady && updatePressed && !iconRequestsInFlight)
    {
        gamesCount = 0;
        iconCache.ClearGameIcons();
        if(authKey == ~0u)
        {
            request = RequestAuthKey(api);
            if(request)
                state = kMainScreenWaitAuthKey;
        }
        else
        {
            request = RequestGamesList(api, authKey);
            if(request)
                state = kMainScreenWaitGameList;
        }
    }

    if(state == kMainScreenLoadGameCode && request->done && !iconRequestsInFlight)
    {
        if(request->succeed)
        {
            char buf[64];
            api->memset(buf, 0, 64);
            api->sprintf(buf, "Start game '%s'", games[curGame].name);
            api->printf(buf);
            notificationsCtx.Post(api->GetUserName(), buf);

            uint8_t* ptr = gameCodeMem;

            uint32_t gameInitOffset = 0;
            api->memcpy(&gameInitOffset, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            uint32_t gameUpdateOffset = 0;
            api->memcpy(&gameUpdateOffset, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            uint8_t* gameInitAddr = ptr + gameInitOffset;
            uint8_t* gameUpdateAddr = ptr + gameUpdateOffset;

            TGameInit gameInit = (TGameInit)&gameInitAddr[1];
            gameCtx = gameInit(api, curSdram);
            gameUpdate = (TGameUpdate)&gameUpdateAddr[1];

            state = kMainScreenWaitForGameFinish;
        }
        else
        {
            api->printf("Failed to load game code\n");
            state = kMainScreenReady;
            curGame = ~0u;
        }

        api->FreeHTTPRequest(request);
        request = NULL;
    }

    // rendering
    if(state == kMainScreenLoadGameCode)
    {
        api->LCD_Clear(0x00000000);
        api->LCD_SetBackColor(0x00000000);
        api->LCD_SetTextColor(0xffffffff);
        api->LCD_SetFont(kFont24);
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

    notificationsCtx.Render(dt);

    return true;
}

