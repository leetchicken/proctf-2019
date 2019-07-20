#include "../../../hw/api.h"


struct Context
{
    float prevTime;

    Context()
    {}

    bool Update(API* api);
};


inline void* operator new(size_t, void* __p) { return __p; }


void* GameInit(API* api, uint8_t* sdram)
{
    void* mem = api->Malloc(sizeof(Context));
    Context* ctx = new(mem) Context();
    return (void*)ctx;
}


bool GameUpdate(API* api, void* ctxVoid)
{
    Context* ctx = (Context*)ctxVoid;
    return ctx->Update(api);
}


bool Context::Update(API* api)
{
    if(api->GetButtonState())
    {
        api->Free(this);
        return false;
    }

    float dt = api->time() - prevTime;
    prevTime = api->time();

    api->LCD_Clear(0xff000000);

    char buf[128];
    api->LCD_SetFont(kFont12);
    api->LCD_SetTextColor(0xffffffff);
    api->sprintf(buf, "Frame time: %8.2f ms", api->floatToDouble(dt * 1000.0f));
    api->LCD_DisplayStringAt(5, 5, buf, kTextAlignNone);
    api->sprintf(buf, "Press button to exit");
    api->LCD_DisplayStringAt(5, 17, buf, kTextAlignNone);

    return true;
}