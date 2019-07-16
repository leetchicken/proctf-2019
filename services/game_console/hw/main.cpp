#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "EthernetInterface.h"
#include "BlockDevice.h"
#include "FATFileSystem.h"
#include "api_impl.h"

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


char GMacAddress[] = {0x00, 0x80, 0xe1, 0xff, 0xff, 0xff};


uint8_t mbed_otp_mac_address(char *mac)
{
    memcpy(mac, GMacAddress, 6);
    return 1;
};


void InitNetwork()
{
    FILE* f = fopen("/fs/mac", "r");
    char macStr[128];
    memset(macStr, 0, sizeof(macStr));
    fread(macStr, 1, sizeof(macStr), f);
    fclose(f);

    for(size_t i = 0; i < 6; i++)
    {
        macStr[2 + i * 3] = 0;
        GMacAddress[i] = strtoul(&macStr[i * 3], NULL, 16);
    }

    GEthernet.set_dhcp(true);
    GEthernet.set_blocking(false);
    GEthernet.connect();   
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

#if _XX_
    ScopedRamExecutionLock make_ram_executable;
    void* gameCtx = GameInit(&GAPIImpl, GSdram);
    while(1)
    {
        GAPIImpl.SwapFramebuffer();
        GameUpdate(&GAPIImpl, gameCtx);
    }
#else
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

    void* gameCtx = gameInit(&GAPIImpl, GSdram);

    while(1)
    {
        GAPIImpl.SwapFramebuffer();
        gameUpdate(&GAPIImpl, gameCtx);
    }
#endif
}
