#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "common.h"

char *server = DEFAULT_SERVER;
const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;

void check_parameters(int argc, char **argv)
{
	int opt;

	while ((opt = getopt(argc, argv, "s:p:h")) != -1)
	{
		switch (opt)
		{
		case 'h':
			printf("factorization_server -s [server, default %s] -p [port, default %s]\n", DEFAULT_SERVER, DEFAULT_PORT_STR);
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
		}
	}
}

int main(int argc, char **argv)
{
	check_parameters(argc, argv);
	printf("server: %s\n", server);
	printf("port: %d\n", port);

	return 0;
}