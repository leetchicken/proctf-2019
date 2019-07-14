#include "api_impl.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "http_request.h"
#include "MemoryPool.h"
#include "Queue.h"
#include "ip4string.h"


Thread GHttpThread;
MemoryPool<HTTPRequest, 64> GRequestsPool;
Queue<HTTPRequest, 64> GRequestsQueue;


uint8_t* GHttpBodyCallbackPtr = NULL;
uint32_t GHttpBodySize = 0;
void HttpBodyCallback(const char* data, uint32_t data_len) 
{
    memcpy(GHttpBodyCallbackPtr, data, data_len);
    GHttpBodyCallbackPtr += data_len;
    GHttpBodySize += data_len;
}


static http_method ConvertHttpMethod(EHttpMethod m)
{
    if(m == kHttpMethodGet)
        return HTTP_GET;
    else if(m == kHttpMethodPost)
        return HTTP_POST;

    return HTTP_GET;
}


void HttpThread(EthernetInterface* ethInterface)
{
    while(true)
    {
        osEvent evt = GRequestsQueue.get();
        if (evt.status == osEventMessage) 
        {
            HTTPRequest* request = (HTTPRequest*)evt.value.p;
            printf("Request: %s\n", request->url);
            bool needBodyCallback = request->responseData != NULL;
            GHttpBodyCallbackPtr = (uint8_t*)request->responseData;
            GHttpBodySize = 0;
            http_method httpMethod = ConvertHttpMethod(request->httpMethod);
            HttpRequest* httpRequest = needBodyCallback ? new HttpRequest(ethInterface, httpMethod, request->url, HttpBodyCallback) 
                                                        : new HttpRequest(ethInterface, httpMethod, request->url);
            HttpResponse* httpResponse = httpRequest->send(request->requestBody, request->requestBodySize);
            if (httpResponse) 
            {
                printf("Response code: %d\n", httpResponse->get_status_code());
                if(!request->responseData)
                {
                    request->responseData = httpResponse->get_body();
                    request->responseDataSize = httpResponse->get_body_length();
                }
                else
                {
                    request->responseDataSize = GHttpBodySize;
                }
                request->statusCode = httpResponse->get_status_code();
                request->succeed = httpResponse->get_status_code() == 200;
                request->done = true;
            }
            else
            {
                printf("Http request failed (error code %d)\n", httpRequest->get_error());
                request->statusCode = 0;
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
    memset(m_tcpSockets, 0, sizeof(m_tcpSockets));
    m_freeTcpSockets = ~0u;
    m_acceptedSockets = 0;
    memset(m_udpSockets, 0, sizeof(m_udpSockets));
    m_freeUdpSockets = ~0u;
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


HTTPRequest* APIImpl::AllocHTTPRequest()
{
    HTTPRequest* request = GRequestsPool.alloc();
    memset(request, 0, sizeof(HTTPRequest));
    return request;
}


bool APIImpl::SendHTTPRequest(HTTPRequest* request)
{
    return GRequestsQueue.put(request) == osOK;
}


void APIImpl::FreeHTTPRequest(HTTPRequest* request)
{
    HttpRequest* httpRequest = (HttpRequest*)request->internalData;
    delete httpRequest;
    GRequestsPool.free(request);
}


uint32_t APIImpl::aton(const char* ip)
{
    uint32_t ret;
    SocketAddress addr(ip);
    memcpy(&ret, addr.get_ip_bytes(), 4);
    return ret;
}


void APIImpl::ntoa(uint32_t ip, char* ipStr)
{
    ip4tos(&ip, ipStr);
}


static int GetSocketIdx(int socket)
{
    if(socket < 32)
        return socket;
    else
        return socket - 32;
    return -1;
}


static bool IsTcpSocket(int socket)
{
    if(socket < 32)
        return true;
    else
        return false;
    return false;
}


static int ConvertSocketRetVal(nsapi_size_or_error_t sizeOrError)
{
    switch(sizeOrError)
    {
        case NSAPI_ERROR_OK:
            return kSocketErrorOk;
        case NSAPI_ERROR_WOULD_BLOCK:
            return kSocketErrorWouldBlock;
        case NSAPI_ERROR_UNSUPPORTED:
            return kSocketErrorUnsupported;    
        case NSAPI_ERROR_PARAMETER:
            return kSocketErrorParameter;   
        case NSAPI_ERROR_NO_CONNECTION:
            return kSocketErrorNoConnection;     
        case NSAPI_ERROR_NO_SOCKET:
            return kSocketErrorNoSocket;  
        case NSAPI_ERROR_NO_ADDRESS:
            return kSocketErrorNoAddress;     
        case NSAPI_ERROR_NO_MEMORY:
            return kSocketErrorNoMemory;
        case NSAPI_ERROR_DNS_FAILURE:
            return kSocketErrorDnsFailure;   
        case NSAPI_ERROR_DEVICE_ERROR:
            return kSocketErrorDeviceError;   
        case NSAPI_ERROR_IN_PROGRESS:
            return kSocketErrorInProgress;   
        case NSAPI_ERROR_ALREADY:
            return kSocketErrorAlready;   
        case NSAPI_ERROR_IS_CONNECTED:
            return kSocketErrorIsConnected;      
        case NSAPI_ERROR_CONNECTION_LOST:
            return kSocketErrorConnectionLost;  
        case NSAPI_ERROR_CONNECTION_TIMEOUT:
            return kSocketErrorConnectionTimeout;
    }

    if(sizeOrError < 0)
        return kSocketErrorUnsupported;

    return sizeOrError;
}


int APIImpl::socket(bool tcp)
{
    if(tcp)
    {
        if(m_freeTcpSockets == 0)
            return kSocketErrorNoMemory;
        int sockIdx = __builtin_ctz(m_freeTcpSockets);
        m_freeTcpSockets &= ~(1 << sockIdx);
        m_tcpSockets[sockIdx] = new TCPSocket();
        int err = m_tcpSockets[sockIdx]->open(m_ethInterface);
        if(err < 0)
            return ConvertSocketRetVal(err);
        return sockIdx;
    }
    else
    {
        if(m_freeUdpSockets == 0)
            return kSocketErrorNoMemory;
        int sockIdx = __builtin_ctz(m_freeUdpSockets);
        m_freeUdpSockets &= ~(1 << sockIdx);
        m_udpSockets[sockIdx] = new UDPSocket();
        int err = m_udpSockets[sockIdx]->open(m_ethInterface);
        if(err < 0)
            return ConvertSocketRetVal(err);
        return sockIdx + 32;
    }

    return kSocketErrorParameter;
}


int APIImpl::set_blocking(int socket, bool blocking)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        m_tcpSockets[socketIdx]->set_blocking(blocking);
        return kSocketErrorOk;
    }
    else
    {
        m_udpSockets[socketIdx]->set_blocking(blocking);
        return kSocketErrorOk;
    }

    return kSocketErrorParameter;
}


int APIImpl::send(int socket, const void* data, uint32_t size, NetAddr* addr)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        int ret = m_tcpSockets[socketIdx]->send(data, size);
        return ConvertSocketRetVal(ret);
    }
    else
    {
        if(!addr)
            return kSocketErrorParameter;
        SocketAddress sockAddr(&addr->ip, NSAPI_IPv4, addr->port);
        int ret = m_udpSockets[socketIdx]->sendto(sockAddr, data, size);
        return ConvertSocketRetVal(ret);
    }

    return kSocketErrorParameter;
}


