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
    void LCD_OnOff(bool onOff);
    bool LCD_IsOn();
    void LCD_Clear(uint32_t color);
    void LCD_SetBackColor(uint32_t color);
    void LCD_SetTextColor(uint32_t color);
    void LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode);
    void LCD_FillRect(const Rect& rect, uint32_t color);
    void LCD_DrawImage(const Rect& rect, uint8_t* image, uint32_t pitch);
    void LCD_DrawImageWithBlend(const Rect& rect, uint8_t* image, uint32_t pitch);

    bool GetButtonState();
    void LedOnOff(bool on);

    void* fopen(const char* filename, const char* mode);
    uint32_t fread(void* buffer, uint32_t size, void* file);
    uint32_t fwrite(const void* buffer, uint32_t size, void* file);
    uint32_t fsize(void* file);
    void fclose(void* file);

    void* memcpy(void* dst, const void* src, uint32_t size);
    uint32_t strlen(const char* str);
    void strcpy(char* dst, const char* src);
    void sprintf(char* str, const char* formatStr, ...);
    void printf(const char* formatStr, ...);
    void sleep(float t);
    float time();

private:
    int m_curFrameBuffer;
    EthernetInterface* m_ethInterface;
    uint8_t* m_sdram;
    Timer m_timer;
    bool m_displayIsOn;
};

