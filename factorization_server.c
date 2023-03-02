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
#include "common.h"
#include "libfactorization.h"
#include <arpa/inet.h>

#define BACKLOG 10

const char *port_str = DEFAULT_PORT_STR;
short port = DEFAULT_PORT;
int server_socket_fd = -1;

typedef struct pthread_arg_t
{
	int new_socket_fd;
	struct sockaddr_in client_address;
	/* TODO: Put arguments passed to threads here. See lines 116 and 139. */
} pthread_arg_t;

/* Thread routine to serve connection to client. */
void *pthread_routine(void *arg);

/* Signal handler to handle SIGTERM and SIGINT signals. */
void signal_handler(int signal_number);

void check_parameters(int argc, char **argv)
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

void start_server()
{
	printf("start_server");
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

		/* Create thread to serve connection to client. */
		if (pthread_create(&pthread, &pthread_attr, pthread_routine, (void *)pthread_arg) != 0)
		{
			perror("pthread_create");
			free(pthread_arg);
			continue;
		}
	}

	/* close(server_socket_fd);
	 * TODO: If you really want to close the socket, you would do it in
	 * signal_handler(), meaning server_socket_fd would need to be a global variable.
	 */
}

void *pthread_routine(void *arg)
{
	pthread_arg_t *pthread_arg = (pthread_arg_t *)arg;
	int new_socket_fd = pthread_arg->new_socket_fd;
	struct sockaddr_in client_address = pthread_arg->client_address;
	/* TODO: Get arguments passed to threads here. See lines 22 and 116. */

	free(arg);

    /* gethostbyaddr: determine who sent the message */
    struct hostent *hostp = gethostbyaddr((const char *)&client_address.sin_addr.s_addr, 
			  sizeof(client_address.sin_addr.s_addr), AF_INET);
    if (hostp == NULL) {
		fprintf(stderr, "ERROR on gethostbyaddr\n");
		close(new_socket_fd);
		return NULL;
	}

    char *hostaddrp = inet_ntoa(client_address.sin_addr);
    if (hostaddrp == NULL) {
		fprintf(stderr, "ERROR on inet_ntoa\n");
		close(new_socket_fd);
		return NULL;
	}

    printf("server established connection with %s (%s)\n", 
	   hostp->h_name, hostaddrp);

	/* TODO: Put client interaction code here. For example, use
	 * write(new_socket_fd,,) and read(new_socket_fd,,) to send and receive
	 * messages with the client.
	 */
	int n;
	const size_t SIZE = 2 * 1024;
	char buffer[SIZE];

	for (;;)
	{
		bzero(buffer, SIZE);
		n = recv(new_socket_fd, buffer, SIZE, 0);
		printf("read %d bytes from %s (%s)\n", n, hostp->h_name, hostaddrp);
		if (n <= 0)
		{
			break;
		}
		if (strstr(buffer, "BYE") != NULL)
		{
			printf("received BYE, closing socket\n");
			break;
		}
		printf("received: %s", buffer);
	    n = write(new_socket_fd, buffer, strlen(buffer));
		if (n < 0) {
			fprintf(stderr, "ERROR writing to socket\n");
			break;
		}
	}
	printf("bye thread\n");
	close(new_socket_fd);
	return NULL;
}

void signal_handler(int signal_number)
{
	/* TODO: Put exit cleanup code here. */
	if (server_socket_fd != -1) {
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
	int *factors;
	if (factorize(32768, &factors, &factors_len) == 0)
	{
		printf("factors: ");
		for (size_t i = 0; i < factors_len; i++)
		{
			printf(" %d ", factors[i]);
		}
		printf("\n");
		free(factors);
	}

	start_server();
	return 0;
}
