#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "EthernetInterface.h"
#include "BlockDevice.h"
#include "FATFileSystem.h"
#include "api_impl.h"
#include "../main_screen/constants.h"
#include "ip4string.h"
#include "../common.h"

APIImpl GAPIImpl;
uint8_t* GSdram = NULL;
EthernetInterface GEthernet;
FATFileSystem fs ("fs", BlockDevice::get_default_instance());


void InitDisplay()
{
    BSP_LCD_Init();

    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS + BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * 4);

    GSdram = (uint8_t*)(LCD_FB_START_ADDRESS + BSP_LCD_GetXSize() * BSP_LCD_GetYSize() * 4 * 2);

    BSP_LCD_DisplayOn();

    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SelectLayer(1);
    BSP_LCD_Clear(LCD_COLOR_BLACK);

    BSP_LCD_SetFont(&LCD_DEFAULT_FONT);

    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);

    BSP_TS_Init(BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}


void InitNetwork()
{
    GEthernet.set_dhcp(true);
    GEthernet.set_blocking(false);
    GEthernet.connect();   
}

#define CS_DEBUG 0
#if CS_DEBUG || defined(TARGET_DEBUG)
#define CS_PRINTF(...) printf(__VA_ARGS__)
#else
#define CS_PRINTF(...)
#endif


MemoryPool<TCPSocket, 1> GSocketPool;


TCPSocket* AllocSocket()
{
    return new(GSocketPool.alloc()) TCPSocket();
}


void FreeSocket(TCPSocket*& sock)
{
    sock->~TCPSocket();
    GSocketPool.free(sock);
    sock = nullptr;
}


void ChecksystemThread()
{
    TCPSocket* socket = nullptr;
    SocketAddress sockAddr(kServerIP, kServerChecksystemPort);

    const uint32_t kMask = (1 << kBitsForCoord) - 1;
    const float kWaitTime = 0.5f;

    while(1)
    {
        if(GEthernet.get_connection_status() != NSAPI_STATUS_GLOBAL_UP)
        {
            wait(kWaitTime);
            continue;
        }

        if(!socket)
        {
            socket = AllocSocket();
            socket->open(&GEthernet);
            socket->set_blocking(false);
        }

        int ret = socket->connect(sockAddr);
        if(ret == NSAPI_ERROR_IN_PROGRESS || ret == NSAPI_ERROR_ALREADY)
        {
            wait(kWaitTime);
            continue;
        }
        else if(ret != 0 && ret != NSAPI_ERROR_IS_CONNECTED)
        {
            CS_PRINTF("CHECKSYSTEM: connect err = %d\n", ret);
            FreeSocket(socket);
            wait(kWaitTime);
            continue;
        }

        Point2D v[3];
        ret = socket->recv(&v, sizeof(v));
        if(ret == NSAPI_ERROR_WOULD_BLOCK)
        {
            wait(kWaitTime);
            continue;
        }
        else if(ret != sizeof(v))
        {
            CS_PRINTF("CHECKSYSTEM: recv err = %d\n", ret);
            FreeSocket(socket);
            wait(kWaitTime);
            continue;
        }

        for(uint32_t i = 0; i < 3; i++)
        {
            v[i].x = v[i].x & kMask;
            v[i].y = v[i].y & kMask;
        }

        uint32_t result = Rasterize(v);
        ret = socket->send(&result, sizeof(result));
    }
}


void RebootTimer(uint32_t textY, uint32_t charHeight)
{
    char buf[512];
    int timer = 5;
    while(timer > 0)
    {
        sprintf(buf, "Reboot in %d seconds", timer);
        GAPIImpl.LCD_DisplayStringAt(10, textY, buf, kTextAlignNone);
        textY += charHeight;
        wait(1.0f);
        timer--;
    }
}


