#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <limits.h>

#include "common.h"
#include "libfactorization.h"
#include "thpool.h"
#include "messageprocessor.h"
#include "serverconnection.h"

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif
#define SERVER_THPOOL_SIZE 10

const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;
int server_socket_fd = -1;
threadpool server_thpool;

static void signal_handler(int signal_number);

static void check_parameters(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "p:h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("factorization_server -p [port, default %s]\n", DEFAULT_PORT_STR);
            exit(0);
            break;
        case 'p':
            port = parsePort(optarg);
            if (port < 0)
            {
                fprintf(stderr, "invalid port: %s\n", argv[optind]);
                exit(1);
            }
            break;
        }
    }
}

static void start_server()
{
    D(printf("start_server\n"));

    server_thpool = thpool_init(SERVER_THPOOL_SIZE);
    int new_socket_fd;
    struct sockaddr_in address;
    serverconnection_arg_t *serverconnection_arg;
    socklen_t client_address_len;

    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    if (bind(server_socket_fd, (struct sockaddr *)&address, sizeof address) == -1)
    {
        perror("bind");
        exit(1);
    }

    if (listen(server_socket_fd, 0) == -1)
    {
        perror("listen");
        exit(1);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    if (signal(SIGTERM, signal_handler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }
    if (signal(SIGINT, signal_handler) == SIG_ERR)
    {
        perror("signal");
        exit(1);
    }

    for (;;)
    {
        serverconnection_arg = (serverconnection_arg_t *)malloc(sizeof *serverconnection_arg);
        if (!serverconnection_arg)
        {
            perror("malloc");
            continue;
        }

        client_address_len = sizeof serverconnection_arg->client_address;
        new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&serverconnection_arg->client_address, &client_address_len);
        if (new_socket_fd == -1)
        {
            perror("accept");
            free(serverconnection_arg);
            continue;
        }

        serverconnection_arg->new_socket_fd = new_socket_fd;
        if (thpool_num_threads_working(server_thpool) >= SERVER_THPOOL_SIZE)
        {
            fprintf(stderr, "cannot handle client request, max num of dispatching clients reached");
            close(new_socket_fd);
            free(serverconnection_arg);
            continue;
        }

        if (thpool_add_work(server_thpool, serverconnection_handler, (void *)serverconnection_arg))
        {
            fprintf(stderr, "cannot handle client request, error adding work to thread pool");
            close(new_socket_fd);
            free(serverconnection_arg);
            continue;
        }
    }
}

static void signal_handler(int signal_number)
{
    D(printf("signal_handler: signal %d\n", signal_number));

    D(printf("signal_handler: start destroy threads\n"));
    thpool_destroy(server_thpool);

    if (server_socket_fd != -1)
    {
        D(printf("signal_handler: closing server server_socket_fd\n"));
        close(server_socket_fd);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    check_parameters(argc, argv);
    printf("port: %d\n", port);
    start_server();
    return 0;
}