int APIImpl::recv(int socket, void* data, uint32_t size, NetAddr* addr)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        int ret = m_tcpSockets[socketIdx]->recv(data, size);
        return ConvertSocketRetVal(ret);
    }
    else
    {
        SocketAddress sockAddr;
        int ret = m_udpSockets[socketIdx]->recvfrom(&sockAddr, data, size);
        if(addr)
        {
            memcpy(&addr->ip, sockAddr.get_ip_bytes(), 4);
            addr->port = sockAddr.get_port();
        }
        return ConvertSocketRetVal(ret);
    }
    
    return kSocketErrorParameter;
}


int APIImpl::connect(int socket, const NetAddr& addr)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        SocketAddress sockAddr(&addr.ip, NSAPI_IPv4, addr.port);
        int ret = m_tcpSockets[socketIdx]->connect(sockAddr);
        return ConvertSocketRetVal(ret);
    }

    return kSocketErrorUnsupported;
}


int APIImpl::bind(int socket, uint32_t ip, uint16_t port)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    SocketAddress sockAddr(&ip, NSAPI_IPv4, port);
    if(IsTcpSocket(socket))
    {
        int ret = m_tcpSockets[socketIdx]->bind(sockAddr);
        return ConvertSocketRetVal(ret);
    }
    else
    {
        int ret = m_udpSockets[socketIdx]->bind(sockAddr);
        return ConvertSocketRetVal(ret);
    }
    
    return kSocketErrorParameter;
}


