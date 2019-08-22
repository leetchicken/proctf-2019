#pragma once
#include <mutex>
#include <list>
#include "misc.h"
#include "notification.h"
#include "team.h"


enum EUserErrorCodes
{
    kUserErrorOk = 0,
    kUserErrorTooLong,
    kUserErrorAlreadyExists,
    kUserErrorInvalidCredentials,
    kUserErrorInvalidAuthKey,
    kUserErrorUnauthorized,
    kUserErrorForbidden,
};


struct User
{
    static const uint32_t kNotificationQueueSize = 32;

    enum ESocketState
    {
        kSocketStateClosed = 0,
        kSocketStateReady,
        kSocketStateSending,
        kSocketStateReceiving
    };

    struct Socket
    {
        ESocketState state = kSocketStateClosed;
        int fd = -1;
        uint8_t sendBuffer[16];
        uint32_t sendBufferSize = 0;
        uint32_t sendBufferOffset = 0;
        uint8_t recvBuffer[16];
        uint32_t recvBufferSize = 0;
        uint32_t recvBufferOffset = 0;
        float lastTouchTime = 0.0f;

        int Send();
        int Recv();
    };

    User() = delete;
    User(const std::string& name, const std::string& password, Team* team);

    Team* GetTeam();
    const std::string& GetName() const;
    IPAddr GetIPAddr() const;

    bool AddNotification(Notification* n);
    Notification* GetNotification();
    uint32_t GetNotificationsInQueue();

    void SetSocket(int sock);
    Socket& GetSocket();
    bool Update();

    void DumpStats(std::string& out, IPAddr hwConsoleIp) const;

    static EUserErrorCodes Add(const std::string& name, const std::string& password, Team* team);
    static User* Get(const std::string& name);
    static EUserErrorCodes Authorize(const std::string& name, const std::string& password, IPAddr ipAddr, AuthKey& authKey);
    static EUserErrorCodes ChangePassword(const std::string& userName, const std::string& newPassword);
    static EUserErrorCodes ChangePassword(AuthKey authKey, const std::string& newPassword, Team* team);
    static User* Get(AuthKey authKey);
    static void GetUsers(std::vector<User*>& users);
    static void BroadcastNotification(Notification* n, User* sourceUser);
    static void Start();

private:
    Team* m_team = nullptr;
    std::string m_name;

    // guarded by GUsersGuard
    std::string m_password;
    IPAddr m_ipAddr = ~0u;
    AuthKey m_authKey = kInvalidAuthKey;

    mutable std::mutex m_notificationMutex;
    std::list<Notification*> m_notifications;
    Socket m_socket;
    float m_lastUserNotifyTime = 0.0f;

    void NotifyUser();

    static void DumpStorage();
    static void ReadStorage();
    static void ChangePassword(User* user, const std::string& newPassword);
    static void NetworkThread();
};
