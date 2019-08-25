# Game Console
Service is a game console with public available SDK. Service consists of hardware game console, each team has one, and server which serves all consoles. 
Console interacts with server by HTTP to register user, to authenticate, to request games list, game code and assets, notifications. 

## HTTP methods:

### GET /register?u=\<user name\>&p=\<password\>
Register user on the server. This is first request on console's startup. User name and password are stored in the files on Quad SPI Flash. User name has to match team name, defaut password is '00000000'


### GET /auth?u=\<user name\>&p=\<password\>
Authenticates user on the server. In response server sends 4 bytes long authentication key, which used in all other HTTP requests


### POST /change_password?auth=\<auth key\>&p=\<new password\>
Changes password for authenticated user


### GET /list?auth=\<auth key\>
Requests games list. List is an array of structures:
~~~~
struct 
{
    uint32_t gameId;
    uint32_t numberOfAssets;
    <variable length string> gameName;
};
~~~~

### GET /icon?auth=\<auth key\>&id=\<game id\>
Requests icon for game with specified id

### GET /asset?auth=\<auth key\>&id=\<game id\>&index=\<asset index\>
Request asset with specified index for game with specified id. Asset index must be less that 'numberOfAssets'

### GET /code?auth=\<auth key\>&id=\<game id\>
Requests code for game with specified id

### GET /notification?auth=\<auth key\>
Requests notification available for this console. Notification sent by server in binary form:
~~~~
struct 
{
    uint32_t userNameLength;
    <variable length string> userName;
    uint32_t messageLength;
    <variable length string> message;
};
~~~~

### POST /notification?auth=\<auth key\>
Console can send notification to server by using this request. Server broadcasts this notification to all other consoles.


## TCP Connections
Besides HTTP there are 2 raw TCP connections between each console and server. Both of them established on console's startup and kept in connected state in runtime. 

### Notifications
First TCP connection(port 8001) server use to notificate console that there notifications available for this console on the server. Server sends 16 bytes, first 4 bytes contains number of notifications available. Next 12 bytes is just a random(we use it just to make reverse harder). Console on response sends another 16 bytes. This 16 bytes is a result of some bytes juggling of 16 bytes from the server, again, just to make reverse harder. If the number of available notifications is not zero, console requests them one by one using GET /notification and draw them one by one on the screen.

### Checksystem
Another TCP connection(port 8002) is a part of checksystem. We use it to identify hardware console

## Console architecture
Console is based on STM32F746G Discovery kit and Mbed OS. Binary in the internal flash memory contains Mbed OS, implementaion of API and thread that maintance checksystem's TCP connection. Binary is about 200KB in size. It was expected that teams wont reverse it because of its size, and will take a look to Quad-SPI Flash. Quad-SPI Flash memory contains FAT file system. On this filesystem there are few bmp files, files contains user name and password, and code.bin. code.bin is binary which contains code of 'Main screen application'. This application is built like an ordinary game, ie uses API for operation. Its responsible for interacting with server by HTTP and TCP, its renders main screen, loads icons, game code and assets and manages game execution, manages notifications. Vuln is hidden in it.

## Vuln
vulnerability is in notification system. Because of the lack of stack and code memory randomization, boundary checks there are buffer overflow followed by RCE is possible. This is happen while processing response of GET /notification. Notification must be 280 bytes long to overwrite return address. To make the hack process easier, console prints some useful addresses on startup. Also 'sp' register value might be useful. It can be retrived by using debugger. Our exploit loads shell. This shell steals auth key and send it to specified server by TCP. See exploit for more information

### How to defend
For some reason developer write something strange in GET /notification response processing. First he copies response data in to temporary memory on stack without boundary checks, and right after that copies from temporary to permanent. So to patch you just need to remove redudant copy to temporary buffer and copy response data to permanent memory.
