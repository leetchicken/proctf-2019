#pragma once
#include "api.h"
#include "mbed.h"
#include "EthernetInterface.h"

class APIImpl : public API
{
public:
    APIImpl();

    void Init(EthernetInterface* ethInterface, uint8_t* sdram);

    uint8_t* GetSDRam();
    void* Malloc(uint32_t size);
    void Free(void* ptr);

    void GetScreenRect(Rect* rect);
    void GetTouchScreenState(TouchScreenState* state);

    ENetwokConnectionStatus GetNetwokConnectionStatus();
    const char* GetIPAddress();
    ServerRequest* AllocServerRequest();
    bool SendServerRequest(ServerRequest* request);
    void FreeServerRequest(ServerRequest* request);

    void SwapFramebuffer();
    void LCD_Clear(uint32_t color);
    void LCD_SetBackColor(uint32_t color);
    void LCD_SetTextColor(uint32_t color);
    void LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode);
    void LCD_FillRect(const Rect& rect, uint32_t color);
    void LCD_DrawImage(const Rect& rect, uint8_t* image, uint32_t pitch);

    void* memcpy(void* dst, const void* src, uint32_t size);
    uint32_t strlen(const char* str);
    void strcpy(char* dst, const char* src);
    void sprintf(char* str, const char* formatStr, ...);
    void printf(const char* formatStr, ...);
    void sleep(float t);

private:
    int m_curFrameBuffer;
    EthernetInterface* m_ethInterface;
    uint8_t* m_sdram;
};
