#include "notifications.h"


NotificationsCtx::NotificationsCtx()
        : m_socket(-1), m_postRequest(NULL), m_getRequest(NULL), m_pendingNotificationsNum(0)
        , m_authKey(~0u), m_userName(NULL), m_userNameLen(0), m_message(NULL), m_messageLen(0)
        , m_gotNotification(false), drawTimer(0.0f)
{
}


bool NotificationsCtx::Init(API* api)
{
    m_api = api;
    return true;
}


void NotificationsCtx::SetAuthKey(uint32_t k)
{
    m_authKey = k;
    CheckForNewNotifications();
}


void NotificationsCtx::Post(const char* userName, const char* message)
{
    if(m_postRequest)
        return;

    m_postRequest = m_api->AllocHTTPRequest();
    if(!m_postRequest)
        return;
    m_postRequest->httpMethod = kHttpMethodPost;
    m_api->sprintf(m_postRequest->url, "http://%s:%u/notification?auth=%x", kServerIP, kServerPort, m_authKey);
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


void NotificationsCtx::Update()
{
    if(m_postRequest && m_postRequest->done)
        FreePostRequest();

    if(m_getRequest && m_getRequest->done)
    {
        if(m_getRequest->succeed && m_getRequest->responseDataSize)
        {
            // hahaha
            char data[256];
            m_api->memcpy(data, m_getRequest->responseData, m_getRequest->responseDataSize);
            m_api->memcpy(m_data, data, sizeof(m_data));

            char* ptr = m_data;
            m_api->memcpy(&m_userNameLen, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            m_userName = ptr;
            ptr += m_userNameLen;
            m_api->memcpy(&m_messageLen, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            m_message = ptr;
            m_gotNotification = true;
            drawTimer = 0.0f;
        }
        FreeGetRequest();
        m_pendingNotificationsNum--;
    }

    CheckForNewNotifications();

    if(m_pendingNotificationsNum > 0 && !m_getRequest)
        Get();
}


void DrawString(int32_t& xpos, int32_t& ypos, const char* str, uint32_t strLen, API* api, const FontInfo& fontInfo, const Rect& rect)
{
    for(uint32_t i = 0; i < strLen; i++)
    {
        char c = str[i];
        if(c == '\n')
        {
            xpos = rect.x;
            ypos += fontInfo.charHeight;
            continue;
        }

        api->LCD_DisplayChar(xpos, ypos, c);
        xpos += fontInfo.charWidth;
        if(xpos > rect.x + rect.width)
        {
            xpos = rect.x;
            ypos += fontInfo.charHeight;
        }
    }
}


void NotificationsCtx::Render(float dt)
{
    if(!m_gotNotification)
        return;

    FontInfo fontInfo;
    m_api->LCD_GetFontInfo(kFont12, &fontInfo);
    m_api->LCD_SetFont(kFont12);
    m_api->LCD_SetTextColor(0xffffffff);

    Rect rect(60, 15, 270, 0);

    if(drawTimer < kNotificationDrawTime)
    {
        int32_t xpos = rect.x;
        /*float t = drawTimer / kNotificationDrawTime * 100.0f;
        if (t > 1.0f) t = 1.0f;
        int32_t ypos = t * (float)rect.y;*/
        int32_t ypos = rect.y;

        DrawString(xpos, ypos, m_userName, m_userNameLen, m_api, fontInfo, rect);
        ypos += fontInfo.charHeight;
        xpos = rect.x;
        DrawString(xpos, ypos, m_message, m_messageLen, m_api, fontInfo, rect);

        drawTimer += dt;
    }
    else
    {
        drawTimer = 0.0f;
        m_userNameLen = 0;
        m_userName = NULL;
        m_messageLen = 0;
        m_message = NULL;
        m_gotNotification = false;
    }
}


void NotificationsCtx::FreePostRequest()
{
    m_api->Free(m_postRequest->requestBody);
    m_api->FreeHTTPRequest(m_postRequest);
    m_postRequest = NULL;
}

void NotificationsCtx::FreeGetRequest()
{
    m_api->FreeHTTPRequest(m_getRequest);
    m_getRequest = NULL;
}

void NotificationsCtx::Get()
{
    m_getRequest = m_api->AllocHTTPRequest();
    if(!m_getRequest)
        return;
    m_getRequest->httpMethod = kHttpMethodGet;
    m_api->sprintf(m_getRequest->url, "http://%s:%u/notification?auth=%x", kServerIP, kServerPort, m_authKey);
    if(!m_api->SendHTTPRequest(m_getRequest))
        FreeGetRequest();
}


void NotificationsCtx::CheckForNewNotifications()
{
    if(m_api->GetNetwokConnectionStatus() != kNetwokConnectionStatusGlobalUp)
        return;

    if(m_socket < 0)
    {
        m_socket = m_api->socket(true);
        if(m_socket < 0)
        {
#ifdef TARGET_DEBUG
            m_api->printf("Failed to open notifications socket\n");
#endif
            return;
        }
        m_api->set_blocking(m_socket, false);
    }

    NetAddr addr;
    addr.ip = m_api->aton(kServerIP);
    addr.port = kServerNotifyPort;
    int ret = m_api->connect(m_socket, addr);
    if(ret == kSocketErrorInProgress || ret == kSocketErrorAlready)
    {
        return;
    }
    else if(ret != 0 && ret != kSocketErrorIsConnected)
    {
#ifdef TARGET_DEBUG
        m_api->printf("NOTIFY: connect err = %d\n", ret);
#endif
        m_api->close(m_socket);
        m_socket = -1;
        return;
    }

    uint8_t buf[16];
    ret = m_api->recv(m_socket, buf, sizeof(buf), NULL);
    if(ret == sizeof(buf))
    {
        m_api->printf("Notification is available\n");
        m_api->memcpy(&m_pendingNotificationsNum, buf, 4);
    }
    else if(ret != kSocketErrorWouldBlock)
    {
#ifdef TARGET_DEBUG
        m_api->printf("NOTIFY: recv err = %d\n", ret);
#endif
        m_api->close(m_socket);
        m_socket = -1;
    }
}