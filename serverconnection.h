#ifndef _SERVERCONNECTION_
#define _SERVERCONNECTION_

#include <netinet/in.h>

typedef struct serverconnection_arg_t
{
    int new_socket_fd;
    struct sockaddr_in client_address;
} serverconnection_arg_t;

void serverconnection_handler(void *arg);

#endif