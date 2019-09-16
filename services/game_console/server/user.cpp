#include "user.h"
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <thread>
#include <vector>
#include "team.h"
#include "log.h"


struct UsersStorageRecord
{
    IPAddr ip;
    AuthKey authKey;
    char userName[kMaxUserNameLen];
    char password[kMaxPasswordLen];
};
static const char* kUsersStorageFileName = "data/users.dat";


static std::mutex GUsersGuard;
static std::unordered_map<std::string, User*> GUsers;
static std::unordered_map<AuthKey, User*> GAuthUsers;
static std::thread GNetworkThread;


User::User(const std::string& name, const std::string& password, Team* team)
    : m_team(team), m_name(name), m_password(password)
{

}


Team* User::GetTeam()
{
    return m_team;
}


const std::string& User::GetName() const
{
    return m_name;
}


IPAddr User::GetIPAddr() const
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    return m_ipAddr;
}


bool User::AddNotification(Notification* n)
{
	std::lock_guard<std::mutex> guard(m_notificationMutex);
    if(m_notifications.size() >= kNotificationQueueSize)
    {
        Log("  Console %s: notification queue overflowed\n", inet_ntoa(m_ipAddr));
        return false;
    }
    m_notifications.push_back(n);
    n->AddRef();

    m_lastUserNotifyTime = 0.0f;

    return true;
}


Notification* User::GetNotification()
{
	std::lock_guard<std::mutex> guard(m_notificationMutex);
    if(m_notifications.empty())
        return nullptr;
    auto* retVal = m_notifications.front();
    m_notifications.pop_front();
    return retVal;
}


uint32_t User::GetNotificationsInQueue()
{
	std::lock_guard<std::mutex> guard(m_notificationMutex);
    return m_notifications.size();
}


void User::SetSocket(Socket* socket)
{
    if(m_socket)
    {
        Log("NOTIFY: close previous connection\n");
        m_socket->Close();
    }

    m_socket = socket;

    m_socket->state = Socket::kStateReady;
    m_socket->recvBufferSize = 0;
    m_socket->recvBufferOffset = 0;
    m_socket->sendBufferSize = 0;
    m_socket->sendBufferOffset = 0;

    m_socket->updateCallback = [this](Socket* sock)
    {
        if(sock->state == Socket::kStateReady)
        {
            std::lock_guard<std::mutex> guard(m_notificationMutex);
            float dt = GetTime() - m_lastUserNotifyTime;
            if(!m_notifications.empty() && dt > 5.0f)
            {
                sock->sendBufferSize = 16;
                for(uint32_t i = 0; i < sock->sendBufferSize; i++)
                    sock->sendBuffer[i] = rand();
                uint32_t num = m_notifications.size();
                memcpy(sock->sendBuffer, &num, sizeof(num));
                sock->sendBufferOffset = 0;

                sock->state = Socket::kStateSending;
                sock->lastTouchTime = GetTime();
                m_lastUserNotifyTime = GetTime();

                Log("Notify user '%s'\n", m_name.c_str());
            }
        }
        else if(sock->state == Socket::kStateSending)
        {
            if(sock->sendBufferOffset == sock->sendBufferSize)
            {
                sock->state = Socket::kStateReceiving;
                sock->recvBufferSize = 16;
                sock->recvBufferOffset = 0;
            }
        }
        else if(sock->state == Socket::kStateReceiving)
        {
            if(sock->recvBufferOffset == sock->recvBufferSize)
                sock->state = Socket::kStateReady;
        }
    };

    m_socket->timeoutCallback = [this](Socket* socket){
        m_socket->Close();
        m_socket = nullptr;
    };
    m_socket->closedByPeerCallback = m_socket->timeoutCallback;
    m_socket->errorCallback = m_socket->timeoutCallback;
}


Socket* User::GetSocket()
{
    return m_socket;
}


void User::DumpStats(std::string& out, IPAddr hwConsoleIp) const
{
    char buf[512];

    sprintf(buf, "    Password: %s\n", m_password.c_str());
    out.append(buf);

    sprintf(buf, "    IP: %s\n", inet_ntoa(m_ipAddr));
    out.append(buf);

    bool isHw = m_ipAddr == hwConsoleIp;
    sprintf(buf, "    Is HW: %s\n", isHw ? "yes" : "no");
    out.append(buf);

    sprintf(buf, "    Notifications in queue: %u\n", m_notifications.size());
    out.append(buf);

    sprintf(buf, "    Last user notify time: %f\n", m_lastUserNotifyTime);
    out.append(buf);

    sprintf(buf, "    Auth key: %x\n\n", m_authKey);
    out.append(buf);
}


