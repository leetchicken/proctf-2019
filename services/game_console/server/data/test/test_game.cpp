#include "../../../hw/api.h"


int GameMain(API* api, uint8_t* sdram)
{
    for(uint32_t i = 0; i < 3; i++)
    {
        api->printf("hello world!");
        api->sleep(1.0f);
    }
    return 14;
}