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

static const char* kServerAddr = "192.168.1.1:8000";
static const uint32_t kIconWidth = 172;
static const uint32_t kIconHeight = 172;
static const uint32_t kMaxIconsOnScreen = 3;
static const uint32_t kIconCacheSize = kMaxIconsOnScreen + 1;
static const uint32_t kMaxGameCodeSize = 1024;
 
DigitalOut led1(LED1);

typedef int (*TGameMain)(API*);
APIImpl GAPIImpl;

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

    bool IsPointInside(int32_t px, int32_t py)
    {
        return (px >= x) && (py >= y) && (px < (x + width)) && (py < (y + height));
    }

    void ClampByRect(const Rect& rect)
    {
        if(x < rect.x)
            x = rect.x;
        if(x >= rect.x + rect.width)
            x = rect.x + rect.width;

        if(y < rect.y)
            y = rect.y;
        if(y >= rect.y + rect.height)
            y = rect.y + rect.height;

        if(x + width - 1 < rect.x)
            width = 0;
        if(x + width - 1 >= rect.x + rect.width)
            width = rect.width - x;

        if(y + height - 1 < rect.y)
            height = 0;
        if(y + height - 1 >= rect.y + rect.height)
            height = rect.height - y;
    }

    int32_t Area() const
    {
        return width * height;
    }
};

struct IconCache
{
    struct Item
    {
        uint8_t* addr;
        uint32_t gameIndex;
    };

    Item cache[kIconCacheSize];

    IconCache(uint8_t* startAddr)
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
        {
            cache[i].addr = startAddr + i * kIconWidth * kIconHeight * 4;
            cache[i].gameIndex = ~0u;
        }
    }

    void Clear()
    {
        for(uint32_t i = 0; i < kIconCacheSize; i++)
            cache[i].gameIndex = ~0u;
    }
};

enum EIconState
{
    kIconInvalid = 0,
    kIconLoading,
    kIconValid,

    kIconStatesCount
};

struct GameDesc
{
    Rect uiRect;
    uint32_t id;
    std::string name;
    uint8_t* iconAddr;
    EIconState iconState;

    GameDesc()
        : id(~0u), iconAddr(NULL), iconState(kIconInvalid)
    {}
};

typedef std::vector<GameDesc> TGameList;

enum ERequests
{
    kRequestGameList = 0,
    kRequestIcon,
    kRequestGameCode,
    kReqeustGameAssets,

    kRequestsCount
};


struct Request
{
    ERequests type;
    uint32_t gameId;
    void* responseData;
    bool done;
    bool succeed;
};

EthernetInterface GEthernet;
Thread GHttpThread;
MemoryPool<Request, 64> GRequestsPool;
Queue<Request, 64> GRequestsQueue;
Rect GScreenRect;

uint8_t* GSdram = NULL;


uint8_t* GHttpBodyCallbackPtr = NULL;
void HttpBodyCallback(const char* data, uint32_t data_len) 
{
    memcpy(GHttpBodyCallbackPtr, data, data_len);
    GHttpBodyCallbackPtr += data_len;
}


