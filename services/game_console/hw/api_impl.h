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
    HTTPRequest* AllocHTTPRequest();
    bool SendHTTPRequest(HTTPRequest* request);
    void FreeHTTPRequest(HTTPRequest* request);

    uint32_t aton(const char* ip);
    void ntoa(uint32_t ip, char* ipStr);
    int socket(bool tcp);
    int set_blocking(int socket, bool blocking);
    int send(int socket, const void* data, uint32_t size, NetAddr* addr = NULL);
    int recv(int socket, void* data, uint32_t size, NetAddr* addr = NULL);
    int connect(int socket, const NetAddr& addr);
    int bind(int socket, uint32_t ip, uint16_t port);
    int listen(int socket, int backlog = 1);
    int accept(int socket);
    int getpeername(int socket, NetAddr& addr);
    void close(int socket);

    void SwapFramebuffer();
    void LCD_OnOff(bool onOff);
    bool LCD_IsOn();
    void LCD_Clear(uint32_t color);
    void LCD_SetBackColor(uint32_t color);
    void LCD_SetTextColor(uint32_t color);
    void LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode);
    void LCD_DisplayChar(uint16_t xpos, uint16_t ypos, char ascii);
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
    void* memset(void* dst, uint32_t val, uint32_t size);
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
    TCPSocket* m_tcpSockets[32];
    uint32_t m_freeTcpSockets;
    uint32_t m_acceptedSockets;
    UDPSocket* m_udpSockets[32];
    uint32_t m_freeUdpSockets;
};

