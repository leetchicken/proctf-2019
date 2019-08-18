#pragma once
#include <mutex>
#include <list>
#include "misc.h"
#include "notification.h"

struct User
{
    IPAddr ipAddr;
    static const uint32_t kNotificationQueueSize = 32;

    User() = delete;
    User(const std::string& name, const std::string& password);

    void SetAuthKey(uint32_t authKey);
    uint32_t GetAuthKey() const;
    uint32_t GenerateAuthKey();

    const std::string& GetName() const;

    const std::string& GetPassword() const;
    void ChangePassword(const std::string& newPassword);

    bool AddNotification(Notification* n);
    Notification* GetNotification();
    uint32_t GetNotificationsInQueue();
    void Update();
    void SetNotifySocket(int sock);

    void DumpStats(std::string& out, IPAddr hwConsoleIp) const;

private:
    mutable std::mutex mutex;
    std::string name;
    std::string password;
    AuthKey authKey = ~0u;
    std::list<Notification*> notifications;
    int notifySocket = -1;
    float lastUserNotifyTime = 0.0f;

    void NotifyUser();
};