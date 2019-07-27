#pragma once
#include <stdint.h>


struct Notification
{
    char* notification = nullptr;
    uint32_t notificationLen = 0;

    Notification(const char* userName, const char* message);
    Notification(void* data, uint32_t dataSize);
    ~Notification();

    void AddRef() const;
    uint32_t Release() const;

    static bool Validate(void* data, uint32_t dataSize);

private:
    mutable uint32_t refCount = 0;
};