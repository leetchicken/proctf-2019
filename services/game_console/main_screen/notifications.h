#pragma once
#include "../hw/api.h"
#include "constants.h"


struct NotificationsCtx
{
    NotificationsCtx();
    bool Init(API* api);
    void SetAuthKey(uint32_t k);
    void Post(const char* userName, const char* message);
    void Update();
    void Render(float dt);

private:

    API* m_api;
    int m_socket;
    HTTPRequest* m_postRequest;
    HTTPRequest* m_getRequest;
    int32_t m_pendingNotificationsNum;
    float m_lastConnectTime;
    bool m_isConnected;

    uint32_t m_authKey;

    char m_data[256];
    char* m_userName;
    uint32_t m_userNameLen;
    char* m_message;
    uint32_t m_messageLen;
    bool m_gotNotification;

    float drawTimer;

    void FreePostRequest();
    void FreeGetRequest();
    void Get();
    void CheckForNewNotifications();
};