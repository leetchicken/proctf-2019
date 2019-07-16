#include "../../../hw/api.h"


struct Context
{
    uint32_t color;
};


inline void* operator new(size_t, void* __p) { return __p; }


void* GameInit(API* api, uint8_t* sdram)
{
    void* mem = api->Malloc(sizeof(Context));
    Context* ctx = new(mem) Context();
    ctx->color = 0;
    return (void*)ctx;
}


bool GameUpdate(API* api, void* ctxVoid)
{
    Context* ctx = (Context*)ctxVoid;
    if(api->GetButtonState())
    {
        api->Free(ctx);
        return false;
    }

    api->LCD_Clear(0xff000000 | (ctx->color << 16) | (ctx->color << 8));
    ctx->color = (ctx->color + 1) % 128;

    char buf[128];
    api->sprintf(buf, "%u", ctx->color);

    api->LCD_SetFont(kFont12);
    api->LCD_SetTextColor(0xffffffff);
    api->LCD_DisplayStringAt(5, 5, buf, kTextAlignNone);

    return true;
}