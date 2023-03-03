#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <limits.h>
#include "common.h"
#include "libfactorization.h"
#include "thpool.h"
#include "messageprocessor.h"

#define BACKLOG 10
#define SERVER_THPOOL_SIZE 10

const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;
int server_socket_fd = -1;
threadpool server_thpool;

typedef struct pthread_arg_t
{
    int new_socket_fd;
    struct sockaddr_in client_address;
    /* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

/* Thread routine to serve connection to client. */
static void client_handler(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
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
    printf("start_server\n");

    server_thpool = thpool_init(SERVER_THPOOL_SIZE);
    int new_socket_fd;
    struct sockaddr_in address;
    pthread_attr_t pthread_attr;
    pthread_arg_t *pthread_arg;
    pthread_t pthread;
    socklen_t client_address_len;

    /* Initialise IPv4 address. */
    memset(&address, 0, sizeof address);
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    /* Create TCP socket. */
    if ((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* Bind address to socket. */
    if (bind(server_socket_fd, (struct sockaddr *)&address, sizeof address) == -1)
    {
        perror("bind");
        exit(1);
    }

    /* Listen on socket. */
    if (listen(server_socket_fd, BACKLOG) == -1)
    {
        perror("listen");
        exit(1);
    }

    /* Assign signal handlers to signals. */
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

    /* Initialise pthread attribute to create detached threads. */
    if (pthread_attr_init(&pthread_attr) != 0)
    {
        perror("pthread_attr_init");
        exit(1);
    }

    if (pthread_attr_setdetachstate(&pthread_attr, PTHREAD_CREATE_DETACHED) != 0)
    {
        perror("pthread_attr_setdetachstate");
        exit(1);
    }

    for (;;)
    {
        /* Create pthread argument for each connection to client. */
        /* TODO: malloc'ing before accepting a connection causes only one small
         * memory when the program exits. It can be safely ignored.
         */
        pthread_arg = (pthread_arg_t *)malloc(sizeof *pthread_arg);
        if (!pthread_arg)
        {
            perror("malloc");
            continue;
        }

        /* Accept connection to client. */
        client_address_len = sizeof pthread_arg->client_address;
        new_socket_fd = accept(server_socket_fd, (struct sockaddr *)&pthread_arg->client_address, &client_address_len);
        if (new_socket_fd == -1)
        {
            perror("accept");
            free(pthread_arg);
            continue;
        }

        /* Initialise pthread argument. */
        pthread_arg->new_socket_fd = new_socket_fd;
        /* TODO: Initialise arguments passed to threads here. See lines 22 and
         * 139.
         */
        if (thpool_num_threads_working(server_thpool) >= SERVER_THPOOL_SIZE)
        {
            fprintf(stderr, "cannot handle client request, max num of dispatching clients reached");
            close(new_socket_fd);
            free(pthread_arg);
            continue;
        }

        if (thpool_add_work(server_thpool, client_handler, (void *)pthread_arg))
        {
            fprintf(stderr, "cannot handle client request, error adding work to thread pool");
            close(new_socket_fd);
            free(pthread_arg);
            continue;
        }
        /* Create thread to serve connection to client. */
        // if (pthread_create(&pthread, &pthread_attr, client_handler, (void *)pthread_arg) != 0)
        // {
        //     perror("pthread_create");
        //     free(pthread_arg);
        //     continue;
        // }
    }
}

static void client_handler(void *arg)
{
    pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
    int new_socket_fd = pthread_arg->new_socket_fd;
    struct sockaddr_in client_address = pthread_arg->client_address;
    unsigned short remote_port = 0;
    int n = 0;
    const size_t BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    messageprocessor msgprocessor = NULL;
    /* TODO: Get arguments passed to threads here. See lines 22 and 116. */

    free(arg);

    /* gethostbyaddr: determine who sent the message */
    struct hostent *hostp = gethostbyaddr((const char *)&client_address.sin_addr.s_addr,
                                          sizeof(client_address.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
    {
        fprintf(stderr, "ERROR on gethostbyaddr\n");
        close(new_socket_fd);
        return;
    }

    char *hostaddrp = inet_ntoa(client_address.sin_addr);
    if (hostaddrp == NULL)
    {
        fprintf(stderr, "ERROR on inet_ntoa\n");
        close(new_socket_fd);
        return;
    }
    remote_port = ntohs(client_address.sin_port);

    printf("server established connection with %s (%s:%hu)\n",
           hostp->h_name, hostaddrp, remote_port);

    /* TODO: Put client interaction code here. For example, use
     * write(new_socket_fd,,) and read(new_socket_fd,,) to send and receive
     * messages with the client.
     */

    msgprocessor = msgprocessor_init();
    if (msgprocessor == NULL)
    {
        fprintf(stderr, "ERROR on msgprocessor_init\n");
        close(new_socket_fd);
        return;
    }

    do
    {
        bzero(buffer, BUFFER_SIZE);
        n = recv(new_socket_fd, buffer, BUFFER_SIZE, 0);
        printf("read %d bytes from %s (%s:%hu)\n",
               n, hostp->h_name, hostaddrp, remote_port);
        if (n <= 0)
        {
            break;
        }
        msgprocessor_add_raw_bytes(msgprocessor, buffer, n);
        // if (strstr(buffer, END_OF_MESSAGE) != NULL)
        // {
        //     printf("received END_OF_MESSAGE break read loop\n");
        //     success = 1;
        //     break;
        // }
        // printf("received: %s", buffer);
        // n = write(new_socket_fd, buffer, strlen(buffer));
        // if (n < 0)
        // {
        //     fprintf(stderr, "ERROR writing to socket\n");
        //     break;
        // }
    } while (msgprocessor_get_available_messages_count(msgprocessor) > 0);

        printf("bye thread\n");
    close(new_socket_fd);
    return;
}

static void signal_handler(int signal_number)
{
    printf("signal_handler: signal %d\n", signal_number);
    printf("signal_handler: start destroy threads\n");
    // thpool_destroy(server_thpool);
    if (server_socket_fd != -1)
    {
        printf("signal_handler: closing server server_socket_fd\n");
        close(server_socket_fd);
    }
    exit(0);
}

int main(int argc, char **argv)
{
    check_parameters(argc, argv);
    printf("port: %d\n", port);

    int factors_len;
    unsigned short *factors;
    if (factorize(USHRT_MAX, &factors, &factors_len) == 0)
    {
        printf("factors: factors_len %d, [", factors_len);
        for (size_t i = 0; i < factors_len; i++)
        {
            printf(" %d ", factors[i]);
        }
        printf("]\n");
        free(factors);
    }

    start_server();
    return 0;
}