void FactoryReset()
{
    printf("Factory reset\n");
    GAPIImpl.LCD_SetFont(kFont12);
    FontInfo fontInfo;
    GAPIImpl.LCD_GetFontInfo(kFont12, &fontInfo);
    uint32_t textY = 10;

    GAPIImpl.LCD_Clear(0xff000000);
    GAPIImpl.LCD_SetTextColor(0xffffffff);

    GAPIImpl.LCD_DisplayStringAt(10, textY, "Factory reset", kTextAlignNone);
    textY += fontInfo.charHeight;
    GAPIImpl.LCD_DisplayStringAt(10, textY, "Press button if you want to reset username and password", kTextAlignNone);
    textY += fontInfo.charHeight;
    GAPIImpl.LCD_DisplayStringAt(10, textY, "Press reset to exit", kTextAlignNone);
    textY += fontInfo.charHeight;

    wait(2.0f);

    while(!GAPIImpl.GetButtonState())
        wait(0.1f);

    GAPIImpl.LCD_DisplayStringAt(10, textY, "Waiting for the network...", kTextAlignNone);
    textY += fontInfo.charHeight;
    while(GEthernet.get_connection_status() != NSAPI_STATUS_GLOBAL_UP)
    {
        wait(0.1f);
        continue;
    }

    const char* ipStr = GAPIImpl.GetIPAddress();
    char buf[512];
    memset(buf, 0, 512);
    sprintf(buf, "Connected. IP = %s", ipStr);
    GAPIImpl.LCD_DisplayStringAt(10, textY, buf, kTextAlignNone);
    textY += fontInfo.charHeight;

    HTTPRequest* request = GAPIImpl.AllocHTTPRequest();
    if(!request)
    {
        GAPIImpl.LCD_DisplayStringAt(10, textY, "Factory reset failed: AllocHTTPRequest failed", kTextAlignNone);
        textY += fontInfo.charHeight;
        RebootTimer(textY, fontInfo.charHeight);
        return;
    }
    request->httpMethod = kHttpMethodGet;
    GAPIImpl.sprintf(request->url, "http://%s:%u/teamname", kServerIP, kServerPort);
    if(!GAPIImpl.SendHTTPRequest(request))
    {
        GAPIImpl.FreeHTTPRequest(request);
        GAPIImpl.LCD_DisplayStringAt(10, textY, "Factory reset failed: SendHTTPRequest failed", kTextAlignNone);
        textY += fontInfo.charHeight;
        RebootTimer(textY, fontInfo.charHeight);
        return;
    }

    while(!request->done)
    {
        wait(0.1f);
        continue;
    }

    if(request->statusCode != 200 || request->responseDataSize > 256)
    {
        GAPIImpl.FreeHTTPRequest(request);
        sprintf(buf, "Factory reset failed: status code = %u, response size = %u", request->statusCode, request->responseDataSize);
        GAPIImpl.LCD_DisplayStringAt(10, textY, buf, kTextAlignNone);
        textY += fontInfo.charHeight;
        RebootTimer(textY, fontInfo.charHeight);
        return;
    }

    char userName[256];
    memset(userName, 0, sizeof(userName));
    memcpy(userName, request->responseData, request->responseDataSize); 

    const char* password = "00000000";

    sprintf(buf, "Set userName = '%s', password = '%s'", userName, password);
    GAPIImpl.LCD_DisplayStringAt(10, textY, buf, kTextAlignNone);
    textY += fontInfo.charHeight;        

    FILE* f = fopen("/fs/username", "w");
    fwrite(userName, 1, strlen(userName), f);
    fflush(f);
    fclose(f);

    f = fopen("/fs/password", "w");
    fwrite(password, 1, strlen(password), f);
    fflush(f);
    fclose(f);

    RebootTimer(textY, fontInfo.charHeight);
}


#define _XX_ 0

#if _XX_
extern void* GameInit(API*, uint8_t*);
extern bool GameUpdate(API*, void*);
#endif

 
int main()
{
    InitDisplay();
    InitNetwork();
    BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
    BSP_LED_Init(LED_GREEN);

    GAPIImpl.Init(&GEthernet);

    if(GAPIImpl.GetButtonState())
    {
        float start = GAPIImpl.time();
        while(GAPIImpl.GetButtonState())
        {
            wait(0.1f);
            float duration = GAPIImpl.time() - start;
            if(duration > 5.0f)
            {
                FactoryReset();
                NVIC_SystemReset();
                return 0;
            }
        }
    }

    Thread checksystemThread;
    checksystemThread.start(callback(ChecksystemThread));

#if _XX_
    ScopedRamExecutionLock make_ram_executable;
    void* gameCtx = GameInit(&GAPIImpl, GSdram);
    while(1)
    {
        GAPIImpl.SwapFramebuffer();
        GameUpdate(&GAPIImpl, gameCtx);
    }
#else
    printf("Loading main screen application...\n");
    FILE* f = fopen("/fs/code.bin", "r");
    if(!f)
    {
        printf("Failed to open /fs/code.bin\n");
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size_t codeFileSize = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* code = (uint8_t*)malloc(codeFileSize);
    fread(code, 1, codeFileSize, f);
    fclose(f);

    uint8_t* ptr = code;

    uint32_t gameInitOffset = 0;
    memcpy(&gameInitOffset, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    uint32_t gameUpdateOffset = 0;
    memcpy(&gameUpdateOffset, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    uint8_t* gameInitAddr = ptr + gameInitOffset;
    uint8_t* gameUpdateAddr = ptr + gameUpdateOffset;

    ScopedRamExecutionLock make_ram_executable;
    TGameInit gameInit = (TGameInit)&gameInitAddr[1];
    TGameUpdate gameUpdate = (TGameUpdate)&gameUpdateAddr[1];

    printf("GameInit: %x\n", gameInitAddr);
    printf("GameUpdate: %x\n", gameUpdateAddr);

    void* gameCtx = gameInit(&GAPIImpl, GSdram);

    printf("Game context: %x\n", gameCtx);

    while(1)
    {
        GAPIImpl.SwapFramebuffer();
        gameUpdate(&GAPIImpl, gameCtx);
    }
#endif
}
