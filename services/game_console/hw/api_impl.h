#pragma once
#include "api.h"
#include "mbed.h"

class APIImpl : public API
{
public:
    void printf(const char* str)
    {
        ::printf("%s\n", str);
    }

    void sleep(float t)
    {
        wait(t);
    }
};
