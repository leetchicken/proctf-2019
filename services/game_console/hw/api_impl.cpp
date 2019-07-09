#include "api_impl.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "http_request.h"
#include "MemoryPool.h"
#include "Queue.h"


static const char* kServerAddr = "192.168.1.1:8000";
Thread GHttpThread;
MemoryPool<ServerRequest, 64> GRequestsPool;
Queue<ServerRequest, 64> GRequestsQueue;


uint8_t* GHttpBodyCallbackPtr = NULL;
void HttpBodyCallback(const char* data, uint32_t data_len) 
{
    memcpy(GHttpBodyCallbackPtr, data, data_len);
    GHttpBodyCallbackPtr += data_len;
}


void HttpThread(EthernetInterface* ethInterface)
{
    while(true)
    {
        osEvent evt = GRequestsQueue.get();
        if (evt.status == osEventMessage) 
        {
            ServerRequest* request = (ServerRequest*)evt.value.p;
            char url[64];
            sprintf(url, "http://%s/%s", kServerAddr, request->url);

            printf("Request: %s\n", url);
            bool needBodyCallback = request->responseData != NULL;
            GHttpBodyCallbackPtr = (uint8_t*)request->responseData;
            HttpRequest* httpRequest = needBodyCallback ? new HttpRequest(ethInterface, HTTP_GET, url, HttpBodyCallback) 
                                                        : new HttpRequest(ethInterface, HTTP_GET, url);
            HttpResponse* httpResponse = httpRequest->send();
            if (httpResponse) 
            {
                printf("Response code: %d\n", httpResponse->get_status_code());
                if(!request->responseData)
                    request->responseData = httpResponse->get_body();
                request->succeed = httpResponse->get_status_code() == 200;
                request->done = true;
            }
            else
            {
                printf("HttpRequest failed (error code %d)\n", httpRequest->get_error());
                request->succeed = false;
                request->done = true;
            }

            request->internalData = (void*)httpRequest;
        }
    }
}


APIImpl::APIImpl()
{
    m_curFrameBuffer = 0;
    m_ethInterface = NULL;
    m_sdram = NULL;
    m_timer.start();
    m_displayIsOn = true;
}


void APIImpl::Init(EthernetInterface* ethInterface, uint8_t* sdram)
{
    m_curFrameBuffer = 0;
    m_ethInterface = ethInterface;
    m_sdram = sdram;
    GHttpThread.start(callback(HttpThread, m_ethInterface));
}


uint8_t* APIImpl::GetSDRam()
{
    return m_sdram;
}


void* APIImpl::Malloc(uint32_t size)
{
    return malloc(size);
}


void APIImpl::Free(void* ptr)
{
    free(ptr);
}


void APIImpl::GetScreenRect(Rect* rect)
{
    *rect = Rect(0, 0, BSP_LCD_GetXSize(), BSP_LCD_GetYSize());
}


void APIImpl::GetTouchScreenState(TouchScreenState* state)
{
    TS_StateTypeDef bspState;
    BSP_TS_GetState(&bspState);

    state->touchDetected = bspState.touchDetected;
    memcpy(state->touchX, bspState.touchX, sizeof(state->touchX));
    memcpy(state->touchY, bspState.touchY, sizeof(state->touchY));
    memcpy(state->touchWeight, bspState.touchWeight, sizeof(state->touchWeight));
    memcpy(state->touchEventId, bspState.touchEventId, sizeof(state->touchEventId));
    memcpy(state->touchArea, bspState.touchArea, sizeof(state->touchArea));
}


ENetwokConnectionStatus APIImpl::GetNetwokConnectionStatus()
{
    switch(m_ethInterface->get_connection_status())
    {
        case NSAPI_STATUS_LOCAL_UP:
            return kNetwokConnectionStatusLocalUp;
        case NSAPI_STATUS_GLOBAL_UP:
            return kNetwokConnectionStatusGlobalUp;
        case NSAPI_STATUS_DISCONNECTED:
            return kNetwokConnectionStatusDisconnected;
        case NSAPI_STATUS_CONNECTING:
            return kNetwokConnectionStatusConnecting;
        default:
            return kNetwokConnectionStatusUnkown;
    }
    return kNetwokConnectionStatusUnkown;
}


const char* APIImpl::GetIPAddress()
{
    return m_ethInterface->get_ip_address();
}


ServerRequest* APIImpl::AllocServerRequest()
{
    ServerRequest* request = GRequestsPool.alloc();
    memset(request, 0, sizeof(ServerRequest));
    return request;
}


