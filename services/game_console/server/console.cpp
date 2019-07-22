#include "console.h"
#include <unistd.h>


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
    if(notifySocket >= 0)
        close(notifySocket);
    notifySocket = sock;
}


void Console::NotifyConsole()
{
    lastConsoleNotifyTime = GetTime();
    if(notifySocket >= 0)
    {
        char randomData[16];
        send(notifySocket, randomData, sizeof(randomData), 0);
    }
}