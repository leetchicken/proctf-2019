#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "EthernetInterface.h"
#include "api_impl.h"
 
DigitalOut led1(LED1);


APIImpl GAPIImpl;
uint8_t* GSdram = NULL;
EthernetInterface GEthernet;


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


extern int GameMain(API* api);
 
int main()
{  
	led1 = 1;

    InitDisplay();
    InitNetwork();
    
    GAPIImpl.Init(&GEthernet, GSdram);

    ScopedRamExecutionLock make_ram_executable;
    GameMain(&GAPIImpl);
}
