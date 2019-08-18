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
    uint32_t assetsNum;
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


enum EMainScreenState
{
    kMainScreenReady = 0,
    kMainScreenRegistration,
    kMainScreenWaitAuthKey,
    kMainScreenWaitForNetwork,
    kMainScreenWaitGameList,
    kMainScreenLoadGameAsset,
    kMainScreenLoadGameCode,
    kMainScreenWaitForGameFinish,

    kMainScreenStatesCount
};


struct Context
{
    API* m_api;
    char m_userName[128];
    char m_password[128];
    Rect m_screenRect;
    TouchScreenState m_tsState;

    Rect m_backgroundRect;
    Rect m_networkRect;
    Rect m_loadingRect;
    Rect m_refreshRect;

    HTTPRequest* m_request;
    EMainScreenState m_state;
    uint32_t m_authKey;

    uint8_t* m_curSdram;
    IconsManager m_iconCache;
    bool m_evictIcon;
    uint32_t m_iconRequestsInFlight;

    GameDesc* m_games;
    uint32_t m_gamesCount;

    uint8_t* m_gameCodeMem;
    uint32_t m_curGame;
    uint32_t m_curAssetLoading;
    TGameUpdate m_gameUpdate;
    void* m_gameCtx;

    bool m_touchOnPrevFrame;
    uint16_t m_prevTouchX;
    uint16_t m_prevTouchY;
    float m_pressDownTime;

    NotificationsCtx m_notificationsCtx;

    float m_timer;

    Context(API* api, uint8_t* sdram)
        : m_api(api)
    {
        m_api->GetScreenRect(&m_screenRect);

        ReadCredentials();

        m_backgroundRect = Rect(0, 0, kBackgroundWidth, kBackgroundHeight);
        m_networkRect = Rect(10, 10, kInfoIconsWidth, kInfoIconsHeight);
        m_loadingRect = Rect(430, 10, kInfoIconsWidth, kInfoIconsHeight);
        m_refreshRect = Rect(163, 238, kRefreshButtonWidth, kRefreshButtonHeight);

        m_request = NULL;
        m_state = kMainScreenWaitForNetwork;
        m_authKey = ~0u;

        m_curSdram = m_iconCache.Init(api, sdram);
        m_evictIcon = false;
        m_iconRequestsInFlight = 0;

        m_games = (GameDesc*)m_curSdram;
        m_gamesCount = 0;
        m_curSdram += sizeof(GameDesc) * kMaxGamesCount;

        m_gameCodeMem = (uint8_t*)api->Malloc(kMaxGameCodeSize);
        m_curGame = ~0u;
        m_curAssetLoading = ~0u;
        m_gameUpdate = NULL;
        m_gameCtx = NULL;

        m_touchOnPrevFrame = false;
        m_prevTouchX = 0;
        m_prevTouchY = 0;
        m_pressDownTime = 0.0f;

        m_notificationsCtx.Init(api);

        m_timer = api->time();
    }

    void ReadCredentials()
    {
        m_api->memset(m_userName, 0, sizeof(m_userName));
        m_api->memset(m_password, 0, sizeof(m_password));

        void* f = m_api->fopen("/fs/username", "r");
        if(f)
        {
            uint32_t size = m_api->fsize(f);
            m_api->fread(m_userName, size, f);
            m_api->fclose(f);
        }
        else
        {
            m_api->sprintf(m_userName, "Unknown");
        }

        f = m_api->fopen("/fs/password", "r");
        if(f)
        {
            uint32_t size = m_api->fsize(f);
            m_api->fread(m_password, size, f);
            m_api->fclose(f);
        }
    }

    bool Update();

    void ProcessInput();
    void OnGameSelected(uint32_t gameIdx);
    void OnUpdatePressed();
    void UpdateIcons();
    bool ProcessLoadedAsset();
    void Render(float dt);
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


HTTPRequest* RequestRegistration(API* api, const char* userName, const char* password)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/register?u=%s&p=%s", kServerIP, kServerPort, userName, password);
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


HTTPRequest* RequestAuthKey(API* api, const char* userName, const char* password)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/auth?u=%s&p=%s", kServerIP, kServerPort, userName, password);
    if(!api->SendHTTPRequest(request))
    {
        api->FreeHTTPRequest(request);
        return NULL;
    }
    return request;
}


bool ParseAuthResponse(API* api, HTTPRequest* r, uint32_t& authKey)
{
    if(!r->succeed)
    {
        authKey = ~0u;
        return false;
    }

    api->memcpy(&authKey, r->responseData, sizeof(uint32_t));
    return true;
}


