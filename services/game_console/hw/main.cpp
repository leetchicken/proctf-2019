#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "EthernetInterface.h"
#include "TCPServer.h"
#include "TCPSocket.h"
#include "http_request.h"
#include "MemoryPool.h"
#include "Queue.h"
#include "api_impl.h"
 
DigitalOut led1(LED1);

typedef int (*TGameMain)(API*);
APIImpl GAPIImpl;

struct Rect
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;

    bool IsPointInside(uint32_t px, uint32_t py)
    {
        return (px >= x) && (py >= y) && (px < (x + width)) && (py < (y + height));
    }
};

struct GameDesc
{
    Rect uiRect;
    uint32_t id;
    std::string name;
};

typedef std::vector<GameDesc> TGameList;

enum ERequests
{
    kRequestGameList = 0,
    kRequestGameCode,
    kReqeustGameAssets,

    kRequestsCount
};


struct Request
{
    ERequests type;
    void* responeData;
    bool done;
    bool succeed;
};

static const char* kServerAddr = "192.168.1.1:8000";

EthernetInterface GEthernet;
Thread GHttpThread;
MemoryPool<Request, 64> GRequestsPool;
Queue<Request, 64> GRequestsQueue;


void HttpThread()
{
    while(true)
    {
        osEvent evt = GRequestsQueue.get();
        if (evt.status == osEventMessage) 
        {
            Request* request = (Request*)evt.value.p;

            char url[64];
            if(request->type == kRequestGameList)
                sprintf(url, "http://%s/list", kServerAddr);
            else
            {
                printf("Invalid request: %d\n", request->type);
                continue;
            }            

            printf("Request: %s\n", url);
            HttpRequest* httpRequest = new HttpRequest(&GEthernet, HTTP_GET, url);
            HttpResponse* httpResponse = httpRequest->send();
            if (httpResponse) 
            {
                if(httpResponse->get_status_code() == 200)
                {
                    request->succeed = true;
                }
                else
                {
                    request->succeed = false;
                }
                
                delete httpRequest;

                request->done = true;
            }
            else
            {
                printf("HttpRequest failed (error code %d)\n", httpRequest->get_error());
                request->succeed = false;
                request->done = true;
            }
        }
    }
}


void BSP_LCD_FillRect(const Rect& rect)
{
    BSP_LCD_FillRect(rect.x, rect.y, rect.width, rect.height);
}


enum EMainScreenState
{
    kMainScreenReady = 0,
    kMainScreenWaitGameList,

    kMainScreenStatesCount
};


Request* SendGetGameList(TGameList* games)
{
    Request* request = GRequestsPool.alloc();
    request->type = kRequestGameList;
    request->done = false;
    request->succeed = false;
    request->responeData = (void*)&games;
    GRequestsQueue.put(request);
    return request;
}


void MainScreen()
{
    TS_StateTypeDef tsState;

    Rect updateRect = {440, 20, 40, 40};

    TGameList games;
    EMainScreenState state = kMainScreenReady;

    Request* request = SendGetGameList(&games);
    state = kMainScreenWaitGameList;

    uint32_t progressColor = 0xFF0000FF;

    int vsyncCurrentBuffer = 0;

    while(1)
    {
        // vsync
        while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
        BSP_LCD_SetLayerVisible(vsyncCurrentBuffer, DISABLE);
        BSP_LCD_SelectLayer(vsyncCurrentBuffer);       
        vsyncCurrentBuffer = (vsyncCurrentBuffer + 1) % 2;
        BSP_LCD_SetLayerVisible(vsyncCurrentBuffer, ENABLE); 

        if(state == kMainScreenWaitGameList)
        {
            if(request->done)
            {
                state = kMainScreenReady;
                GRequestsPool.free(request);
                request = NULL;
                printf("Received games list\n");
            }
        }

        // touch screen
        BSP_TS_GetState(&tsState);

        uint32_t selectedGame = ~0u;
        for (uint8_t i = 0; i < tsState.touchDetected; i++)
        {
            for(uint32_t g = 0; g < games.size(); g++)
            {
                Rect& rect = games[g].uiRect;
                if(rect.IsPointInside(tsState.touchX[i], tsState.touchY[i]))
                {
                    selectedGame = g;
                    break;
                }
            }
            
            if(selectedGame != ~0u)
            	break;

            if(state == kMainScreenReady && updateRect.IsPointInside(tsState.touchX[i], tsState.touchY[i]))
            {
                request = SendGetGameList(&games);
                state = kMainScreenWaitGameList;
                printf("Requested games list\n");
            }
        }

        // rendering
        BSP_LCD_Clear(LCD_COLOR_DARKBLUE);

        BSP_LCD_SetTextColor(LCD_COLOR_LIGHTCYAN);
        BSP_LCD_FillRect(20, 20, 40, 40);

        if(state != kMainScreenReady)
        {
            BSP_LCD_SetTextColor(progressColor);
            BSP_LCD_FillRect(440, 20, 40, 40);
            uint32_t b = progressColor & 255;
            b = (b + 1) & 255;
            progressColor = 0xFF000000 | ((255 - b) << 8) | b;
        }

        for(uint32_t g = 0; g < games.size(); g++)
        {
            BSP_LCD_SetTextColor(g == selectedGame ? LCD_COLOR_LIGHTMAGENTA: LCD_COLOR_DARKMAGENTA);    
            BSP_LCD_FillRect(games[g].uiRect);
        }
    }
}


void InitDisplay()
{
    BSP_LCD_Init();

    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_LayerDefaultInit(1, LCD_FB_START_ADDRESS+(BSP_LCD_GetXSize()*BSP_LCD_GetYSize()*4));

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
    GEthernet.connect();   
    printf("The target IP address is '%s'\n", GEthernet.get_ip_address());

    GHttpThread.start(HttpThread);
}

 
int main()
{  
	led1 = 1;

    InitDisplay();
    InitNetwork();
    MainScreen();
}
