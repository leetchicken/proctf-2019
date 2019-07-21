#include "../../../hw/api.h"


static const uint32_t kLogoWidth = 128;
static const uint32_t kLogoHeight = 128;


struct Context
{
    float prevTime;
    uint8_t* logo;
    Rect logoRect;
    bool touchOnPrevFrame;
    int prevTouchX;
    int prevTouchY;
    int dirX;
    int dirY;
    int diffX;
    int diffY;

    Context()
    {}

    bool Update(API* api);
};


inline void* operator new(size_t, void* __p) { return __p; }


void* GameInit(API* api, uint8_t* sdram)
{
    void* mem = api->Malloc(sizeof(Context));
    Context* ctx = new(mem) Context();

    ctx->prevTime = api->time();

    ctx->logo = sdram;
    void* f = api->fopen("/fs/logo.png", "r");
    uint32_t fileSize = api->fsize(f);
    api->fread(ctx->logo, fileSize, f);
    api->fclose(f);

    ctx->touchOnPrevFrame = false;
    ctx->logoRect = Rect(0, 80, kLogoWidth, kLogoHeight);
    ctx->dirX = 1;
    ctx->dirY = 1;

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

    TouchScreenState tsState;
    api->GetTouchScreenState(&tsState);

    Rect screenRect;
    api->GetScreenRect(&screenRect);

    if(tsState.touchDetected)
    {
        if(!touchOnPrevFrame)
        {
            diffX = (int32_t)tsState.touchX[0] - logoRect.x;
            diffY = (int32_t)tsState.touchY[0] - logoRect.y;
        }
        else
        {
            logoRect.x = (int32_t)tsState.touchX[0] - diffX;
            logoRect.y = (int32_t)tsState.touchY[0] - diffY;
        }
        touchOnPrevFrame = true;

        dirX = (int32_t)tsState.touchX[0] - prevTouchX;
        dirY = (int32_t)tsState.touchY[0] - prevTouchY;
        prevTouchX = (int32_t)tsState.touchX[0];
        prevTouchY = (int32_t)tsState.touchY[0];
    }
    else
    {
        touchOnPrevFrame = false;

        logoRect.x += dirX;
        if(logoRect.x + logoRect.width > screenRect.width)
            dirX = -dirX;
        else if(logoRect.x < 0)
            dirX = -dirX;

        logoRect.y += dirY;
        if(logoRect.y + logoRect.height > screenRect.height)
            dirY = -dirY;
        else if(logoRect.y < 0)
            dirY = -dirY;
    }

    api->LCD_Clear(0xff000000);

    char buf[128];
    api->LCD_SetFont(kFont12);
    api->LCD_SetTextColor(0xffffffff);
    api->sprintf(buf, "Frame time: %8.2f ms", api->floatToDouble(dt * 1000.0f));
    api->LCD_DisplayStringAt(5, 5, buf, kTextAlignNone);
    api->sprintf(buf, "Press button to exit");
    api->LCD_DisplayStringAt(5, 17, buf, kTextAlignNone);

    Rect clampedRect = logoRect;
    clampedRect.ClampByRect(screenRect);
    if(clampedRect.Area())
    {
        uint32_t pitch = kLogoWidth * 4;
        uint32_t offset = 0;
        if(logoRect.x < 0)
            offset = -logoRect.x * 4;
        if(logoRect.y < 0)
            offset += -logoRect.y * pitch;
        api->LCD_DrawImageWithBlend(clampedRect, logo + offset, pitch);
    }

    return true;
}