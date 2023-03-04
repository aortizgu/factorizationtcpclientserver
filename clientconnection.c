
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#include "common.h"
#include "messageprocessor.h"
#include "clientconnection.h"
#include "blockingqueue.h"

typedef struct print_results_work_arg_t
{
    blockingqueue blockingqueue_ptr;
    int expected_responses;
} print_results_work_arg_t;

static void print_result(responsemessage *responsemessages_ptr, int responsemessages_ptr_count)
{
    responsemessage responsemessage_ptr = NULL;
    for (size_t i = 0; i < responsemessages_ptr_count; i++)
    {
        responsemessage_ptr = responsemessages_ptr[i];
        printf("##################################\n");
        printf("# NUMBER: %d\n", responsemessage_ptr->number);
        printf("# FACTORS:");
        for (size_t j = 0; j < responsemessage_ptr->factors_len; j++)
        {
            printf(" %d", responsemessage_ptr->factors[j]);
        }
        printf("\n");
    }
    printf("##################################\n");
}

static void *print_results_work(void *arg)
{
    D(printf("print_results_work: start\n"));
    print_results_work_arg_t *print_results_work_arg = (print_results_work_arg_t *)arg;
    int expected_responses = print_results_work_arg->expected_responses;
    blockingqueue blockingqueue_ptr = print_results_work_arg->blockingqueue_ptr;
    int received_responses = 0;
    responsemessage responsemessage_ptr = NULL;
    responsemessage *responsemessages_ptr = NULL;
    free(arg);
    responsemessages_ptr = calloc(sizeof(responsemessage), expected_responses);
    if (responsemessages_ptr == NULL)
    {
        fprintf(stderr, "print_results_work: cannot allocate memory for responsemessages_ptr\n");
        return NULL;
    }

    do
    {
        D(printf("print_results_work: wait for queue\n"));
        dequeue(blockingqueue_ptr, &responsemessage_ptr);
        D(printf("print_results_work: dequeue\n"));
        responsemessages_ptr[received_responses++] = responsemessage_ptr;

    } while (received_responses < expected_responses);

    D(printf("print_results_work: received_responses %d\n", received_responses));
    D(printf("print_results_work: end\n"));
    print_result(responsemessages_ptr, received_responses);
    for (size_t i = 0; i < received_responses; i++)
    {
        rspmsg_destroy(responsemessages_ptr[i]);
    }
    free(responsemessages_ptr);
    return NULL;
}

