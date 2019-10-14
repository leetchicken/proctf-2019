#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>

#include "db.h"
#include "debug.h"
#include "malloc_debug.h"
#include "protocol.h"

#define DEFAULT_PORT   5555

#define sys_fatal(message)  { perror(message); exit(1); }
#define app_fatal(message) { fprintf(stderr, "%s\n",message); exit(1); }

int create_server_socket (int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        sys_fatal("create socket");
    }

    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    struct sockaddr_in name;
    name.sin_family = AF_INET;
    name.sin_addr.s_addr = INADDR_ANY;
    name.sin_port = htons(port);

    if (bind(sock, (void *) &name, sizeof(name)) < 0)
    {
        sys_fatal("bind");
    }

    if (listen(sock, 1) < 0)
    {
        sys_fatal("listen");
    }

    LOG("Listening to port: %d\n", port);
    return sock;
}

void process_client()
{
    init_client_state();
    print_greeting();

    while (1)
    {
        handle_request();
    }
}

void test_db_connection()
{
    int connect = db_connect();
    if (connect != 0)
    {
        app_fatal("db connection failed");
    }
    db_disconnect();
}

int main(int argc, char **argv)
{
#ifdef DEBUG
    // setup_malloc_hook();
#endif

    signal(SIGCHLD, SIG_IGN);
    signal(SIGUSR1, SIG_IGN);
    test_db_connection();

    int port = DEFAULT_PORT;
    if (argc >= 2)
    {
        port = atoi(argv[1]);
    }
    int server_socket = create_server_socket(port);

    while (1)
    {
        struct sockaddr client_address;
        int address_len = sizeof(client_address);
        int client_socket = accept(server_socket, &client_address, &address_len);
        if (client_socket < 0)
        {
            sys_fatal("accept");
        }

        int pid = fork();
        if (pid < 0)
        {
            sys_fatal("fork");
        }

        if (pid == 0)
        {
            if (dup2(client_socket, STDIN_FILENO) < 0)
            {
                sys_fatal("dup2 stdin");
            }
            if (dup2(client_socket, STDOUT_FILENO) < 0)
            {
                sys_fatal("dup2 stdout");
            }

            setvbuf(stdout, 0, _IONBF, 0);

            LOG("  [%d] process started\n", getpid());

            if (db_connect() == 0)
            {
                process_client();
                db_disconnect();
            }
            else
            {
                puts("DB connection problem, sorry");
                LOG("  [%d] cannot connect to db\n", getpid());
            }

            LOG("  [%d] process finished\n", getpid());

            shutdown(client_socket, 2);
            exit(0);
        }
        close(client_socket);
    }
    close(server_socket);
    return 0;
}
