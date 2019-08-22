#include "user.h"
#include <unistd.h>
#include <string.h>
#include <unordered_map>
#include <thread>
#include "team.h"


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
static std::thread GUpdateThread;
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
        printf("  Console %s: notification queue overflowed\n", inet_ntoa(m_ipAddr));
        return false;
    }
    m_notifications.push_back(n);
    n->AddRef();

    NotifyUser();

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


void User::Update()
{
    float dt = GetTime() - m_lastUserNotifyTime;
    if(!m_notifications.empty() && dt > 5.0f)
    {
        std::lock_guard<std::mutex> guard(m_notificationMutex);
        NotifyUser();
    }
}


void User::SetNotifySocket(int sock)
{
    std::lock_guard<std::mutex> guard(m_notificationMutex);
    if(m_notifySocket >= 0)
        close(m_notifySocket);
    m_notifySocket = sock;
}


void User::NotifyUser()
{
    m_lastUserNotifyTime = GetTime();
    if(m_notifySocket >= 0)
    {
        uint8_t data[16];
        for(uint32_t i = 0; i < sizeof(data); i++)
            data[i] = rand();
        uint32_t num = m_notifications.size();
        memcpy(data, &num, sizeof(num));
        Send(m_notifySocket, data, sizeof(data), 5000);

        const uint32_t kKeys[4] = {0x1dd232c4, 0xc8cc0ca2, 0xc439178e, 0x19950a80};

        uint32_t response[4];
        int ret = Recv(m_notifySocket, response, sizeof(response), 5000);
        if(ret != sizeof(response))
        {
            close(m_notifySocket);
            m_notifySocket = -1;
        }
    }
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


static void UpdateThread()
{
    while(1)
    {
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            for(auto& iter : GUsers)
            {
                User* user = iter.second;
                user->Update();
            }
        }
        sleep(1);
    }
}


static void NetworkThread()
{
    int listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock < 0)
    {
        printf("NOTIFY: socket failed\n");
        return;
    }

    int opt_val = 1;
	setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof(opt_val));

	sockaddr_in addr;
	memset(&addr, 0, sizeof(sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(kNotifyPort);
	inet_aton("0.0.0.0", &addr.sin_addr);
    int ret = bind(listenSock, (sockaddr*)&addr, sizeof(addr));
	if(ret < 0)
	{
		printf("NOTIFY: bind failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

	ret = listen(listenSock, 128);
    if(ret < 0)
	{
		printf("NOTIFY: listen failed: %s\n", strerror(errno));
		close(listenSock);
		return;
	}

    while(1)
    {
        sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);
        int newSocket = accept4(listenSock, (sockaddr*)&clientAddr, &addrLen, SOCK_NONBLOCK);
        if(newSocket < 0)
        {
            printf("NOTIFY: accept failed: %s\n", strerror(errno));
            sleep(1);
            continue;
        }

        printf("NOTIFY: Accepted connection from: %s\n", inet_ntoa(clientAddr.sin_addr));

        Team* team = FindTeam(clientAddr.sin_addr);
        if(!team)
        {
            printf("NOTIFY: unknown network\n");
            close(newSocket);
            continue;
        }

        User* user = nullptr;
        {
            std::lock_guard<std::mutex> guard(GUsersGuard);
            IPAddr ipAddr = clientAddr.sin_addr.s_addr;
            for(auto& iter : GUsers)
            {
                if(iter.second->GetIPAddr() == ipAddr)
                {
                    user = iter.second;
                    break;
                }
            }
        }

        if(!user)
        {
            printf("NOTIFY: Unknown ip address\n");
            close(newSocket);
            continue;
        }

        printf("NOTIFY: OK\n");
        user->SetNotifySocket(newSocket);
    }
}


EUserErrorCodes User::Add(const std::string& name, const std::string& password, Team* team)
{
    std::lock_guard<std::mutex> guard(GUsersGuard);

    if(name.length() > kMaxUserNameLen || password.length() > kMaxPasswordLen)
    {
        printf("  ERROR: too long\n");
        return kUserErrorTooLong;
    }

    auto iter = GUsers.find(name);
    if(iter != GUsers.end())
    {
        printf("  ERROR: already exists\n");
        return kUserErrorAlreadyExists;
    }

    User* user = new User(name, password, team);
    GUsers[name] = user;

    printf("  new user\n");

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
        printf("  ERROR: invalid user name\n");
        return kUserErrorInvalidCredentials;
    }

    User* user = usersIter->second;    

    if(user->m_password != password)
    {
        printf("  ERROR: invalid password\n");
        return kUserErrorInvalidCredentials;
    }

    authKey = user->m_authKey;
    auto authUsersIter = GAuthUsers.find(authKey);
    if(authUsersIter != GAuthUsers.end())
    {
        printf("  reauth detected, prev auth key: %x\n", authKey);
        User* user2 = authUsersIter->second;
        if(user != user2)
        {
            printf("  CRITICAL ERROR, mismatched user data '%s':%u '%s':%u\n", user->m_name.c_str(), authKey, user2->m_name.c_str(), user2->m_authKey);
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
            printf("  CRITICAL ERROR, dublicate user was found, auth key: %x\n", authKey);
            exit(1);
        }

        authKey = GenerateAuthKey();
        triesCount++;
    }

    printf("  auth key: %x\n", authKey);
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
        printf("  ERROR: invalid user name\n");
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
        printf("  Team '%s' tries to change password for '%s' from '%s' team\n", team->name.c_str(), user->GetName().c_str(), user->GetTeam()->name.c_str());
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


void User::Start()
{
    ReadStorage();
    GUpdateThread = std::thread(UpdateThread);
    GNetworkThread = std::thread(NetworkThread);
}


void User::DumpStorage()
{
    FILE* f = fopen(kUsersStorageFileName, "w");
    if(!f)
    {
        printf("Failed to open users storage\n");
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
                    printf("Failed to read consoles storage\n");
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
            }
        }
        else
        {
            printf("Consoles storage is corrupted\n");
        }

        if(!error)
            printf("Consoles storage has been read succefully\n");

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