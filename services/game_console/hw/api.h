#pragma once

class API
{
public:
    virtual void printf(const char* str) = 0;
    virtual void sleep(float t) = 0;
};