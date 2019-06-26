#include "mbed.h"
#include "LCD_DISCO_F746NG.h"
#include "EthernetInterface.h"
#include "TCPServer.h"
#include "TCPSocket.h"
#include "api.h"
 
LCD_DISCO_F746NG lcd;
DigitalOut led1(LED1);
Thread thread;


class APIImpl : public API
{
public:
    void printf(const char* str)
    {
    	lcd.SetBackColor(LCD_COLOR_ORANGE);
		lcd.SetTextColor(LCD_COLOR_CYAN);
		lcd.DisplayStringAt(0, LINE(5), (uint8_t *)str, CENTER_MODE);
    }
};

typedef int (*TGameMain)(API*);

APIImpl GAPIImpl;


void NetworkThread()
{
    printf("Basic HTTP server example\n");
    
    EthernetInterface eth;
    eth.set_network("192.168.1.5", "255.255.255.0", "192.168.1.1");
    eth.connect();
    
    printf("The target IP address is '%s'\n", eth.get_ip_address());
    
    TCPServer srv;
    TCPSocket clt_sock;
    SocketAddress clt_addr;
    
    /* Open the server on ethernet stack */
    srv.open(&eth);
    
    /* Bind the HTTP port (TCP 80) to the server */
    srv.bind(eth.get_ip_address(), 80);
    
    /* Can handle 5 simultaneous connections */
    srv.listen(5);
    
    while (true) 
    {
        srv.accept(&clt_sock, &clt_addr);
        printf("accept %s:%d\n", clt_addr.get_ip_address(), clt_addr.get_port());
        int rcount = 0;

        uint8_t code[1024];
        rcount = clt_sock.recv(code, sizeof(code));
        if(rcount < 0)
        {
            clt_sock.close();
            continue;
        }

        ScopedRamExecutionLock make_ram_executable;
        TGameMain gameMain;
        gameMain = (TGameMain)&code[1];
        int retVal = gameMain(&GAPIImpl);

        char str[32];
        memset(str, 0, 32);
        sprintf(str, "%d", retVal);

        clt_sock.send(str, strlen(str));
        clt_sock.close();
    }
}

 
int main()
{  
	led1 = 1;

    thread.start(NetworkThread);
 
    lcd.DisplayStringAt(0, LINE(1), (uint8_t *)"MBED EXAMPLE", CENTER_MODE);
    wait(1);
  
    while(1)
    {
      lcd.Clear(LCD_COLOR_BLUE);
      lcd.SetBackColor(LCD_COLOR_BLUE);
      lcd.SetTextColor(LCD_COLOR_WHITE);
      wait(0.3);
      lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"DISCOVERY STM32F746NG", CENTER_MODE);
      wait(1);
 
      lcd.Clear(LCD_COLOR_GREEN);
      
      lcd.SetTextColor(LCD_COLOR_BLUE);
      lcd.DrawRect(10, 20, 50, 50);
      wait(0.1);
      lcd.SetTextColor(LCD_COLOR_BROWN);
      lcd.DrawCircle(80, 80, 50);
      wait(0.1);
      lcd.SetTextColor(LCD_COLOR_YELLOW);
      lcd.DrawEllipse(150, 150, 50, 100);
      wait(0.1);
      lcd.SetTextColor(LCD_COLOR_RED);
      lcd.FillCircle(200, 200, 40);
      wait(1);
 
      lcd.SetBackColor(LCD_COLOR_ORANGE);
      lcd.SetTextColor(LCD_COLOR_CYAN);
      lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"HAVE FUN !!!", CENTER_MODE);
      wait(1);
 
      led1 = !led1;
    }
}