static uint32_t GenerateAuthKey()
{
    uint32_t authKey = 0;
    for(uint32_t i = 0; i < 4; i++)
        authKey |= (rand() % 255) << (i * 8);
    return authKey;
}


EUserErrorCodes User::Add(const std::string& name, const std::string& password, Team* team)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    if(name.length() > kMaxUserNameLen || password.length() > kMaxPasswordLen)
    {
        Log("  ERROR: too long\n");
        return kUserErrorTooLong;
    }

    auto iter = GUsers.find(name);
    if(iter != GUsers.end())
    {
        Log("  ERROR: already exists\n");
        return kUserErrorAlreadyExists;
    }

    User* user = new User(name, password, team);
    GUsers[name] = user;

    team->m_usersCount++;

    Log("  new user\n");

    DumpStorage();

    return kUserErrorOk;
}


User* User::Get(const std::string& name)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto iter = GUsers.find(name);
    if(iter == GUsers.end())
        return nullptr;
    return iter->second;
}


EUserErrorCodes User::Authorize(const std::string& name, const std::string& password, IPAddr ipAddr, AuthKey& authKey)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto usersIter = GUsers.find(name);
    if(usersIter == GUsers.end())
    {
        Log("  ERROR: invalid user name\n");
        return kUserErrorInvalidCredentials;
    }

    User* user = usersIter->second;    

    if(user->m_password != password)
    {
        Log("  ERROR: invalid password\n");
        return kUserErrorInvalidCredentials;
    }

    authKey = user->m_authKey;
    auto authUsersIter = GAuthUsers.find(authKey);
    if(authUsersIter != GAuthUsers.end())
    {
        Log("  reauth detected, prev auth key: %x\n", authKey);
        User* user2 = authUsersIter->second;
        if(user != user2)
        {
            Log("  CRITICAL ERROR, mismatched user data '%s':%u '%s':%u\n", user->m_name.c_str(), authKey, user2->m_name.c_str(), user2->m_authKey);
            exit(1);
        }
        GAuthUsers.erase(authUsersIter);
    }

    authKey = GenerateAuthKey();
    const uint32_t kMaxTriesCount = 5;
    uint32_t triesCount = 0;
    while(GAuthUsers.find(authKey) != GAuthUsers.end())
    {
        if(triesCount >= kMaxTriesCount)
        {
            Log("  CRITICAL ERROR, dublicate user was found, auth key: %x\n", authKey);
            exit(1);
        }

        authKey = GenerateAuthKey();
        triesCount++;
    }

    Log("  auth key: %x\n", authKey);
    GAuthUsers[authKey] = user;
    user->m_authKey = authKey;
    user->m_ipAddr = ipAddr;

    DumpStorage();

    return kUserErrorOk;
}


EUserErrorCodes User::ChangePassword(const std::string& userName, const std::string& newPassword)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    auto iter = GUsers.find(userName);
    if(iter == GUsers.end())
    {
        Log("  ERROR: invalid user name\n");
        return kUserErrorInvalidCredentials;
    }
    
    User* user = iter->second;
    ChangePassword(user, newPassword);

    return kUserErrorOk;
}


EUserErrorCodes User::ChangePassword(AuthKey authKey, const std::string& newPassword, Team* team)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    if(authKey == kInvalidAuthKey)
        return kUserErrorInvalidAuthKey;

    auto iter = GAuthUsers.find(authKey);
    if(iter == GAuthUsers.end())
        return kUserErrorUnauthorized;
    
    User* user = iter->second;

    if(user->GetTeam() != team)
    {
        Log("  Team '%s' tries to change password for '%s' from '%s' team\n", team->name.c_str(), user->GetName().c_str(), user->GetTeam()->name.c_str());
        return kUserErrorForbidden;
    }

    ChangePassword(user, newPassword);

    return kUserErrorOk;
}


User* User::Get(AuthKey authKey)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    auto iter = GAuthUsers.find(authKey);
    if(iter == GAuthUsers.end())
        return nullptr;
    return iter->second;
}


void User::GetUsers(std::vector<User*>& users)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    users.reserve(GUsers.size());
    for(auto& iter : GUsers)
        users.push_back(iter.second);
}


