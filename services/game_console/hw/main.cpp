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


void InitNetwork()
{
    GEthernet.set_network("192.168.1.5", "255.255.255.0", "192.168.1.1");
    GEthernet.set_blocking(false);
    GEthernet.connect();   
}

 
int main()
{
    InitDisplay();
    InitNetwork();
    BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_GPIO);
    BSP_LED_Init(LED_GREEN);
    
    GAPIImpl.Init(&GEthernet, GSdram);

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

    uint32_t baseAddr = 0;
    memcpy(&baseAddr, code, 4);
    uint8_t* gameMainAddr = code + baseAddr + 4;

    ScopedRamExecutionLock make_ram_executable;
    TGameMain gameMain = (TGameMain)&gameMainAddr[1];
    gameMain(&GAPIImpl);
}
