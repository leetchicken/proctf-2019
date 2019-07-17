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

    m_socket = api->socket(false);
    if(m_socket < 0)
    {
        api->printf("Failed to open notifications socket, can not continue\n");
        return false;
    }
    api->set_blocking(m_socket, false);

    return true;
}


void NotificationsCtx::SetNotifyPort(uint16_t notifyPort)
{
    m_api->bind(m_socket, 0, notifyPort);
}


void NotificationsCtx::SetAuthKey(uint32_t k)
{
    m_authKey = k;
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
        int32_t ypos = rect.y;
        for(uint32_t i = 0; i < m_userNameLen; i++)
        {
            m_api->LCD_DisplayChar(xpos, ypos, m_userName[i]);
            xpos += fontInfo.charWidth;
            if(xpos > rect.x + rect.width)
            {
                xpos = rect.x;
                ypos += fontInfo.charHeight;
            }
        }
        
        ypos += fontInfo.charHeight;
        xpos = rect.x;
        for(uint32_t i = 0; i < m_messageLen; i++)
        {
            m_api->LCD_DisplayChar(xpos, ypos, m_message[i]);
            xpos += fontInfo.charWidth;
            if(xpos > rect.x + rect.width)
            {
                xpos = rect.x;
                ypos += fontInfo.charHeight;
            }
        }

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
    m_getRequest->responseData = (void*)data;
    m_getRequest->responseDataCapacity = 512; // hahaha, should be 256
    if(!m_api->SendHTTPRequest(m_getRequest))
        FreeGetRequest();
}