bool APIImpl::SendServerRequest(ServerRequest* request)
{
    return GRequestsQueue.put(request) == osOK;
}


void APIImpl::FreeServerRequest(ServerRequest* request)
{
    HttpRequest* httpRequest = (HttpRequest*)request->internalData;
    delete httpRequest;
    GRequestsPool.free(request);
}


void APIImpl::SwapFramebuffer()
{
    // wait for vsync
    while (!(LTDC->CDSR & LTDC_CDSR_VSYNCS));
    // swap buffers
    BSP_LCD_SetLayerVisible(m_curFrameBuffer, DISABLE);
    BSP_LCD_SelectLayer(m_curFrameBuffer);       
    m_curFrameBuffer = (m_curFrameBuffer + 1) % 2;
    BSP_LCD_SetLayerVisible(m_curFrameBuffer, ENABLE); 
}


void APIImpl::LCD_OnOff(bool onOff)
{
    if(onOff)
        BSP_LCD_DisplayOn();
    else
        BSP_LCD_DisplayOff();
    
    m_displayIsOn = onOff;
}


bool APIImpl::LCD_IsOn()
{
    return m_displayIsOn;
}


void APIImpl::LCD_Clear(uint32_t color)
{
    BSP_LCD_Clear(color);
}


void APIImpl::LCD_SetBackColor(uint32_t color)
{
    BSP_LCD_SetBackColor(color);
}


void APIImpl::LCD_SetTextColor(uint32_t color)
{
    BSP_LCD_SetTextColor(color);
}


void APIImpl::LCD_DisplayStringAt(uint16_t xpos, uint16_t ypos, const char* text, ETextAlignMode mode)
{
    Text_AlignModeTypdef bspMode = CENTER_MODE;
    switch(mode)
    {
        case kTextAlignCenter:
            bspMode = CENTER_MODE;
            break;
        case kTextAlignLeft:
            bspMode = LEFT_MODE;
            break;
        case kTextAlignRight:
            bspMode = RIGHT_MODE;
            break;
    }
    BSP_LCD_DisplayStringAt(xpos, ypos, (uint8_t*)text, bspMode);
}


void APIImpl::LCD_FillRect(const Rect& rect, uint32_t color)
{
    BSP_LCD_FillRect(rect.x, rect.y, rect.width, rect.height, color);
}


void APIImpl::LCD_DrawImage(const Rect& rect, uint8_t* image, uint32_t pitch)
{
    BSP_LCD_DrawImage(rect.x, rect.y, rect.width, rect.height, image, pitch, 0);
}


void APIImpl::LCD_DrawImageWithBlend(const Rect& rect, uint8_t* image, uint32_t pitch)
{
    BSP_LCD_DrawImage(rect.x, rect.y, rect.width, rect.height, image, pitch, 1);
}


void* APIImpl::fopen(const char* filename, const char* mode)
{
    return (void*)::fopen(filename, mode);
}


uint32_t APIImpl::fread(void* buffer, uint32_t size, void* file)
{
    return ::fread(buffer, 1, size, (FILE*)file);
}


uint32_t APIImpl::fwrite(const void* buffer, uint32_t size, void* file)
{
    return ::fwrite(buffer, 1, size, (FILE*)file);
}


uint32_t APIImpl::fsize(void* file)
{
    size_t curPos = ftell((FILE*)file);
    fseek((FILE*)file, 0, SEEK_END);
    size_t size = ftell((FILE*)file);
    fseek((FILE*)file, curPos, SEEK_SET);
    return size;
}


void APIImpl::fclose(void* file)
{
    ::fclose((FILE*)file);
}


void* APIImpl::memcpy(void* dst, const void* src, uint32_t size)
{
    return ::memcpy(dst, src, size);
}


uint32_t APIImpl::strlen(const char* str)
{
    return ::strlen(str);
}


void APIImpl::strcpy(char* dst, const char* src)
{
    ::strcpy(dst, src);
}


void APIImpl::sprintf(char* str, const char* formatStr, ...)
{
    va_list args;
    va_start(args, formatStr);
    vsprintf(str, formatStr, args);
    va_end(args);
}


void APIImpl::printf(const char* formatStr, ...)
{
    va_list args;
    va_start(args, formatStr);
    vprintf(formatStr, args);
    va_end(args);
}


void APIImpl::sleep(float t)
{
    wait(t);
}


float APIImpl::time()
{
    return (float)m_timer.read_ms() / 1000.0f;
}