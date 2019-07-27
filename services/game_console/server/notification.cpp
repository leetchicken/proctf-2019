#include "notification.h"
#include <string.h>
#include <stdlib.h>


Notification::Notification(const char* userName, const char* message)
{
    uint32_t userNameLen = strlen(userName);
    uint32_t messageLen = strlen(message);

    notificationLen = sizeof(uint32_t) + userNameLen + sizeof(uint32_t) + messageLen;
    notification = (char*)malloc(notificationLen);

    char* ptr = notification;
    memcpy(ptr, &userNameLen, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, userName, userNameLen);
    ptr += userNameLen;

    memcpy(ptr, &messageLen, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    memcpy(ptr, message, messageLen);
}


Notification::Notification(void* data, uint32_t dataSize)
{
    notification = (char*)malloc(dataSize);
    memcpy(notification, data, dataSize);
    notificationLen = dataSize;
}


Notification::~Notification()
{
    if(notification)
        free(notification);
    notification = nullptr;
    notificationLen = 0;
}


void Notification::AddRef() const
{
    __sync_add_and_fetch(&refCount, 1);
}


uint32_t Notification::Release() const
{
    uint32_t count = __sync_sub_and_fetch(&refCount, 1);
    if ((int32_t)count <= 0)
        delete this;
    return count;
}


bool Notification::Validate(void* data, uint32_t dataSize)
{
    if(dataSize < sizeof(uint32_t))
        return false;

    char* ptr = (char*)data;
    uint32_t userNameLen = 0, messageLen = 0;
    memcpy(&userNameLen, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    ptr += userNameLen;

    if(2 * sizeof(uint32_t) + userNameLen > dataSize)
        return false;

    memcpy(&messageLen, ptr, sizeof(uint32_t));
    
    return 2 * sizeof(uint32_t) + userNameLen + messageLen == dataSize;
}