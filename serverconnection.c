#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "messageprocessor.h"
#include "serverconnection.h"
#include "libfactorization.h"

#ifdef DEBUG
#define D(x) x
#else
#define D(x)
#endif

static void send_response(responsemessage responsemessage_ptr, int socket_fd)
{
    char *raw_request_message_buff = NULL;
    int raw_request_message_buff_len = 0, n_write = 0, n_write_total = 0;

    if (rspmsg_serialize(responsemessage_ptr, &raw_request_message_buff, &raw_request_message_buff_len) != 0)
    {
        fprintf(stderr, "send_response: cannot allocate memory for raw_request_message_buff\n");
        return;
    }

    do
    {
        n_write = write(socket_fd, raw_request_message_buff + n_write_total, raw_request_message_buff_len - n_write_total);
        D(printf("send_response: written %d bytes\n", n_write));
        if (n_write <= 0)
        {
            break;
        }
        n_write_total += n_write;
    } while (n_write_total < raw_request_message_buff_len);

    free(raw_request_message_buff);
}

static void process_number(unsigned short number, int socket_fd)
{
    D(printf("process_number: number %d\n", number));
    int factors_len;
    unsigned short *factors;
    if (factorize(number, &factors, &factors_len) == 0)
    {
        D(printf("factors: factors_len %d, [", factors_len));
        for (size_t i = 0; i < factors_len; i++)
        {
            D(printf(" %d ", factors[i]));
        }
        D(printf("]\n"));
        responsemessage responsemessage_ptr = rspmsg_init(number, factors, factors_len);
        if (responsemessage_ptr != NULL)
        {
            send_response(responsemessage_ptr, socket_fd);
            rspmsg_destroy(responsemessage_ptr);
        }
        else
        {
            fprintf(stderr, "process_number: cannot create responsemessage_ptr for number %d\n", number);
        }
        free(factors);
    }
    else
    {
        fprintf(stderr, "process_number: factorizing %d\n", number);
    }
}

static void process_request(requestmessage requestmessage_ptr, int socket_fd)
{
    D(printf("process_request: requestmessage number_len %d\n", requestmessage_ptr->numbers_len));
    for (size_t i = 0; i < requestmessage_ptr->numbers_len; i++)
    {
        process_number(requestmessage_ptr->numbers[i], socket_fd);
    }
    rqtmsg_destroy(requestmessage_ptr);
}

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
        D(printf("serverconnection_handler: read %d bytes from %s (%s:%hu)\n",
                 n, hostp->h_name, hostaddrp, remote_port));
        if (n <= 0)
        {
            break;
        }
        msgprocessor_add_raw_bytes(msgprocessor, read_buffer, n);
    } while (msgprocessor_get_available_messages_count(msgprocessor) <= 0);

    while (msgprocessor_get_message(msgprocessor, &msg_ptr) == 0)
    {
        D(printf("serverconnection_handler: dequed message\n"));
        msg_type = msg_get_msg_type(msg_ptr);
        if (msg_type == REQUEST_MSG_ID)
        {
            requestmessage_ptr = rqtmsg_init_from_msg(msg_ptr);
            if (requestmessage_ptr != NULL)
            {
                process_request(requestmessage_ptr, new_socket_fd);
            }
            else
            {
                fprintf(stderr, "serverconnection_handler: error in rqtmsg_init_from_msg\n");
            }
        }
        else
        {
            D(printf("serverconnection_handler: received unhandled message type %d\n", msg_type));
        }
        msg_destroy(msg_ptr);
    }

    msgprocessor_destroy(msgprocessor);
    D(printf("serverconnection_handler: bye thread\n"));
    close(new_socket_fd);
    printf("serverconnection_handler: connection with %s (%s:%hu) dispatched\n",
           hostp->h_name, hostaddrp, remote_port);
    return;
}