void HttpThread()
{
    while(true)
    {
        osEvent evt = GRequestsQueue.get();
        if (evt.status == osEventMessage) 
        {
            Request* request = (Request*)evt.value.p;

            char url[64];
            bool needBodyCallback = false;
            if(request->type == kRequestGameList)
            {
                sprintf(url, "http://%s/list", kServerAddr);
            }
            else if(request->type == kRequestIcon)
            {
                sprintf(url, "http://%s/icon?id=%u", kServerAddr, request->gameId);
                needBodyCallback = true;
                GHttpBodyCallbackPtr = (uint8_t*)request->responseData;
            }
            else if(request->type == kRequestGameCode)
            {
                sprintf(url, "http://%s/code?id=%u", kServerAddr, request->gameId);
                needBodyCallback = true;
                GHttpBodyCallbackPtr = (uint8_t*)request->responseData;
            }
            else
            {
                printf("Invalid request: %d\n", request->type);
                continue;
            }            

            printf("Request: %s\n", url);
            HttpRequest* httpRequest = needBodyCallback ? new HttpRequest(&GEthernet, HTTP_GET, url, HttpBodyCallback) 
                                                        : new HttpRequest(&GEthernet, HTTP_GET, url);
            HttpResponse* httpResponse = httpRequest->send();
            if (httpResponse) 
            {
                if(httpResponse->get_status_code() == 200)
                {
                    if(request->type == kRequestGameList)
                    {
                        TGameList& gameList = *(TGameList*)request->responseData;
                        uint8_t* bodyPtr = (uint8_t*)httpResponse->get_body();
                        uint32_t gamesNum;
                        memcpy(&gamesNum, bodyPtr, 4);
                        bodyPtr += 4;
                        gameList.resize(gamesNum);
                        for(uint32_t g = 0; g < gamesNum; g++)
                        {
                            GameDesc& desc = gameList[g];
                            memcpy(&desc.id, bodyPtr, 4);
                            bodyPtr += 4;
                            desc.name = (char*)bodyPtr;
                            bodyPtr += desc.name.length() + 1;
                        }
                    }
                    else if(request->type == kRequestIcon)
                    {

                    }
                    request->succeed = true;
                }
                else
                {
                    request->succeed = false;
                }
                request->done = true;
            }
            else
            {
                printf("HttpRequest failed (error code %d)\n", httpRequest->get_error());
                request->succeed = false;
                request->done = true;
            }

            delete httpRequest;
        }
    }
}


void BSP_LCD_FillRect(Rect rect)
{
    rect.ClampByRect(GScreenRect);
    if(rect.Area())
        BSP_LCD_FillRect(rect.x, rect.y, rect.width, rect.height);
}


void DrawIcon(const GameDesc& desc)
{
    Rect rect = desc.uiRect;
    rect.ClampByRect(GScreenRect);
    if(rect.Area())
    {
        if(desc.iconState == kIconValid)
        {
            uint32_t pitch = kIconWidth * 4;
            BSP_LCD_DrawImage(rect.x, rect.y, rect.width, rect.height, desc.iconAddr, pitch);
        }
        else
        {
            BSP_LCD_FillRect(rect.x, rect.y, rect.width, rect.height);
        }        
    }
}


enum EMainScreenState
{
    kMainScreenReady = 0,
    kMainScreenWaitForNetwork,
    kMainScreenWaitGameList,
    kMainScreenLoadIcons,
    kMainScreenLoadGameCode,

    kMainScreenStatesCount
};


Request* SendGetGameList(TGameList* games)
{
    Request* request = GRequestsPool.alloc();
    request->type = kRequestGameList;
    request->done = false;
    request->succeed = false;
    request->responseData = (void*)games;
    GRequestsQueue.put(request);
    return request;
}


Request* SendLoadIcon(uint32_t gameId, uint8_t* iconAddr)
{
    Request* request = GRequestsPool.alloc();
    request->type = kRequestIcon;
    request->gameId = gameId;
    request->done = false;
    request->succeed = false;
    request->responseData = (void*)iconAddr;
    GRequestsQueue.put(request);
    return request;
}


Request* SendLoadGameCode(uint32_t gameId, uint8_t* codeAddr)
{
    Request* request = GRequestsPool.alloc();
    request->type = kRequestGameCode;
    request->gameId = gameId;
    request->done = false;
    request->succeed = false;
    request->responseData = (void*)codeAddr;
    GRequestsQueue.put(request);
    return request;
}


