#pragma once
#include <mutex>
#include <list>
#include "misc.h"
#include "notification.h"

struct User
{
    std::string name;
    std::string password;

    IPAddr ipAddr;
    float lastUserNotifyTime = 0.0f;
    AuthKey authKey = ~0u;
    static const uint32_t kNotificationQueueSize = 32;

    void GenerateAuthKey();

    bool AddNotification(Notification* n);
    Notification* GetNotification();
    uint32_t GetNotificationsInQueue();
    void Update();
    void SetNotifySocket(int sock);

private:
    std::mutex mutex;
    std::list<Notification*> notifications;
    int notifySocket = -1;

    void NotifyUser();
};