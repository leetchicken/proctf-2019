#include "../../../hw/api.h"


struct Context
{
    uint32_t* buffer;
    float prevTime;
    uint32_t prevTouchX[kTouchScreenMaxTouches];
    uint32_t prevTouchY[kTouchScreenMaxTouches];

    Context()
    {}

    bool Update(API* api);
};


inline void* operator new(size_t, void* __p) { return __p; }


void* GameInit(API* api, uint8_t* sdram)
{
    void* mem = api->Malloc(sizeof(Context));
    Context* ctx = new(mem) Context();
    ctx->buffer = (uint32_t*)sdram;

    Rect rect;
    api->GetScreenRect(&rect);
    api->memset(ctx->buffer, 0, rect.width * rect.height * 4);

    ctx->prevTime = api->time();

    for(uint32_t i = 0; i < kTouchScreenMaxTouches; i++)
    {
        ctx->prevTouchX[i] = ~0u;
        ctx->prevTouchY[i] = ~0u;
    }

    return (void*)ctx;
}


bool GameUpdate(API* api, void* ctxVoid)
{
    Context* ctx = (Context*)ctxVoid;
    return ctx->Update(api);
}


int32_t abs(int32_t a)
{
    if(a < 0) return -a;
    return a;
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

    uint32_t colors[kTouchScreenMaxTouches];
    colors[0] = 0xff0000ff;
    colors[1] = 0xff00ff00;
    colors[2] = 0xffff0000;
    colors[3] = 0xffff00ff;
    colors[3] = 0xffffff00;

    Rect rect;
    api->GetScreenRect(&rect);

    TouchScreenState tsState;
    api->GetTouchScreenState(&tsState);
    for(uint32_t i = 0; i < tsState.touchDetected; i++)
    {
        uint32_t x = tsState.touchX[i];
        uint32_t y = tsState.touchY[i];
        uint32_t color = colors[i];
        
        if(prevTouchX[i] != ~0u)
        {
            int32_t x0 = prevTouchX[i];
            int32_t y0 = prevTouchY[i];
            int32_t x1 = x;
            int32_t y1 = y;
            
            const int32_t deltaX = abs(x1 - x0);
            const int32_t deltaY = abs(y1 - y0);
            const int32_t signX = x0 < x1 ? 1 : -1;
            const int32_t signY = y0 < y1 ? 1 : -1;
            int32_t error = deltaX - deltaY;

            buffer[y1 * rect.width + x1] = color;
            while(x0 != x1 || y0 != y1) 
            {
                buffer[y0 * rect.width + x0] = color;
                const int32_t error2 = error * 2;
            
                if(error2 > -deltaY) 
                {
                    error -= deltaY;
                    x0 += signX;
                }
                if(error2 < deltaX) 
                {
                    error += deltaX;
                    y0 += signY;
                }
            }
        }
        else
        {
            buffer[y * rect.width + x] = color;
        }
        prevTouchX[i] = x;
        prevTouchY[i] = y;
    }

    api->LCD_DrawImage(rect, (uint8_t*)buffer, rect.width * 4);

    char buf[128];
    api->LCD_SetFont(kFont12);
    api->LCD_SetTextColor(0xffffffff);
    api->sprintf(buf, "Frame time: %8.2f ms", api->floatToDouble(dt * 1000.0f));
    api->LCD_DisplayStringAt(5, 5, buf, kTextAlignNone);
    api->sprintf(buf, "Press button to exit");
    api->LCD_DisplayStringAt(5, 17, buf, kTextAlignNone);
    api->sprintf(buf, "Draw with your fingers");
    api->LCD_DisplayStringAt(5, 29, buf, kTextAlignNone);

    return true;
}