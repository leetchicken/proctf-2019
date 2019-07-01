#include "../../../hw/api.h"


int GameMain(API* api)
{
    while(1)
    {
        api->printf("hello world!");
        api->sleep(0.1f);
    }
    return 14;
}