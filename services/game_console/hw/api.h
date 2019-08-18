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

    bool IsPointInside(int32_t px, int32_t py) const
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


enum EHttpMethod
{
    kHttpMethodGet = 0,
    kHttpMethodPost
};


struct HTTPRequest
{
    EHttpMethod httpMethod;
    char url[64];
    void* requestBody;
    uint32_t requestBodySize;
    void* responseData;
    uint32_t responseDataCapacity;
    // members below are only for reading
    bool done;
    bool succeed;
    uint32_t responseDataSize;
    // standart HTTP status code, or 0 in case of error
    uint32_t statusCode;
    // do not modify!!
    void* internalData;
};


enum ETextAlignMode
{
    kTextAlignLeft = 0,
    kTextAlignCenter,
    kTextAlignRight,
    kTextAlignNone
};


enum EFont
{
    kFont8 = 0,
    kFont12,
    kFont16,
    kFont20,
    kFont24
};


struct FontInfo
{
    uint32_t charWidth;
    uint32_t charHeight;
};


struct NetAddr
{
    uint32_t ip;
    uint16_t port;
};


enum ESocketError
{
    kSocketErrorOk                 =  0,     /*!< no error */
    kSocketErrorWouldBlock         = -1,     /*!< no data is not available but call is non-blocking */
    kSocketErrorUnsupported        = -2,     /*!< unsupported functionality */
    kSocketErrorParameter          = -3,     /*!< invalid configuration */
    kSocketErrorNoConnection       = -4,     /*!< not connected to a network */
    kSocketErrorNoSocket           = -5,     /*!< socket not available for use */
    kSocketErrorNoAddress          = -6,     /*!< IP address is not known */
    kSocketErrorNoMemory           = -7,     /*!< memory resource not available */
    kSocketErrorDnsFailure         = -8,     /*!< DNS failed to complete successfully */
    kSocketErrorDeviceError        = -9,     /*!< failure interfacing with the network processor */
    kSocketErrorInProgress         = -10,     /*!< operation (eg connect) in progress */
    kSocketErrorAlready            = -11,     /*!< operation (eg connect) already in progress */
    kSocketErrorIsConnected        = -12,     /*!< socket is already connected */
    kSocketErrorTimeout             = -13,      /*!< operation timed out */
    kSocketErrorUnknown             = -14,
};


class API
{
public:
    virtual void* Malloc(uint32_t size) = 0;
    virtual void Free(void* ptr) = 0;

    virtual void GetScreenRect(Rect* rect) = 0;
    virtual void GetTouchScreenState(TouchScreenState* state) = 0;

    virtual ENetwokConnectionStatus GetNetwokConnectionStatus() = 0;
    virtual const char* GetIPAddress() = 0;

    /*
    HTTP Interface.
    Usage example:

    HTTPRequest* request = api->AllocHTTPRequest();
    if(!request)
        <error handling>
    request->httpMethod = kHttpMethodGet;
    api->sprintf(request->url, "http://%s/icon?id=%x", kServerAddr, gameId);
    request->responseData = (void*)iconAddr;
    request->responseDataCapacity = kGameIconSize;
    if(!api->SendHTTPRequest(request))
        <error handling>
    while(!request->done)
        api->sleep(0.1);
    if(request->successed)
    {
        ...
    }
    api->FreeHTTPRequest(request);
     */
    virtual HTTPRequest* AllocHTTPRequest() = 0;
    virtual bool SendHTTPRequest(HTTPRequest* request) = 0;
    virtual void FreeHTTPRequest(HTTPRequest* request) = 0;

    virtual uint32_t aton(const char* ip) = 0;
    virtual void ntoa(uint32_t ip, char* ipStr) = 0;
    virtual int socket(bool tcp) = 0;
    virtual int set_blocking(int socket, bool blocking) = 0;
    virtual int send(int socket, const void* data, uint32_t size, NetAddr* addr = NULL) = 0;
    virtual int recv(int socket, void* data, uint32_t size, NetAddr* addr = NULL) = 0;
    virtual int connect(int socket, const NetAddr& addr) = 0;
    virtual int bind(int socket, uint32_t ip, uint16_t port) = 0;
    virtual int listen(int socket, int backlog = 1) = 0;
    virtual int accept(int socket) = 0;
    virtual int getpeername(int socket, NetAddr& addr) = 0;
    virtual void close(int socket) = 0;

    virtual void LCD_OnOff(bool onOff) = 0;
    virtual bool LCD_IsOn() = 0;
    virtual void LCD_Clear(uint32_t color) = 0;
    virtual void LCD_DrawPixel(uint32_t x, uint32_t y, uint32_t pixel) = 0;
    virtual void LCD_SetBackColor(uint32_t color) = 0;
    virtual void LCD_SetTextColor(uint32_t color) = 0;
    virtual void LCD_SetFont(EFont font) = 0;
    virtual void LCD_GetFontInfo(EFont font, FontInfo* fontInfo) = 0;
    virtual void LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode) = 0;
    virtual void LCD_DisplayChar(uint16_t xpos, uint16_t ypos, char ascii) = 0;
    virtual void LCD_FillRect(const Rect& rect, uint32_t color) = 0;
    virtual void LCD_DrawImage(const Rect& rect, uint8_t* image, uint32_t pitch) = 0;
    virtual void LCD_DrawImageWithBlend(const Rect& rect, uint8_t* image, uint32_t pitch) = 0;

    virtual bool GetButtonState() = 0;
    virtual void LedOnOff(bool on) = 0;

    virtual void* fopen(const char* filename, const char* mode) = 0;
    virtual uint32_t fread(void* buffer, uint32_t size, void* file) = 0;
    virtual uint32_t fwrite(const void* buffer, uint32_t size, void* file) = 0;
    virtual uint32_t fsize(void* file) = 0;
    virtual void fclose(void* file) = 0;

    virtual void* memcpy(void* dst, const void* src, uint32_t size) = 0;
    virtual void* memset(void* dst, uint32_t val, uint32_t size) = 0;
    virtual uint32_t strlen(const char* str) = 0;
    virtual void strcpy(char* dst, const char* src) = 0;
    virtual void sprintf(char* str, const char* formatStr, ...) = 0;
    virtual void printf(const char* formatStr, ...) = 0;
    virtual void sleep(float t) = 0;
    virtual float time() = 0;

    virtual double floatToDouble(float f) = 0;
};

typedef void* (*TGameInit)(API*, uint8_t*);
typedef bool (*TGameUpdate)(API*, void*);