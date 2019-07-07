#pragma once
#include <stdint.h>
#include <stddef.h>

enum EColors
{
    kColorBlue          = 0xFF0000FF,
    kColorGreen         = 0xFF00FF00,
    kColorRed           = 0xFFFF0000,
    kColorCyan          = 0xFF00FFFF,
    kColorMagenta       = 0xFFFF00FF,
    kColorYellow        = 0xFFFFFF00,
    kColorLightBlue     = 0xFF8080FF,
    kColorLightGreen    = 0xFF80FF80,
    kColorLightRed      = 0xFFFF8080,
    kColorLightCyan     = 0xFF80FFFF,
    kColorLightMagenta  = 0xFFFF80FF,
    kColorLightYellow   = 0xFFFFFF80,
    kColorDarkBlue      = 0xFF000080,
    kColorDarkGreen     = 0xFF008000,
    kColorDarkRed       = 0xFF800000,
    kColorDarkCyan      = 0xFF008080,
    kColorDarkMagenta   = 0xFF800080,
    kColorDarkYellow    = 0xFF808000,
    kColorWhite         = 0xFFFFFFFF,
    kColorLightGray     = 0xFFD3D3D3,
    kColorGray          = 0xFF808080,
    kColorDartGray      = 0xFF404040,
    kColorBlack         = 0xFF000000,
    kColorBrown         = 0xFFA52A2A,
    kColorOrange        = 0xFFFFA500,
    kColorTransparent   = 0xFF000000,
};

struct Rect
{
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    Rect()
        : x(0), y(0), width(0), height(0)
    {}

    Rect(int32_t x_, int32_t y_, int32_t width_, int32_t height_)
        : x(x_), y(y_), width(width_), height(height_)
    {}

    bool IsPointInside(int32_t px, int32_t py)
    {
        return (px >= x) && (py >= y) && (px < (x + width)) && (py < (y + height));
    }

    void ClampByRect(const Rect& rect)
    {
        if(x < rect.x)
        {
            width -= (rect.x - x);
            if(width < 0)
                width = 0;
            x = rect.x;
        }
        if(x >= rect.x + rect.width)
        {
            x = rect.x + rect.width;
            width = 0;
        }

        if(y < rect.y)
        {
            height -= (rect.y - y);
            if(height < 0)
                height = 0;
            y = rect.y;
        }
        if(y >= rect.y + rect.height)
        {
            y = rect.y + rect.height;
            height = 0;
        }

        if(x + width - 1 >= rect.x + rect.width)
            width = rect.width - x;

        if(y + height - 1 >= rect.y + rect.height)
            height = rect.height - y;
    }

    int32_t Area() const
    {
        return width * height;
    }
};

#define kTouchScreenMaxTouches 5

struct TouchScreenState
{
    uint8_t  touchDetected;
    uint16_t touchX[kTouchScreenMaxTouches];
    uint16_t touchY[kTouchScreenMaxTouches];

    uint8_t  touchWeight[kTouchScreenMaxTouches];
    uint8_t  touchEventId[kTouchScreenMaxTouches];
    uint8_t  touchArea[kTouchScreenMaxTouches];
};


enum ENetwokConnectionStatus
{
    kNetwokConnectionStatusLocalUp = 0,
    kNetwokConnectionStatusGlobalUp = 1,
    kNetwokConnectionStatusDisconnected = 2,
    kNetwokConnectionStatusConnecting = 3,
    kNetwokConnectionStatusUnkown
};


struct ServerRequest
{
    char url[64];
    void* responseData;
    uint32_t responseDataCapacity;
    bool done;
    bool succeed;
    void* internalData;
};


enum ETextAlignMode
{
    kTextAlignLeft = 0,
    kTextAlignCenter,
    kTextAlignRight
};


class API
{
public:
    virtual uint8_t* GetSDRam() = 0;
    virtual void* Malloc(uint32_t size) = 0;
    virtual void Free(void* ptr) = 0;

    virtual void GetScreenRect(Rect* rect) = 0;
    virtual void GetTouchScreenState(TouchScreenState* state) = 0;

    virtual ENetwokConnectionStatus GetNetwokConnectionStatus() = 0;
    virtual const char* GetIPAddress() = 0;
    virtual ServerRequest* AllocServerRequest() = 0;
    virtual bool SendServerRequest(ServerRequest* request) = 0;
    virtual void FreeServerRequest(ServerRequest* request) = 0;

    virtual void SwapFramebuffer() = 0;
    virtual void LCD_OnOff(bool onOff) = 0;
    virtual bool LCD_IsOn() = 0;
    virtual void LCD_Clear(uint32_t color) = 0;
    virtual void LCD_SetBackColor(uint32_t color) = 0;
    virtual void LCD_SetTextColor(uint32_t color) = 0;
    virtual void LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode) = 0;
    virtual void LCD_FillRect(const Rect& rect, uint32_t color) = 0;
    virtual void LCD_DrawImage(const Rect& rect, uint8_t* image, uint32_t pitch) = 0;

    virtual void* fopen(const char* filename, const char* mode) = 0;
    virtual uint32_t fread(void* buffer, uint32_t size, void* file) = 0;
    virtual uint32_t fwrite(const void* buffer, uint32_t size, void* file) = 0;
    virtual uint32_t fsize(void* file) = 0;
    virtual void fclose(void* file) = 0;

    virtual void* memcpy(void* dst, const void* src, uint32_t size) = 0;
    virtual uint32_t strlen(const char* str) = 0;
    virtual void strcpy(char* dst, const char* src) = 0;
    virtual void sprintf(char* str, const char* formatStr, ...) = 0;
    virtual void printf(const char* formatStr, ...) = 0;
    virtual void sleep(float t) = 0;
    virtual float time() = 0;
};

typedef int (*TGameMain)(API*);