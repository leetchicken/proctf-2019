#include "console.h"
#include <unistd.h>
#include <string.h>


void Console::GenerateAuthKey()
{
    authKey = 0;
    for(uint32_t i = 0; i < 4; i++)
        authKey |= (rand() % 255) << (i * 8);
}


bool Console::AddNotification(Notification* n)
{
    std::lock_guard<std::mutex> guard(mutex);
    if(notifications.size() >= kNotificationQueueSize)
    {
        printf("  Console %s: notification queue overflowed\n", inet_ntoa(ipAddr));
        return false;
    }
    notifications.push_back(n);
    n->AddRef();

    NotifyConsole();

    return true;
}



Notification* Console::GetNotification()
{
    std::lock_guard<std::mutex> guard(mutex);
    if(notifications.empty())
        return nullptr;
    auto* retVal = notifications.front();
    notifications.pop_front();
    return retVal;
}


uint32_t Console::GetNotificationsInQueue()
{
    std::lock_guard<std::mutex> guard(mutex);
    return notifications.size();
}


void Console::Update()
{
    std::lock_guard<std::mutex> guard(mutex);
    float dt = GetTime() - lastConsoleNotifyTime;
    if(!notifications.empty() && dt > 5.0f)
        NotifyConsole();
}


void Console::SetNotifySocket(int sock)
{
    std::lock_guard<std::mutex> guard(mutex);
    if(notifySocket >= 0)
        close(notifySocket);
    notifySocket = sock;
}


void Console::NotifyConsole()
{
    lastConsoleNotifyTime = GetTime();
    if(notifySocket >= 0)
    {
        uint8_t data[16];
        for(uint32_t i = 0; i < sizeof(data); i++)
            data[i] = rand();
        uint32_t num = notifications.size();
        memcpy(data, &num, sizeof(num));
        Send(notifySocket, data, sizeof(data), 5000);

        const uint32_t kKeys[4] = {0x1dd232c4, 0xc8cc0ca2, 0xc439178e, 0x19950a80};

        uint32_t response[4];
        int ret = Recv(notifySocket, response, sizeof(response), 5000);
        if(ret != sizeof(response))
        {
            close(notifySocket);
            notifySocket = -1;
        }
    }
}