int APIImpl::listen(int socket, int backlog)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        int ret = m_tcpSockets[socketIdx]->listen(backlog);
        return ConvertSocketRetVal(ret);
    }
    
    return kSocketErrorUnsupported;
}


int APIImpl::accept(int socket)
{
    if(socket < 0)
        return kSocketErrorParameter;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        nsapi_error_t err = 0;
        TCPSocket* newSock = m_tcpSockets[socketIdx]->accept(&err);
        if(err < 0)
            return ConvertSocketRetVal(err);

        if(m_freeTcpSockets == 0)
        {
            newSock->close();
            return kSocketErrorNoMemory;
        }
        int newSocketIdx = __builtin_ctz(m_freeTcpSockets);
        m_freeTcpSockets &= ~(1 << newSocketIdx);  
        m_acceptedSockets |= 1 << newSocketIdx;
        m_tcpSockets[newSocketIdx] = newSock;

        return newSocketIdx;
    }
    
    return kSocketErrorUnsupported;
}


int APIImpl::getpeername(int socket, NetAddr& addr)
{
    if(socket < 0)
        return kSocketErrorParameter;

    SocketAddress sockAddr;
    int err = 0;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
        err = m_tcpSockets[socketIdx]->getpeername(&sockAddr);
    else
        err = m_udpSockets[socketIdx]->getpeername(&sockAddr);

    if(err < 0)
        return ConvertSocketRetVal(err);

    memcpy(&addr.ip, sockAddr.get_ip_bytes(), 4);
    addr.port = sockAddr.get_port();
    return kSocketErrorOk;
}


void APIImpl::close(int socket)
{
    if(socket < 0)
        return;

    int socketIdx = GetSocketIdx(socket);
    if(IsTcpSocket(socket))
    {
        m_tcpSockets[socketIdx]->close();
        bool acceptedSock = (m_acceptedSockets & (1 << socketIdx)) > 0;
        if(!acceptedSock)
            delete m_tcpSockets[socketIdx];
        m_tcpSockets[socketIdx] = NULL;
        m_freeTcpSockets |= 1 << socketIdx;
        m_acceptedSockets &= ~(1 << socketIdx);  
        m_tcpSockets[socketIdx] = NULL;
    }
    else
    {
        m_udpSockets[socketIdx]->close();
        delete m_udpSockets[socketIdx];
        m_freeUdpSockets |= 1 << socketIdx;
        m_udpSockets[socketIdx] = NULL;
    }
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


void APIImpl::LCD_SetFont(EFont font)
{
    switch(font)
    {
        case kFont8:
            BSP_LCD_SetFont(&Font8);
            break;
        case kFont12:
            BSP_LCD_SetFont(&Font12);
            break;
        case kFont16:
            BSP_LCD_SetFont(&Font16);
            break;
        case kFont20:
            BSP_LCD_SetFont(&Font20);
            break;
        case kFont24:
            BSP_LCD_SetFont(&Font24);
            break;
    }
}


void APIImpl::LCD_GetFontInfo(EFont font, FontInfo* fontInfo)
{
    if(!fontInfo)
        return;

    switch(font)
    {
        case kFont8:
            fontInfo->charWidth = Font8.Width;
            fontInfo->charHeight = Font8.Height;
            break;
        case kFont12:
            fontInfo->charWidth = Font12.Width;
            fontInfo->charHeight = Font12.Height;
            break;
        case kFont16:
            fontInfo->charWidth = Font16.Width;
            fontInfo->charHeight = Font16.Height;
            break;
        case kFont20:
            fontInfo->charWidth = Font20.Width;
            fontInfo->charHeight = Font20.Height;
            break;
        case kFont24:
            fontInfo->charWidth = Font24.Width;
            fontInfo->charHeight = Font24.Height;
            break;
    }
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


void APIImpl::LCD_DisplayChar(uint16_t xpos, uint16_t ypos, char ascii)
{
    if(ascii < 0x20 || ascii > 0x7e)
        ascii = 0x20;
    BSP_LCD_DisplayChar(xpos, ypos, ascii);
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


bool APIImpl::GetButtonState()
{
    return BSP_PB_GetState(BUTTON_KEY) > 0;
}


void APIImpl::LedOnOff(bool on)
{
    if(on)
        BSP_LED_On(LED_GREEN);
    else
        BSP_LED_Off(LED_GREEN);
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


void* APIImpl::memset(void* dst, uint32_t val, uint32_t size)
{
    return ::memset(dst, val, size);
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
