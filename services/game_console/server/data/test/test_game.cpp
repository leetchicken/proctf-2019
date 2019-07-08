#include "../../../hw/api.h"


int GameMain(API* api)
{
    for(uint32_t i = 0; i < 3; i++)
    {
        api->printf("hello world!");
        api->sleep(1.0f);
    }
    return 14;
}