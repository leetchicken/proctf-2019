# Game Console

The service is a game console with a public SDK. The service consists of a piece of hardware, the console itself, given to each team and one server on the jury side serving HTTP API for all consoles.

Teams can play games on the console and even write their own games using [the public SDK](/services/game_console/SDK.md.html).

## Server HTTP API

### `GET /register?u=\<user name\>&p=\<password\>`

Registers a new user. When the console starts this is the first request it sends to the server. Username and password are stored on Quad SPI Flash. Username has to match the team's name, the default password is `00000000`.

### `GET /auth?u=\<user name\>&p=\<password\>`

Authenticates a user. Response contains 4 bytes long authentication key.

### `POST /change_password?auth=\<auth key\>&p=\<new password\>`

Changes the password for an authenticated user.

### `GET /list?auth=\<auth key\>`

Returns game list. List is an array of the following structs:

```c++
struct 
{
    uint32_t gameId;
    uint32_t numberOfAssets;
    <variable_length_string> gameName;
};
```

### `GET /icon?auth=\<auth key\>&id=\<game id\>`

Returns an icon for a game with the specified id.

### `GET /asset?auth=\<auth key\>&id=\<game id\>&index=\<asset index\>`

Returns an asset with the specified index for a game with the specified id. Asset index must be less that the value of the `numberOfAssets` field in the corresponding structure.

### `GET /code?auth=\<auth key\>&id=\<game id\>`

Returns the executable file for a game with the specified id.

### `GET /notification?auth=\<auth key\>`

Returns the next notification available for this console. A notification is a following struct:

```c++
struct 
{
    uint32_t userNameLength;
    <variable_length_string> userName;
    uint32_t messageLength;
    <variable_length_string> message;
};
```

### `POST /notification?auth=\<auth key\>`

Sends a notification to other consoles.

## TCP Connections

There are 2 raw TCP connections between the console and the server. Console establishes these connection upon startup and then keeps them alive. The server uses the first TPC connection on port 8001 to provide the information about incoming notifications to the console. Checksystem uses the second connection on port 8002 to identify the console.

### Notifications

Server periodically sends 16 bytes with the information about incoming notifications. First 4 bytes contain the number of notifications available. Next 12 bytes are random to make reverse engineering harder. In response the console sends back 16 bytes which are the permutation of the bytes received from the server, similarly to make reverse engineering harder.

If the number of available notifications is not zero, console fetches them one by one using HTTP API and draws them on the screen.

## Console

The console is based on STM32F746G Discovery kit and Mbed OS. An executable in the internal flash memory contains Mbed OS and some auxiliary code. This executable is about 200KB in size. It's expected that due to the size of the executable, teams won't be reversing it and will look at the Quad-SPI Flash memory instead.

Quad-SPI Flash memory contains FAT file system. On the file system, there are files with username and password, few images and `code.bin` executable with the main screen application.

Main screen application is built using the same public SDK for writing games and its responsibilities include:

- Rendering main screen
- Interacting with the server
- Loading games, icons and assets from the server
- Managing game execution
- Managing notifications

## Vuln

The vulnerability is hidden in the main screen application, in the notification system. There are no boundary checks for the notification message size. Because of this, during processing the response of the `GET /notification` request, if the notification message size is 280 or greater, the message overflows the dedicated buffer and overrides the return address of the current function on the stack.

Exploiting this vulnerability is made easier by the fact that Mbed OS doesn't have memory randomization. Additionally, some debug information is written to the console on startup. This information can be used to put together an exploit. To get even more information, a hacker can use a debugger to peek at the `sp` register value.

In order to use the vulnerability to steal flags, hacker can for example [as presented in this repository](/sploits/game_console) load a shell which reads the authentication key and sends it to a remote server.

### Fixing

The main screen application doesn't actually need to store the message on the stack: the message is immediately written to permanent memory after being written to the stack. Removing redundant writing message to the stack fixes the vulnerability.