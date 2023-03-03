#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"

#define MAX_NUMBERS 10
#define MAX_FACTORIZABLE_NUMBER 50000

char *server = DEFAULT_SERVER;
const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;
unsigned short numbers[MAX_NUMBERS];
int numbers_count = 0;

static int parseNumber(char *string)
{
    if (isNum(string) != 0)
    {
        return -1;
    }
    int n = atoi(string);
    if (n < 2 || n > MAX_FACTORIZABLE_NUMBER)
    {
        return -1;
    }
    if (numbers_count >= MAX_NUMBERS)
    {
        fprintf(stderr, "max number of input numbers reached: %d\n", MAX_NUMBERS);
        exit(1);
    }
    numbers[numbers_count++] = n;
    return 0;
}

static void check_parameters(int argc, char **argv)
{
    int opt;

    while ((opt = getopt(argc, argv, "s:p:n:h")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("factorization_client -s [server, default %s] -p [port, default %s] -n [list of numbers to factorize]\n", DEFAULT_SERVER, DEFAULT_PORT_STR);
            exit(0);
            break;
        case 's':
            // if (isValidIpAddress(optarg) != 0)
            // {
            //     fprintf(stderr, "invalid ipAddress: %s\n", optarg);
            //     // exit(1);
            // }
            server = optarg;
            break;
        case 'p':
            port = parsePort(optarg);
            if (port < 0)
            {
                fprintf(stderr, "invalid port: %s\n", argv[optind]);
                exit(1);
            }
            break;
        case 'n':
            optind--;
            for (; optind < argc && *argv[optind] != '-'; optind++)
            {
                if (parseNumber(argv[optind]) != 0)
                {
                    fprintf(stderr, "invalid number: %s\n", argv[optind]);
                    exit(1);
                }
            }
            break;
        }
    }
}

void create_connection()
{
    int server_port, socket_fd;
    struct hostent *server_host;
    struct sockaddr_in server_address;

    server_host = gethostbyname(server);
    if (server_host == NULL)
    {
        fprintf(stderr, "cannot get host from %s\n", server);
        exit(1);
    }

    memset(&server_address, 0, sizeof server);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    unsigned int i = 0;
    if (server_host->h_addr_list[0] == NULL)
    {
        fprintf(stderr, "cannot get ip from %s\n", server);
        exit(1);
    }

    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr_list[0], server_host->h_length);

    /* Create TCP socket. */
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    /* Connect to socket with server address. */
    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1)
    {
        perror("connect");
        exit(1);
    }

    /* TODO: Put server interaction code here. For example, use
     * write(socket_fd,,) and read(socket_fd,,) to send and receive messages
     * with the client.
     */

    close(socket_fd);
}

int main(int argc, char **argv)
{
    check_parameters(argc, argv);
    if (numbers_count == 0)
    {
        fprintf(stderr, "invalid arguments: numbers are required\n");
        exit(1);
    }
    printf("server: %s\n", server);
    printf("port: %d\n", port);
    printf("numbers:");
    for (size_t i = 0; i < numbers_count; i++)
    {
        printf(" %d", numbers[i]);
    }
    printf("\n");
    create_connection();

    return 0;
}