void clientconnection(const char *server, unsigned short port, unsigned short *numbers, int numbers_count)
{
    char *raw_request_message_buff = NULL;
    int socket_fd = 0, raw_request_message_buff_len = 0, n_write = 0, n_write_total = 0, n_read = 0;
    struct hostent *server_host = NULL;
    struct sockaddr_in server_address;
    const size_t read_buffer_size = 1024;
    char read_buffer[read_buffer_size];
    messageprocessor msgprocessor_ptr = NULL;
    requestmessage requestmessage_ptr = NULL;
    responsemessage responsemessage_ptr = NULL;
    blockingqueue blockingqueue_ptr = NULL;
    int responsemessages_ptr_count = 0;
    message msg_ptr = NULL;
    pthread_t thread;
    print_results_work_arg_t *thread_arg_ptr = 0;
    unsigned char msg_type = 0;

    server_host = gethostbyname(server);
    if (server_host == NULL)
    {
        fprintf(stderr, "clientconnection: cannot get host from %s\n", server);
        return;
    }

    memset(&server_address, 0, sizeof server);
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    if (server_host->h_addr_list[0] == NULL)
    {
        fprintf(stderr, "clientconnection: cannot get ip from %s\n", server);
        return;
    }

    memcpy(&server_address.sin_addr.s_addr, server_host->h_addr_list[0], server_host->h_length);

    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("clientconnection: socket");
        return;
    }

    if (connect(socket_fd, (struct sockaddr *)&server_address, sizeof server_address) == -1)
    {
        perror("clientconnection: connect");
        close(socket_fd);
        return;
    }

    requestmessage_ptr = rqtmsg_init(numbers, numbers_count);
    if (requestmessage_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: cannot allocate memory for requestmessage\n");
        close(socket_fd);
        return;
    }

    if (rqtmsg_serialize(requestmessage_ptr, &raw_request_message_buff, &raw_request_message_buff_len) != 0)
    {
        fprintf(stderr, "clientconnection: cannot allocate memory for raw_request_message_buff\n");
        close(socket_fd);
        return;
    }
    rqtmsg_destroy(requestmessage_ptr);

    do
    {
        n_write = write(socket_fd, raw_request_message_buff + n_write_total, raw_request_message_buff_len - n_write_total);
        D(printf("clientconnection: written %d bytes\n", n_write));
        if (n_write <= 0)
        {
            break;
        }
        n_write_total += n_write;
    } while (n_write_total < raw_request_message_buff_len);

    free(raw_request_message_buff);

    msgprocessor_ptr = msgprocessor_init();
    if (msgprocessor_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: ERROR on msgprocessor_init\n");
        close(socket_fd);
        return;
    }

    blockingqueue_ptr = bqueue_init(sizeof(responsemessage));
    if (blockingqueue_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: Could not allocate memory for blockingqueue_ptr\n");
        close(socket_fd);
        msgprocessor_destroy(msgprocessor_ptr);
        // free(responsemessages_ptr);
        return;
    }

    thread_arg_ptr = (print_results_work_arg_t *)malloc(sizeof *thread_arg_ptr);
    if (thread_arg_ptr == NULL)
    {
        fprintf(stderr, "clientconnection: Could not allocate memory for thread_arg_ptr\n");
        close(socket_fd);
        msgprocessor_destroy(msgprocessor_ptr);
        bqueue_destroy(blockingqueue_ptr);
        return;
    }
    thread_arg_ptr->blockingqueue_ptr = blockingqueue_ptr;
    thread_arg_ptr->expected_responses = numbers_count;

    if (pthread_create(&thread, NULL, print_results_work, (void *)thread_arg_ptr) != 0)
    {
        fprintf(stderr, "clientconnection: Could not start thread\n");
        close(socket_fd);
        msgprocessor_destroy(msgprocessor_ptr);
        bqueue_destroy(blockingqueue_ptr);
        free(thread_arg_ptr);
        return;
    }

    do
    {
        bzero(read_buffer, read_buffer_size);
        n_read = recv(socket_fd, read_buffer, read_buffer_size, 0);
        D(printf("clientconnection: read %d bytes\n", n_read));
        if (n_read <= 0)
        {
            D(printf("clientconnection: no more bytes to read\n"));
            break;
        }
        msgprocessor_add_raw_bytes(msgprocessor_ptr, read_buffer, n_read);
        while (msgprocessor_get_message(msgprocessor_ptr, &msg_ptr) == 0)
        {
            D(printf("clientconnection: dequed message\n"));
            msg_type = msg_get_msg_type(msg_ptr);
            if (msg_type == RESPONSE_MSG_ID)
            {
                responsemessage_ptr = rspmsg_init_from_msg(msg_ptr);
                if (responsemessage_ptr != NULL)
                {
                    D(printf("clientconnection: response message received\n"));
                    enqueue(blockingqueue_ptr, &responsemessage_ptr);
                }
                else
                {
                    fprintf(stderr, "clientconnection: error in rspmsg_init_from_msg\n");
                }
            }
            else
            {
                D(printf("clientconnection: received unhandled message type %d\n", msg_type));
            }
            msg_destroy(msg_ptr);
        }
    } while (responsemessages_ptr_count < numbers_count);

    D(printf("clientconnection: start join\n"));
    if (pthread_join(thread, NULL))
    {
        fprintf(stderr, "Error joining thread\n");
    }
    D(printf("clientconnection: exit join\n"));
    bqueue_destroy(blockingqueue_ptr);
    msgprocessor_destroy(msgprocessor_ptr);
    close(socket_fd);
}