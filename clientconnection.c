
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common.h"
#include "messageprocessor.h"
#include "clientconnection.h"

void clientconnection(const char *server, unsigned short port, unsigned short *numbers, int numbers_count)
{
    char *raw_request_message_buff = NULL;
    int server_port = 0, socket_fd = 0, raw_request_message_buff_len = 0, n_write = 0, n_write_total = 0, n_read = 0;
    struct hostent *server_host = NULL;
    struct sockaddr_in server_address;
    const size_t read_buffer_size = 1024;
    char read_buffer[read_buffer_size];
    messageprocessor msgprocessor = NULL;
    requestmessage requestmessage_ptr = NULL;
    responsemessage responsemessage_ptr = NULL;
    responsemessage *responsemessages_ptr = NULL;
    int responsemessages_ptr_count = 0;
    message msg_ptr = NULL;
    unsigned char msg_type = 0;

    server_host = gethostbyname(server);
    if (server_host == NULL)
    {
        fprintf(stderr, "clientconnection: cannot get host from %s\n", server);
        exit(1);
    }

    memset(&server_address, 0, sizeof server);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    unsigned int i = 0;
    if (server_host->h_addr_list[0] == NULL)
    {
        fprintf(stderr, "clientconnection: cannot get ip from %s\n", server);
        exit(1);
    }

    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr_list[0], server_host->h_length);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("clientconnection: socket");
        exit(1);
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1)
    {
        perror("clientconnection: connect");
        close(socket_fd);
        exit(1);
    }

    requestmessage_ptr = rqtmsg_init(numbers, numbers_count);
    if (requestmessage_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: cannot allocate memory for requestmessage\n");
        close(socket_fd);
        exit(1);
    }

    if (rqtmsg_serialize(requestmessage_ptr, &raw_request_message_buff, &raw_request_message_buff_len) != 0)
    {
        fprintf(stderr, "clientconnection: cannot allocate memory for raw_request_message_buff\n");
        close(socket_fd);
        exit(1);
    }
    rqtmsg_destroy(requestmessage_ptr);

    do
    {
        n_write = write(socket_fd, raw_request_message_buff + n_write_total, raw_request_message_buff_len - n_write_total);
        printf("clientconnection: written %d bytes\n", n_write);
        if (n_write <= 0)
        {
            break;
        }
        n_write_total += n_write;
    } while (n_write_total < raw_request_message_buff_len);

    free(raw_request_message_buff);

    msgprocessor = msgprocessor_init();
    if (msgprocessor == NULL)
    {
        fprintf(stderr, "clientconnection: ERROR on msgprocessor_init\n");
        close(socket_fd);
        return;
    }

    responsemessages_ptr = calloc(numbers_count, sizeof(responsemessage));
    if (responsemessages_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: Could not allocate memory for responsemessages_ptr\n");
        close(socket_fd);
        return;
    }

    do
    {
        bzero(read_buffer, read_buffer_size);
        n_read = recv(socket_fd, read_buffer, read_buffer_size, 0);
        printf("clientconnection: read %d bytes\n", n_read);
        if (n_read <= 0)
        {
            fprintf(stderr, "clientconnection: no exit read loop\n");
            break;
        }
        msgprocessor_add_raw_bytes(msgprocessor, read_buffer, n_read);
        while (msgprocessor_get_message(msgprocessor, &msg_ptr) == 0)
        {
            printf("clientconnection: dequed message\n");
            msg_type = msg_get_msg_type(msg_ptr);
            if (msg_type == RESPONSE_MSG_ID)
            {
                responsemessage_ptr = rspmsg_init_from_msg(msg_ptr);
                if (responsemessage_ptr != NULL)
                {
                    printf("clientconnection: response message received\n");
                    responsemessages_ptr[responsemessages_ptr_count++] = responsemessage_ptr;
                }
                else
                {
                    fprintf(stderr, "clientconnection: error in rspmsg_init_from_msg\n");
                }
            }
            else
            {
                printf("clientconnection: received unhandled message type %d\n", msg_type);
            }
            msg_destroy(msg_ptr);
        }
    } while (responsemessages_ptr_count < numbers_count);

    free(responsemessages_ptr);
    close(socket_fd);
}