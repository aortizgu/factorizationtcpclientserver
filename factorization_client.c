#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "common.h"

#define MAX_NUMBERS 10

char *server = DEFAULT_SERVER;
const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;
int numbers[MAX_NUMBERS];
int numbers_count = 0;

int parseNumber(char *string)
{
	if (isNum(string) != 0)
	{
		return -1;
	}
	int n = atoi(string);
	if (n < 2)
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

void check_parameters(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "s:p:n:h")) != -1)
	{
		// printf("give me opt %c - %s \n", opt, optarg);
		switch (opt)
		{
		case 'h':
			printf("factorization_client -s [server, default %s] -p [port, default %s] -n [list of numbers to factorize]\n", DEFAULT_SERVER, DEFAULT_PORT_STR);
			exit(0);
			break;
		case 's':
			if (isValidIpAddress(optarg) != 0)
			{
				fprintf(stderr, "invalid ipAddress: %s\n", optarg);
				exit(1);
			}
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

	return 0;
}