#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "EthernetInterface.h"
#include "BlockDevice.h"
#include "FATFileSystem.h"
#include "api_impl.h"
#include "team_data.h"
#include "../main_screen/constants.h"

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


char GMacAddress[] = {0x00, 0x80, 0xe1, 0x00, 0x00, MAC5};


uint8_t mbed_otp_mac_address(char *mac)
{
    memcpy(mac, GMacAddress, 6);
    return 1;
};


void InitNetwork()
{
    GEthernet.set_dhcp(true);
    GEthernet.set_blocking(false);
    GEthernet.connect();   
}


int recv(TCPSocket* socket, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

    while(remain)
    {
        int ret = socket->recv(ptr, remain);
        if(ret <= 0)
            return ret;
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}


int send(TCPSocket* socket, void* data, uint32_t size)
{
    uint8_t* ptr = (uint8_t*)data;
    uint32_t remain = size;

    while(remain)
    {
        int ret = socket->send(ptr, remain);
        if(ret <= 0)
            return ret;
        remain -= ret;
        ptr += ret;
    }
    
    return (int)size;
}


struct Point2D
{
    int32_t x;
    int32_t y;
};


int32_t EdgeFunction(const Point2D& a, const Point2D& b, const Point2D& c) 
{
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}


#define CS_DEBUG 0
#if CS_DEBUG
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

    const uint32_t kScreenSize = 24;

    while(1)
    {
        if(!socket)
        {
            CS_PRINTF("CHECKSYSTEM: Trying to connect to checksystem\n");
            socket = AllocSocket();
            socket->open(&GEthernet);
            socket->set_timeout(-1);
            int ret = socket->connect(sockAddr);
            if(ret == 0)
            {
                CS_PRINTF("CHECKSYSTEM: Connected\n");
            }
            else
            {
                FreeSocket(socket);
                wait(5.0f);
                continue;
            }
        }

        Point2D v[3];
        int ret = recv(socket, &v, sizeof(v));
        if(ret <= 0)
        {
            if(ret == 0)
            {
                CS_PRINTF("CHECKSYSTEM: connection has been closed by checksystem\n");
            }
            else
            {
                CS_PRINTF("CHECKSYSTEM: socket.recv failed(%d)\n", ret);
            }
            FreeSocket(socket);
            continue;
        }

        int32_t minX = kScreenSize - 1;
        int32_t minY = kScreenSize - 1;
        int32_t maxX = 0;
        int32_t maxY = 0;

        for(uint32_t vi = 0; vi < 3; vi++)
        {
            if(v[vi].x > maxX) maxX = v[vi].x;
            if(v[vi].y > maxY) maxY = v[vi].y;
            if(v[vi].x < minX) minX = v[vi].x;
            if(v[vi].y < minY) minY = v[vi].y;
        }

        uint32_t result = 0;

        int doubleTriArea = EdgeFunction(v[0], v[1], v[2]);
        if(doubleTriArea > 0)
        {
            Point2D p;
            for(p.y = minY; p.y <= maxY; p.y++)
            {
                for(p.x = minX; p.x <= maxX; p.x++)
                {
                    int32_t w0 = EdgeFunction(v[1], v[2], p);
                    int32_t w1 = EdgeFunction(v[2], v[0], p);
                    int32_t w2 = EdgeFunction(v[0], v[1], p);

                    if((w0 | w1 | w2) >= 0) 
                    {
                        uint32_t pixel = w0 + w1 + w2;
                        result ^= pixel;
                    }
                }
            }
        }        

        socket->set_timeout(3000);
        ret = send(socket, &result, sizeof(result));
        if(ret < 0)
        {
            CS_PRINTF("CHECKSYSTEM: socket.send failed(%d)\n", ret);
            FreeSocket(socket);
            continue;
        }
        socket->set_timeout(-1);
    }
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

    Thread checksystemThread;
    checksystemThread.start(callback(ChecksystemThread));
    
    GAPIImpl.Init(&GEthernet, USER_NAME);

    printf("User name: %s\n", USER_NAME);

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