void User::BroadcastNotification(Notification* n, User* sourceUser)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);
    for(auto& iter : GUsers)
    {
        User* user = iter.second;
        if(user == sourceUser)
            continue;
        user->AddNotification(n);
    }
}


static bool AcceptConnection(Socket* socket, const sockaddr_in& clientAddr)
{
    Log("NOTIFY: Accepted connection from: %s\n", inet_ntoa(clientAddr.sin_addr));

    socket->recvBufferSize = sizeof(AuthKey);
    socket->recvBufferOffset = 0;
    socket->state = Socket::kStateReceiving;

    socket->updateCallback = [](Socket* socket){
        if(socket->recvBufferOffset == socket->recvBufferSize)
        {
            AuthKey authKey;
            memcpy(&authKey, socket->recvBuffer, 4);
            
            User* user = User::Get(authKey);
            if(!user)
            {
                Log("NOTIFY: Unknown auth key %x, close connection\n", authKey);
                socket->Close();
                return;
            }

            sockaddr_in addr;
            socklen_t addrLen = sizeof(addr);
            int ret = getpeername(socket->fd, (sockaddr*)&addr, &addrLen);
            if(ret < 0)
            {
                Log("NOTIFY: getpeername failed: %s\n", strerror(errno));
                socket->Close();
                return;
            }

            if(user->GetIPAddr() != addr.sin_addr.s_addr)
            {
                Log("NOTIFY: mismatched IP addresses\n");
                socket->Close();
                return;
            }

            user->SetSocket(socket);
            Log("NOTIFY: OK\n");
        }
    };

    socket->timeoutCallback = [](Socket* socket){
        Log("NOTIFY: socket timeout\n");
        socket->Close();
    };
    socket->closedByPeerCallback = [](Socket* socket){
        Log("NOTIFY: connection closed by peer\n");
        socket->Close();
    };
    socket->errorCallback = [](Socket* socket){
        Log("NOTIFY: socket error: %s\n", strerror(errno));
        socket->Close();
    };

    return true;
}


void User::Start()
{
    ReadStorage();
    GNetworkThread = std::thread(NetworkManager, AcceptConnection, kNotifyPort);
}


void User::DumpStorage()
{
    FILE* f = fopen(kUsersStorageFileName, "w");
    if(!f)
    {
        Log("Failed to open users storage\n");
        return;
    }

    for(auto iter : GUsers)
    {
        UsersStorageRecord record;
        User* u = iter.second;
        memset(&record, 0, sizeof(record));
        record.authKey = u->m_authKey;
        record.ip = u->m_ipAddr;
        auto& name = u->m_name;
        auto& password = u->m_password;
        memcpy(record.userName, name.c_str(), name.length());
        memcpy(record.password, password.c_str(), password.length());
        fwrite(&record, sizeof(record), 1, f);
    }

    fclose(f);
}


void User::ReadStorage()
{
    const uint32_t kRecordSize = sizeof(UsersStorageRecord);

    FILE* storage = fopen(kUsersStorageFileName, "r");
	if(storage)
	{
		fseek(storage, 0, SEEK_END);
		size_t fileSize = ftell(storage);
		fseek(storage, 0, SEEK_SET);

        bool error = false;
        if((fileSize % kRecordSize) == 0)
        {
            uint32_t recordsNum = fileSize / kRecordSize;
            for(uint32_t i = 0; i < recordsNum; i++)
            {
                UsersStorageRecord record;
				if(fread(&record, kRecordSize, 1, storage) != 1)
                {
                    error = true;
                    Log("Failed to read consoles storage\n");
                    break;
                }

                in_addr addr;
                addr.s_addr = record.ip;
                Team* team = FindTeam(addr);
                if(!team)
                    continue;

                User* user = new User(record.userName, record.password, team);
                GUsers[record.userName] = user;
                user->m_authKey = record.authKey;
                user->m_ipAddr = record.ip;
                GAuthUsers[record.authKey] = user;

                team->m_usersCount++;
            }
        }
        else
        {
            Log("Consoles storage is corrupted\n");
        }

        if(!error)
            Log("Consoles storage has been read succefully\n");

        fclose(storage);
    }
}


void User::ChangePassword(User* user, const std::string& newPassword)
{
    uint32_t authKey = user->m_authKey;
    GAuthUsers.erase(authKey);

    user->m_password = newPassword;
    user->m_authKey = kInvalidAuthKey;

    DumpStorage();
}
