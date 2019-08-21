#include "user.h"
#include <unistd.h>
#include <string.h>


User::User(const std::string& name, const std::string& password, Team* team)
    : m_name(name), m_password(password), m_team(team)
{

}


Team* User::GetTeam()
{
    return m_team;
}


void User::SetAuthKey(uint32_t k)
{
    m_authKey = k;
}


uint32_t User::GetAuthKey() const
{
    return m_authKey;
}


uint32_t User::GenerateAuthKey()
{
    m_authKey = 0;
    for(uint32_t i = 0; i < 4; i++)
        m_authKey |= (rand() % 255) << (i * 8);
    return m_authKey;
}


const std::string& User::GetName() const
{
    return m_name;
}


const std::string& User::GetPassword() const
{
    return m_password;
}


void User::ChangePassword(const std::string& newPassword)
{
    m_password = newPassword;
    m_authKey = kInvalidAuthKey;
}


void User::SetIPAddr(IPAddr ipAddr)
{
    m_ipAddr = ipAddr;
}


IPAddr User::GetIPAddr() const
{
    return m_ipAddr;
}


bool User::AddNotification(Notification* n)
{
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
    if(m_notifications.empty())
        return nullptr;
    auto* retVal = m_notifications.front();
    m_notifications.pop_front();
    return retVal;
}


uint32_t User::GetNotificationsInQueue()
{
    return m_notifications.size();
}


void User::Update()
{
    float dt = GetTime() - m_lastUserNotifyTime;
    if(!m_notifications.empty() && dt > 5.0f)
        NotifyUser();
}


void User::SetNotifySocket(int sock)
{
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