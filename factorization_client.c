#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "common.h"
#include "messageprocessor.h"
#include "clientconnection.h"

#define MAX_NUMBERS 10
#define MAX_FACTORIZABLE_NUMBER 50000

static char *server = DEFAULT_SERVER;
static unsigned short port = DEFAULT_PORT;
static unsigned short numbers[MAX_NUMBERS];
static int numbers_count = 0;

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
    clientconnection(server, port, numbers, numbers_count);
    return 0;
}