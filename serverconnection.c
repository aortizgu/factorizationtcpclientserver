#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "common.h"
#include "messageprocessor.h"
#include "serverconnection.h"

void serverconnection_handler(void *arg)
{
    serverconnection_arg_t *serverconnection_arg = (serverconnection_arg_t *)arg;
    int new_socket_fd = serverconnection_arg->new_socket_fd;
    struct sockaddr_in client_address = serverconnection_arg->client_address;
    unsigned short remote_port = 0;
    int n = 0;
    unsigned char msg_type = 0;
    const size_t read_buffer_size = 1024;
    char read_buffer[read_buffer_size];
    messageprocessor msgprocessor = NULL;
    requestmessage requestmessage_ptr = NULL;
    message msg_ptr = NULL;

    free(arg);

    struct hostent *hostp = gethostbyaddr((const char *)&client_address.sin_addr.s_addr,
                                          sizeof(client_address.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
    {
        fprintf(stderr, "serverconnection_handler: ERROR on gethostbyaddr\n");
        close(new_socket_fd);
        return;
    }

    char *hostaddrp = inet_ntoa(client_address.sin_addr);
    if (hostaddrp == NULL)
    {
        fprintf(stderr, "serverconnection_handler: ERROR on inet_ntoa\n");
        close(new_socket_fd);
        return;
    }
    remote_port = ntohs(client_address.sin_port);

    printf("serverconnection_handler: server established connection with %s (%s:%hu)\n",
           hostp->h_name, hostaddrp, remote_port);

    msgprocessor = msgprocessor_init();
    if (msgprocessor == NULL)
    {
        fprintf(stderr, "serverconnection_handler: ERROR on msgprocessor_init\n");
        close(new_socket_fd);
        return;
    }

    do
    {
        bzero(read_buffer, read_buffer_size);
        n = recv(new_socket_fd, read_buffer, read_buffer_size, 0);
        printf("serverconnection_handler: read %d bytes from %s (%s:%hu)\n",
               n, hostp->h_name, hostaddrp, remote_port);
        if (n <= 0)
        {
            break;
        }
        msgprocessor_add_raw_bytes(msgprocessor, read_buffer, n);
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
    } while (msgprocessor_get_available_messages_count(msgprocessor) < 4);

    while (msgprocessor_get_message(msgprocessor, &msg_ptr) == 0)
    {
        printf("serverconnection_handler: dequed message\n");
        msg_type = msg_get_msg_type(msg_ptr);
        if (msg_type == REQUEST_MSG_ID)
        {
            requestmessage_ptr = rqtmsg_init_from_msg(msg_ptr);
            if (requestmessage_ptr != NULL)
            {
                printf("serverconnection_handler: request message: numbers\n");
                int len = requestmessage_ptr->numbers_len;
                for (size_t i = 0; i < len; i++)
                {
                    printf("serverconnection_handler: %d\n", requestmessage_ptr->numbers[i]);
                }
            }
            else
            {
                fprintf(stderr, "serverconnection_handler: error in rqtmsg_init_from_msg\n");
            }
        }
        else
        {
            printf("serverconnection_handler: received unhandled message type %d\n", msg_type);
        }
        msg_destroy(msg_ptr);
    }

    msgprocessor_destroy(msgprocessor);
    printf("serverconnection_handler: bye thread\n");
    close(new_socket_fd);
    return;
}