void ParseGamesList(API* api, HTTPRequest* request, GameDesc* games, uint32_t& gamesCount)
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
        api->memcpy(&desc.assetsNum, response, 4);
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


HTTPRequest* RequestGameAsset(API* api, uint32_t authKey, uint32_t gameId, uint32_t assetIndex, uint8_t* assetData)
{
    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        return NULL;
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s:%u/asset?auth=%x&id=%x&index=%u", kServerIP, kServerPort, authKey, gameId, assetIndex);
    request->responseData = (void*)assetData;
    request->responseDataCapacity = kMaxGameAssetSize;
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


void Context::ProcessInput()
{
    m_api->GetTouchScreenState(&m_tsState);

    uint32_t selectedGame = ~0u;
    bool updatePressed = false;

    bool pressDown = false, pressUp = false;
    // press down detection
    if(!m_touchOnPrevFrame && (m_tsState.touchDetected == 1))
    {
        m_pressDownTime = m_api->time();
        pressDown = true;
    }
    // press up detection
    if(m_touchOnPrevFrame && !m_tsState.touchDetected)
        pressUp = true;

    if(pressUp && m_api->time() - m_pressDownTime < 0.1f)
    {
        for(uint32_t g = 0; g < m_gamesCount; g++)
        {
            Rect& rect = m_games[g].uiRect;
            if(rect.IsPointInside(m_prevTouchX, m_prevTouchY))
            {
                selectedGame = g;
                break;
            }
        }

        if(selectedGame == ~0u && m_refreshRect.IsPointInside(m_prevTouchX, m_prevTouchY))
            updatePressed = true;
    }
    else if(m_touchOnPrevFrame && (m_tsState.touchDetected == 1))
    {
        int vecX = (int)m_tsState.touchX[0] - (int)m_prevTouchX;
        for(uint32_t g = 0; g < m_gamesCount; g++)
            m_games[g].uiRect.x += vecX;
    }

    m_touchOnPrevFrame = false;
    if(m_tsState.touchDetected == 1)
    {
        m_prevTouchX = m_tsState.touchX[0];
        m_prevTouchY = m_tsState.touchY[0];
        m_touchOnPrevFrame = true;
    }

    if(selectedGame != ~0u)
        OnGameSelected(selectedGame);
    if(updatePressed)
        OnUpdatePressed();
}


void Context::OnGameSelected(uint32_t gameIdx)
{
    if(m_state == kMainScreenReady)
    {
        GameDesc& desc = m_games[gameIdx];
        if(desc.assetsNum)
        {
            m_curAssetLoading = 0;
            m_request = RequestGameAsset(m_api, m_authKey, desc.id, m_curAssetLoading, m_curSdram);
            if(m_request)
            {
                m_state = kMainScreenLoadGameAsset;
                m_api->printf("Requested game asset: %x %u/%u\n", desc.id, m_curAssetLoading, desc.assetsNum);
            }
        }
        else
        {
            m_request = RequestGameCode(m_api, m_authKey, desc.id, m_gameCodeMem);
            if(m_request)
            {
                m_state = kMainScreenLoadGameCode;
                m_api->printf("Requested game code: %x\n", desc.id);
            }
        }

        m_curGame = gameIdx;
    }
}


void Context::OnUpdatePressed()
{
    if(m_state == kMainScreenReady && !m_iconRequestsInFlight)
    {
        m_gamesCount = 0;
        m_iconCache.ClearGameIcons();
        if(m_authKey == ~0u)
        {
            m_request = RequestAuthKey(m_api, m_userName, m_password);
            if(m_request)
                m_state = kMainScreenWaitAuthKey;
        }
        else
        {
            m_request = RequestGamesList(m_api, m_authKey);
            if(m_request)
                m_state = kMainScreenWaitGameList;
        }
    }
}


void Context::UpdateIcons()
{
    for(uint32_t i = 0; i < m_gamesCount; i++)
    {
        Rect rect = m_games[i].uiRect;
        rect.ClampByRect(m_screenRect);

        if(m_evictIcon && !rect.Area() && m_games[i].iconState == kIconValid)
        {
            m_iconCache.FreeGameIcon(m_games[i].iconIndex);
            m_games[i].ResetIconState();
            m_evictIcon = false;
        }

        if(rect.Area() && m_games[i].iconState == kIconInvalid)
        {
            uint32_t iconIdx = m_iconCache.AllocateGameIcon();
            if(iconIdx != ~0u)
            {
                uint8_t* iconAddr = m_iconCache.gameIcons[iconIdx].addr;
                m_request = RequestIcon(m_api, m_authKey, m_games[i].id, iconAddr);
                if(m_request)
                {
                    m_games[i].iconAddr = iconAddr;
                    m_games[i].iconIndex = iconIdx;
                    m_games[i].iconState = kIconLoading;
                    m_games[i].iconRequest = m_request;
                    m_iconRequestsInFlight++;
                }
                else
                {
                    m_iconCache.FreeGameIcon(iconIdx);
                }
            }
            else
            {
                m_evictIcon = true;
            }                
        }

        if(m_games[i].iconState == kIconLoading && m_games[i].iconRequest->done)
        {
            HTTPRequest* request = m_games[i].iconRequest;
            if(m_games[i].iconRequest->succeed)
            {
                m_games[i].iconState = kIconValid;
            }
            else
            {
                m_iconCache.FreeGameIcon(m_games[i].iconIndex);
                m_games[i].ResetIconState();
                m_games[i].iconState = kIconValidEmpty;
            }

            m_api->FreeHTTPRequest(request);
            m_games[i].iconRequest = NULL;
            m_iconRequestsInFlight--;
        }
    }
}


bool Context::ProcessLoadedAsset()
{
    if(!m_request->succeed)
    {
        m_api->FreeHTTPRequest(m_request);
        m_request = NULL;
        return false;
    }

    const char* name = (char*)m_request->responseData;
    uint32_t nameLen = m_api->strlen(name);
    uint8_t* asset = (uint8_t*)m_request->responseData + nameLen + 1;
    uint32_t assetSize = m_request->responseDataSize - nameLen - 1;

    m_api->FreeHTTPRequest(m_request);
    m_request = NULL;

    m_api->printf("Asset loaded: %s %u bytes\n", name, assetSize);

    char assetPath[256];
    m_api->sprintf(assetPath, "/fs/%s", name);
    void* f = m_api->fopen(assetPath, "w");
    if(!f)
    {
        m_api->printf("Failed to create file %s\n", name);
        m_api->fclose(f);
        return false;
    }

    if(m_api->fwrite(asset, assetSize, f) != assetSize)
    {
        m_api->printf("Failed to create file %s\n", name);
        m_api->fclose(f);
        return false;
    }

    m_api->fclose(f);

    m_curAssetLoading++;

    GameDesc& desc = m_games[m_curGame];
    if(m_curAssetLoading == desc.assetsNum)
    {
        m_request = RequestGameCode(m_api, m_authKey, desc.id, m_gameCodeMem);
        if(m_request)
        {
            m_state = kMainScreenLoadGameCode;
            m_api->printf("Requested game code: %x\n", desc.id);
        }
    }
    else
    {
        m_request = RequestGameAsset(m_api, m_authKey, desc.id, m_curAssetLoading, m_curSdram);
        if(m_request)
            m_api->printf("Requested game asset: %x %u/%u\n", desc.id, m_curAssetLoading, desc.assetsNum);
    }

    if(!m_request)
        return false;

    return true;
}


void Context::Render(float dt)
{
    if(m_state == kMainScreenLoadGameCode || m_state == kMainScreenLoadGameAsset)
    {
        m_api->LCD_Clear(0x00000000);
        m_api->LCD_SetBackColor(0x00000000);
        m_api->LCD_SetTextColor(0xffffffff);
        m_api->LCD_SetFont(kFont24);
        m_api->LCD_DisplayStringAt(0, 100, "Loading...", kTextAlignCenter);
    }
    else
    {
        m_api->LCD_DrawImage(m_backgroundRect, m_iconCache.background, kBackgroundWidth * 4);
        uint8_t* networkIcon = m_api->GetNetwokConnectionStatus() == kNetwokConnectionStatusGlobalUp ? m_iconCache.networkOnIcon : m_iconCache.networkOffIcon;
        m_api->LCD_DrawImageWithBlend(m_networkRect, networkIcon, kInfoIconsWidth * 4);
        m_api->LCD_DrawImageWithBlend(m_refreshRect, m_iconCache.refreshButton, kRefreshButtonWidth * 4);

        if(m_state != kMainScreenReady || m_iconRequestsInFlight)
            m_api->LCD_DrawImageWithBlend(m_loadingRect, m_iconCache.loadingIcon, kInfoIconsWidth * 4);

        for(uint32_t g = 0; g < m_gamesCount; g++)
            DrawIcon(m_api, m_screenRect, m_iconCache, m_games[g]);

        char buf[256];
        m_api->LCD_SetFont(kFont8);
        m_api->LCD_SetTextColor(0xffffffff);
        m_api->sprintf(buf, "User name: %s", m_userName);
        m_api->LCD_DisplayStringAt(5, 248, buf, kTextAlignNone);
        m_api->sprintf(buf, "Password: %s", m_password);
        m_api->LCD_DisplayStringAt(5, 256, buf, kTextAlignNone);
        m_api->sprintf(buf, "Visit our site http://%s", kServerIP);
        m_api->LCD_DisplayStringAt(5, 264, buf, kTextAlignNone);
    }

    m_notificationsCtx.Render(dt);
}


bool Context::Update()
{
    m_notificationsCtx.Update();

    float curTime = m_api->time();
    float dt = curTime - m_timer;
    m_timer = curTime;

    if(m_state == kMainScreenWaitForGameFinish)
    {
        if(m_gameUpdate(m_api, m_gameCtx))
        {
            m_notificationsCtx.Render(dt);
            return true;
        }

        m_state = kMainScreenReady;
        m_curGame = ~0u;
    }

    ProcessInput();
    UpdateIcons();

    if(m_state == kMainScreenWaitForNetwork && m_api->GetNetwokConnectionStatus() == kNetwokConnectionStatusGlobalUp)
    {
        m_api->printf("Connected to the network: '%s'\n", m_api->GetIPAddress());
        m_request = RequestRegistration(m_api, m_userName, m_password);
        if(m_request)
            m_state = kMainScreenRegistration;
        else
            m_state = kMainScreenReady;
    }

    if(m_state == kMainScreenRegistration && m_request->done)
    {
        m_api->FreeHTTPRequest(m_request);
        m_request = RequestAuthKey(m_api, m_userName, m_password);
        m_state = kMainScreenWaitAuthKey;
    }

    if(m_state == kMainScreenWaitAuthKey && m_request->done)
    {
        if(ParseAuthResponse(m_api, m_request, m_authKey))
        {
            m_api->FreeHTTPRequest(m_request);
            m_notificationsCtx.SetAuthKey(m_authKey);
            m_request = RequestGamesList(m_api, m_authKey);
            m_state = kMainScreenWaitGameList;
        }
        else
        {
            m_state = kMainScreenReady;
            m_api->FreeHTTPRequest(m_request);
            m_request = NULL;
        }
    }

    if(m_state == kMainScreenWaitGameList && m_request->done)
    {
        if(m_request->succeed)
        {
            ParseGamesList(m_api, m_request, m_games, m_gamesCount);
            m_api->printf("Received games list\n");
        }
        else
        {
            m_api->printf("Error occured while loading games list\n");
        }            

        m_state = kMainScreenReady;
        m_api->FreeHTTPRequest(m_request);
        m_request = NULL;
    }    

    if(m_state == kMainScreenLoadGameAsset && m_request->done)
    {
        if(!ProcessLoadedAsset())
        {
            GameDesc& desc = m_games[m_curGame];
            m_api->printf("Failed to load game asset %u/%u\n", m_curAssetLoading, desc.assetsNum);
            m_state = kMainScreenReady;
            m_curGame = ~0u;
        }
    }

    if(m_state == kMainScreenLoadGameCode && m_request->done && !m_iconRequestsInFlight)
    {
        if(m_request->succeed)
        {
            char buf[64];
            m_api->memset(buf, 0, 64);
            m_api->sprintf(buf, "Start game '%s'", m_games[m_curGame].name);
            m_notificationsCtx.Post(m_userName, buf);
            
            m_api->printf("Start game '%s'\n", m_games[m_curGame].name);

            uint8_t* ptr = m_gameCodeMem;

            uint32_t gameInitOffset = 0;
            m_api->memcpy(&gameInitOffset, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            uint32_t gameUpdateOffset = 0;
            m_api->memcpy(&gameUpdateOffset, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);

            uint8_t* gameInitAddr = ptr + gameInitOffset;
            uint8_t* gameUpdateAddr = ptr + gameUpdateOffset;

            TGameInit gameInit = (TGameInit)&gameInitAddr[1];
            m_gameCtx = gameInit(m_api, m_curSdram);
            m_gameUpdate = (TGameUpdate)&gameUpdateAddr[1];

            m_state = kMainScreenWaitForGameFinish;
        }
        else
        {
            m_api->printf("Failed to load game code\n");
            m_state = kMainScreenReady;
            m_curGame = ~0u;
        }

        m_api->FreeHTTPRequest(m_request);
        m_request = NULL;
    }

    Render(dt);

    return true;
}


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