void MainScreen()
{
    TS_StateTypeDef tsState;

    Rect updateRect(20, 20, 40, 40);

    TGameList games;
    Request* request = NULL;
    EMainScreenState state = kMainScreenWaitForNetwork;

    uint8_t* iconsMem = GSdram;
    uint8_t* gameCodeMem = (uint8_t*)malloc(kMaxGameCodeSize);//iconsMem + kIconWidth * kIconHeight * 4 * kIconCacheSize;

    IconCache iconCache(iconsMem);
    uint32_t loadingIconForGameIdx = ~0u;

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

        // touch screen
        BSP_TS_GetState(&tsState);

        if(state == kMainScreenWaitForNetwork && GEthernet.get_connection_status() == NSAPI_STATUS_GLOBAL_UP)
        {
            printf("Connected to the network: '%s'\n", GEthernet.get_ip_address());
            request = SendGetGameList(&games);
            state = kMainScreenWaitGameList;
        }

        if(state == kMainScreenWaitGameList && request->done)
        {
            state = kMainScreenLoadIcons;
            GRequestsPool.free(request);
            request = NULL;
            printf("Received games list\n");

            uint32_t curCacheIdx = 0;
            for(uint32_t i = 0; i < games.size(); i++)
            {
                games[i].uiRect.x = i * (kIconWidth + 20) + 20;
                games[i].uiRect.y = 80;
                games[i].uiRect.width = kIconWidth;
                games[i].uiRect.height = kIconHeight;

                Rect rect = games[i].uiRect;
                rect.ClampByRect(GScreenRect);
                if(rect.Area() && curCacheIdx < kIconCacheSize)
                {
                    games[i].iconAddr = iconCache.cache[curCacheIdx].addr;
                    games[i].iconState = kIconInvalid;
                    iconCache.cache[curCacheIdx].gameIndex = i;
                    curCacheIdx++;
                }
                else
                {
                    games[i].iconAddr = NULL;
                    games[i].iconState = kIconInvalid;
                }
            }
        }

        if(state == kMainScreenLoadIcons)
        {
            if(loadingIconForGameIdx != ~0u && request->done)
            {
                GRequestsPool.free(request);
                request = NULL;
                games[loadingIconForGameIdx].iconState = kIconValid;
                loadingIconForGameIdx = ~0u;
            }

            if(loadingIconForGameIdx == ~0u)
            {
                for(uint32_t i = 0; i < games.size(); i++)
                {
                    if(games[i].iconAddr && games[i].iconState == kIconInvalid && !request)
                    {
                        request = SendLoadIcon(games[i].id, games[i].iconAddr);
                        games[i].iconState = kIconLoading;
                        loadingIconForGameIdx = i;
                        break;
                    }
                }
            }

            if(loadingIconForGameIdx == ~0u)
                state = kMainScreenReady;
        }

        uint32_t selectedGame = ~0u;
        for (uint8_t i = 0; i < tsState.touchDetected; i++)
        {
            /*if(tsState.touchEventId[i] != TOUCH_EVENT_PRESS_DOWN)
                continue;*/

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
                games.clear();
                iconCache.Clear();
                request = SendGetGameList(&games);
                state = kMainScreenWaitGameList;
                printf("Requested games list\n");
            }
        }

        if(state == kMainScreenReady && selectedGame != ~0u)
        {
            request = SendLoadGameCode(games[selectedGame].id, gameCodeMem);
            state = kMainScreenLoadGameCode;
            printf("Requested game code: %x\n", games[selectedGame].id);
        }

        if(state == kMainScreenLoadGameCode && request->done)
        {
            GRequestsPool.free(request);
            request = NULL;
            ScopedRamExecutionLock make_ram_executable;
            TGameMain gameMain;
            gameMain = (TGameMain)&gameCodeMem[1];
            gameMain(&GAPIImpl);
        }

        // rendering
        if(state == kMainScreenLoadGameCode)
        {
            BSP_LCD_Clear(LCD_COLOR_BLACK);
            BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
            BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
            BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t*)"Loading...", CENTER_MODE);
        }
        else
        {
            BSP_LCD_Clear(LCD_COLOR_DARKBLUE);

            BSP_LCD_SetTextColor(LCD_COLOR_LIGHTCYAN);
            BSP_LCD_FillRect(updateRect);

            if(state != kMainScreenReady)
            {
                BSP_LCD_SetTextColor(progressColor);
                BSP_LCD_FillRect(440, 20, 40, 40);
                uint32_t b = progressColor & 255;
                b = (b + 1) & 255;
                progressColor = 0xFF000000 | ((255 - b) << 8) | b;
            }

            for(uint32_t g = 0; g < games.size(); g++)
                DrawIcon(games[g]);
        }
    }
}


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

    GScreenRect = Rect(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}


void InitNetwork()
{
    GEthernet.set_network("192.168.1.5", "255.255.255.0", "192.168.1.1");
    GEthernet.set_blocking(false);
    GEthernet.connect();   

    GHttpThread.start(HttpThread);
}

 
int main()
{  
	led1 = 1;

    InitDisplay();
    InitNetwork();
    MainScreen();
}
