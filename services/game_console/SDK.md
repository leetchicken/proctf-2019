# Welcome to Hackerdom Game Console SDK

Hackerdom Game Console is a game console with open and public available SDK. Our console is based on STM32F746G Discovery kit(https://www.st.com/en/evaluation-tools/32f746gdiscovery.html) with Mbed OS© as embedded operating system(https://www.mbed.com/en/platform/mbed-os/). 
Console provides possibilities to develop 2D games and various applications. For this SDK provides you data structures and an API with a set of functions, see 'api.h'. For development you also need GCC ARM compiler toolchain. 

## SDK structure
~~~~
 -api.h - API and data structures
 -SDKExample0/ - basic example of console application, also shows you how to work with LCD
 -SDKExample1/ - example which shows you how to work with touch screen
 -SDKExample2/ - example which shows you how to work with game assets
~~~~

## Limitations
At this moment Console and SDK are in a early stage of development. This means that you have access to hardware and SDK, you can write your games/applications and compile it. But there is no official way to launch those games/applications on console without submission

## Development
Game code:
+ must include api.h and nothing mode
+ must not use any standart function, only API
+ must not declare non constant global variables
+ Game code must declare 2 functions:
~~~~
void* GameInit(API* api, uint8_t* sdram)
~~~~
GameInit is called only once on game startup. Before GameInit call all game assets and game code are loaded. sdram is a pointer to the free region of SDRAM, you can use this memory as you wish. GameInit returns pointer, this pointer will be passed to second argument of GameUpdate
~~~~
bool GameUpdate(API* api, void* ctxVoid)
~~~~
GameUpdate is called every frame. In this function you are supposed to update your game logic and draw something on the screen. Return false if you want to terminate game execution.

## Submission
To submit your game, put 
+ source coude
+ make file
+ icon, only PNG is supported
+ assets, if needed, only PNG is supported

in to zip archive and send it to somewhere@domain.com

After moderation you will be informed about submission results. Your submit might be failed if:

+ there are some files missed in zip archive
+ you are doing something suspicious, illegal in your code
+ icon size is not equal to 172x172
+ at least one of the assets is bigger that 480x272
+ icon or assets in not PNG file

After successful submission all gamers will be informed about new game