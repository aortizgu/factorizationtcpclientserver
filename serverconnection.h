#ifndef _SERVERCONNECTION_
#define _SERVERCONNECTION_

#include <netinet/in.h>

typedef struct serverconnection_arg_t
{
    int new_socket_fd;
    struct sockaddr_in client_address;
} serverconnection_arg_t;

/**
 * @brief Read and reply client requests over the network
 */
void serverconnection_handler(void *arg);

#endif