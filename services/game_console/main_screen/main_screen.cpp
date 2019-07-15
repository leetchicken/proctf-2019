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
static const char* kServerIP = "192.168.1.1";
static const uint32_t kServerPort = 8000;


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


struct NotificationsCtx
{
    uint32_t authKey;

    NotificationsCtx()
        : authKey(~0u), m_socket(-1), m_postRequest(NULL), m_getRequest(NULL), m_pendingNotificationsNum(0), m_gotNotification(false)
        , m_userName(NULL), m_userNameLen(0), m_message(NULL), m_messageLen(0)
    {
    }

    bool Init(API* api)
    {
        m_api = api;

        m_socket = api->socket(false);
        if(m_socket < 0)
        {
            api->printf("Failed to open notifications socket, can not continue\n");
            return false;
        }
        api->set_blocking(m_socket, false);

        return true;
    }

    void SetNotifyPort(uint16_t notifyPort)
    {
        m_api->bind(m_socket, 0, notifyPort);
    }

    void SetAuthKey(uint32_t k)
    {
        authKey = k;
    }

    void Post(const char* userName, const char* message)
    {
        if(m_postRequest)
            return;

        m_postRequest = m_api->AllocHTTPRequest();
        if(!m_postRequest)
            return;
        m_postRequest->httpMethod = kHttpMethodPost;
        m_api->sprintf(m_postRequest->url, "http://%s:%u/notification?auth=%x", kServerIP, kServerPort, authKey);
        uint32_t userNameLen = m_api->strlen(userName);
        uint32_t messageLen = m_api->strlen(message);
        m_postRequest->requestBodySize = userNameLen + messageLen + sizeof(uint32_t) * 2;
        m_postRequest->requestBody = m_api->Malloc(m_postRequest->requestBodySize);
        uint8_t* ptr = (uint8_t*)m_postRequest->requestBody;
        m_api->memset(ptr, 0, m_postRequest->requestBodySize);

        m_api->memcpy(ptr, &userNameLen, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        if(userNameLen)
        {
            m_api->memcpy(ptr, userName, userNameLen);
            ptr += userNameLen;
        }

        m_api->memcpy(ptr, &messageLen, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        if(messageLen)
        {
            m_api->memcpy(ptr, message, messageLen);
            ptr += messageLen;
        }
        
        if(!m_api->SendHTTPRequest(m_postRequest))
            FreePostRequest();
    }

    void Update()
    {
        if(m_postRequest && m_postRequest->done)
            FreePostRequest();

        if(m_getRequest && m_getRequest->done)
        {
            m_gotNotification = false;
            if(m_getRequest->succeed && m_getRequest->responseDataSize)
            {
                char* ptr = data;
                m_api->memcpy(&m_userNameLen, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                m_userName = ptr;
                ptr += m_userNameLen;
                m_api->memcpy(&m_messageLen, ptr, sizeof(uint32_t));
                ptr += sizeof(uint32_t);
                m_message = ptr;
                m_gotNotification = true;
            }
            FreeGetRequest();
            m_pendingNotificationsNum--;
        }

        uint32_t serverIP = m_api->aton(kServerIP);
        char data[16];
        NetAddr addr;
        int ret = m_api->recv(m_socket, data, 16, &addr);
        if(ret > 0)
        {
            m_api->printf("Notification is available\n");
            m_pendingNotificationsNum++;
        }

        if(m_pendingNotificationsNum > 0 && !m_getRequest && !m_gotNotification)
            Get();
    }

    bool GotNotification()
    {
        return m_gotNotification;
    }

    const char* GetUserName(uint32_t& len)
    {
        len = m_userNameLen;
        return m_userName;
    }

    const char* GetMessage(uint32_t& len)
    {
        len = m_messageLen;
        return m_message;
    }

    void ClearNotification()
    {
        m_userNameLen = 0;
        m_userName = NULL;
        m_messageLen = 0;
        m_message = NULL;
        m_gotNotification = false;
    }

private:

    API* m_api;
    int m_socket;
    HTTPRequest* m_postRequest;
    HTTPRequest* m_getRequest;
    int32_t m_pendingNotificationsNum;

    char data[256];
    char* m_userName;
    uint32_t m_userNameLen;
    char* m_message;
    uint32_t m_messageLen;
    bool m_gotNotification;

    void FreePostRequest()
    {
        m_api->Free(m_postRequest->requestBody);
        m_api->FreeHTTPRequest(m_postRequest);
        m_postRequest = NULL;
    }

    void FreeGetRequest()
    {
        m_api->FreeHTTPRequest(m_getRequest);
        m_getRequest = NULL;
    }

    void Get()
    {
        m_getRequest = m_api->AllocHTTPRequest();
        if(!m_getRequest)
            return;
        m_getRequest->httpMethod = kHttpMethodGet;
        m_api->sprintf(m_getRequest->url, "http://%s:%u/notification?auth=%x", kServerIP, kServerPort, authKey);
        m_getRequest->responseData = (void*)data;
        m_getRequest->responseDataCapacity = 512; // hahaha, should be 256
        if(!m_api->SendHTTPRequest(m_getRequest))
            FreeGetRequest();
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

    api->printf("111\n");

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


int GameMain(API* api, uint8_t* sdram)
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
    uint32_t authKey = ~0u;

    uint8_t* curSdram = sdram;
    IconsManager iconCache(curSdram, api);
    curSdram = iconCache.Reset();
    bool evictIcon = false;
    uint32_t iconRequestsInFlight = 0;

    GameDesc* games = (GameDesc*)curSdram;
    uint32_t gamesCount = 0;
    curSdram += sizeof(GameDesc) * kMaxGamesCount;

    uint8_t* gameCodeMem = (uint8_t*)api->Malloc(kMaxGameCodeSize);
    uint32_t curGame = ~0u;

    bool touchOnPrevFrame = false;
    uint16_t prevTouchX = 0, prevTouchY = 0;
    float pressDownTime = 0.0f;

    NotificationsCtx notificationsCtx;
    if(!notificationsCtx.Init(api))
        return 1;
    float notificationDrawTimer = 0.0f;
    const float notificationDrawTime = 5.0f;

    float timer = api->time();
    while(1)
    {
        api->SwapFramebuffer();
        api->GetTouchScreenState(&tsState);

        float curTime = api->time();
        float dt = curTime - timer;
        timer = curTime;

        notificationsCtx.Update();

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
            api->FreeHTTPRequest(request);
            request = NULL;

            if(request->succeed)
            {
                char buf[64];
                api->memset(buf, 0, 64);
                api->sprintf(buf, "Start game '%s'", games[curGame].name);
                notificationsCtx.Post(api->GetIPAddress(), buf);
                //ScopedRamExecutionLock make_ram_executable;
                uint32_t baseAddr = 0;
                api->memcpy(&baseAddr, gameCodeMem, 4);
                uint8_t* gameMainAddr = gameCodeMem + baseAddr + 4;
                TGameMain gameMain;
                gameMain = (TGameMain)&gameMainAddr[1];
                gameMain(api, curSdram);
                iconCache.Reset();
            }
            else
            {
                api->printf("Failed to load game code\n");
            }

            curGame = ~0u;
            gamesCount = 0;
            iconCache.ClearGameIcons();
            request = RequestGamesList(api, authKey);
            if(request)
                state = kMainScreenWaitGameList;
            else
                state = kMainScreenReady;
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

        if(notificationsCtx.GotNotification())
        {
            FontInfo fontInfo;
            api->LCD_GetFontInfo(kFont12, &fontInfo);
            api->LCD_SetFont(kFont12);
            api->LCD_SetTextColor(0xffffffff);

            Rect rect(60, 15, 270, 0);

            if(notificationDrawTimer < notificationDrawTime)
            {
                uint32_t userNameLen = 0;
                const char* userName = notificationsCtx.GetUserName(userNameLen);
                uint32_t messageLen = 0;
                const char* message = notificationsCtx.GetMessage(messageLen);

                uint32_t xpos = rect.x;
                uint32_t ypos = rect.y;
                for(uint32_t i = 0; i < userNameLen; i++)
                {
                    api->LCD_DisplayChar(xpos, ypos, userName[i]);
                    xpos += fontInfo.charWidth;
                    if(xpos > rect.x + rect.width)
                    {
                        xpos = rect.x;
                        ypos += fontInfo.charHeight;
                    }
                }
                
                ypos += fontInfo.charHeight;
                xpos = rect.x;
                for(uint32_t i = 0; i < messageLen; i++)
                {
                    api->LCD_DisplayChar(xpos, ypos, message[i]);
                    xpos += fontInfo.charWidth;
                    if(xpos > rect.x + rect.width)
                    {
                        xpos = rect.x;
                        ypos += fontInfo.charHeight;
                    }
                }

                notificationDrawTimer += dt;
            }
            else
            {
                notificationDrawTimer = 0.0f;
                notificationsCtx.ClearNotification();
            }
        }
    }
}

