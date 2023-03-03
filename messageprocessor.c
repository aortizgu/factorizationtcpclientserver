#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "messageprocessor.h"

#define BUFFER_SIZE 1024
#define MSG_LEN_SIZE sizeof(uint32_t)
#define MSG_TYPE_SIZE sizeof(uint8_t)
#define HEADER_SIZE MSG_LEN_SIZE + MSG_TYPE_SIZE

#define REQUEST_MSG_HEADER_SIZE sizeof(uint8_t)

typedef struct msg_
{
    char *message;
    int message_len;
    struct msg_ *next;
} msg_;

typedef struct msgprocessor_
{
    char *buffer;
    int buffer_capacity;
    int buffer_used_len;
    struct msg_ *msg_queue;
} msgprocessor_;

static void msgprocessor_slice_messages(msgprocessor_ *msgprocessor_ptr);
static void print_buff(char *buffer, const int buffer_len);

rqtmsg_ *rqtmsg_init(unsigned short *numbers, int numbers_len)
{
    rqtmsg_ *rqtmsg_ptr;
    rqtmsg_ptr = (struct rqtmsg_ *)malloc(sizeof(struct rqtmsg_));
    rqtmsg_ptr->numbers_len = numbers_len;
    rqtmsg_ptr->numbers = calloc(numbers_len, sizeof(unsigned short));
    if (rqtmsg_ptr->numbers == NULL)
    {
        fprintf(stderr, "rqtmsg_init: Could not allocate memory for message\n");
        return NULL;
    }
    memcpy(rqtmsg_ptr->numbers, numbers, sizeof(unsigned short) * numbers_len);
    return rqtmsg_ptr;
}

int rqtmsg_serialize(rqtmsg_ *rqtmsg_ptr, char **buffer, int *buff_len)
{
    int pos = 0;
    int message_len = HEADER_SIZE + REQUEST_MSG_HEADER_SIZE + rqtmsg_ptr->numbers_len * sizeof(uint16_t);
    char *message_serialized = malloc(message_len);
    if (message_serialized == NULL)
    {
        fprintf(stderr, "rqtmsg_serialize: Could not allocate memory for message\n");
        return 1;
    }

    uint32_t network_message_len = htonl(message_len);
    memcpy(message_serialized, &network_message_len, sizeof(uint32_t));
    pos += sizeof(uint32_t);
    print_buff(message_serialized, message_len);

    uint8_t message_type = REQUEST_MSG_ID;
    memcpy(message_serialized + pos, &message_type, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    uint8_t network_numbers_len = rqtmsg_ptr->numbers_len;
    memcpy(message_serialized + pos, &network_numbers_len, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    for (size_t i = 0; i < rqtmsg_ptr->numbers_len; i++)
    {
        uint16_t network_number = htons(rqtmsg_ptr->numbers[i]);
        memcpy(message_serialized + pos, &network_number, sizeof(uint16_t));
        pos += sizeof(uint16_t);
    }
    if (pos != message_len)
    {
        fprintf(stderr, "rqtmsg_serialize:  check your code\n");
    }
    print_buff(message_serialized, message_len);
    *buffer = message_serialized;
    *buff_len = message_len;
    return 0;
}

rqtmsg_ *rqtmsg_init_from_msg(msg_ *msg_ptr)
{
    rqtmsg_ *rqtmsg_ptr;
    uint8_t message_type, numbers_len;
    uint16_t network_number;
    unsigned short number;
    int pos = MSG_LEN_SIZE;

    if (msg_ptr == NULL)
    {
        return NULL;
    }

    memcpy(&message_type, msg_ptr->message + pos, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    if (message_type != REQUEST_MSG_ID)
    {
        fprintf(stderr, "rqtmsg_init_from_msg: invalid message type\n");
        return NULL;
    }

    memcpy(&numbers_len, msg_ptr->message + pos, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    rqtmsg_ptr = (struct rqtmsg_ *)malloc(sizeof(struct rqtmsg_));
    rqtmsg_ptr->numbers_len = numbers_len;
    rqtmsg_ptr->numbers = calloc(numbers_len, sizeof(unsigned short));
    if (rqtmsg_ptr->numbers == NULL)
    {
        fprintf(stderr, "rqtmsg_init_from_msg: Could not allocate memory for message\n");
        return NULL;
    }
    for (size_t i = 0; i < numbers_len; i++)
    {
        memcpy(&network_number, msg_ptr->message + pos, sizeof(uint16_t));
        pos += sizeof(uint16_t);
        rqtmsg_ptr->numbers[i] = ntohs(network_number);
    }
    return rqtmsg_ptr;
}

void rqtmsg_destroy(rqtmsg_ *rqtmsg_ptr)
{
    if (rqtmsg_ptr == NULL)
    {
        return;
    }
    if (rqtmsg_ptr->numbers != NULL)
    {
        free(rqtmsg_ptr->numbers);
    }
    free(rqtmsg_ptr);
}

static msg_ *msg_init(char *buffer, const int buffer_len)
{
    msg_ *msg_ptr;
    msg_ptr = (struct msg_ *)malloc(sizeof(struct msg_));
    msg_ptr->message = malloc(buffer_len);
    if (msg_ptr->message == NULL)
    {
        fprintf(stderr, "msg_init: Could not allocate memory for message\n");
        return NULL;
    }
    memcpy(msg_ptr->message, buffer, buffer_len);
    msg_ptr->message_len = buffer_len;
    msg_ptr->next = NULL;
    return msg_ptr;
}

struct msgprocessor_ *msgprocessor_init()
{
    msgprocessor_ *msgprocessor_ptr;
    msgprocessor_ptr = (struct msgprocessor_ *)malloc(sizeof(struct msgprocessor_));
    if (msgprocessor_ptr == NULL)
    {
        fprintf(stderr, "msgprocessor_init: Could not allocate memory for message procesor\n");
        return NULL;
    }
    msgprocessor_ptr->buffer = malloc(BUFFER_SIZE);
    if (msgprocessor_ptr->buffer == NULL)
    {
        fprintf(stderr, "msgprocessor_init: Could not allocate memory for message procesor\n");
        free(msgprocessor_ptr);
        return NULL;
    }
    msgprocessor_ptr->buffer_capacity = BUFFER_SIZE;
    msgprocessor_ptr->buffer_used_len = 0;
    msgprocessor_ptr->msg_queue = NULL;
    return msgprocessor_ptr;
}

static void msgprocessor_add_message(msgprocessor_ *msgprocessor_ptr, msg_ *new_message)
{
    msg_ *current;
    if (msgprocessor_ptr->msg_queue == NULL)
    {
        msgprocessor_ptr->msg_queue = new_message;
    }
    else
    {
        current = msgprocessor_ptr->msg_queue;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_message;
    }
}

int msgprocessor_get_message(msgprocessor_ *msgprocessor_ptr, msg_ **new_message)
{
    if (msgprocessor_ptr->msg_queue == NULL)
    {
        return 1;
    }
    msg_ *tmp = msgprocessor_ptr->msg_queue;
    msgprocessor_ptr->msg_queue = msgprocessor_ptr->msg_queue->next;
    *new_message = tmp;
    return 0;
}

static void msgprocessor_slice_messages(msgprocessor_ *msgprocessor_ptr)
{
    uint32_t expected_msg_len = 0;
    msg_ *new_message;

    memcpy(&expected_msg_len, msgprocessor_ptr->buffer, sizeof(uint32_t));
    expected_msg_len = ntohl(expected_msg_len);
    // printf("msgprocessor_slice_messages: expected_msg_len %d\n", expected_msg_len);
    // printf("msgprocessor_slice_messages: buffer_used_len %d\n", msgprocessor_ptr->buffer_used_len);
    if (expected_msg_len <= msgprocessor_ptr->buffer_used_len)
    {
        // create new message from available data
        new_message = msg_init(msgprocessor_ptr->buffer, expected_msg_len);
        if (new_message == NULL)
        {
            fprintf(stderr, "msgprocessor_init: Could not allocate memory for message\n");
            return;
        }
        printf("msgprocessor_slice_messages: created new message\n");

        // enqueue message into msgprocessor_ptr
        msgprocessor_add_message(msgprocessor_ptr, new_message);

        // printf("AOG: available messges %d\n", msgprocessor_get_available_messages_count(msgprocessor_ptr));
        // move data at the beginig of the buffer
        msgprocessor_ptr->buffer_used_len -= expected_msg_len;
        if (msgprocessor_ptr->buffer_used_len > 0)
        {
            memmove(msgprocessor_ptr->buffer, msgprocessor_ptr->buffer + expected_msg_len, msgprocessor_ptr->buffer_used_len);
        }

        // look for more available messages recursively
        msgprocessor_slice_messages(msgprocessor_ptr);
    }
    else
    {
        // we need more data to create a message
        printf("msgprocessor_slice_messages: no messages available in msgprocessor_ptr\n");
    }
}

int msg_get_msg_type(msg_ *msg_ptr)
{
    unsigned char type;
    if (msg_ptr->message_len < HEADER_SIZE)
    {
        return -1;
    }
    memcpy(&type, msg_ptr->message + MSG_LEN_SIZE, sizeof(unsigned char));
    return type;
}

static void print_buff(char *buffer, const int buffer_len)
{
    printf("print_buff: %d\n", buffer_len);
    for (int i = 0; i < buffer_len; i++)
    {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}

void msgprocessor_add_raw_bytes(msgprocessor_ *msgprocessor_ptr, char *buffer, const int buffer_len)
{
    while ((msgprocessor_ptr->buffer_capacity - msgprocessor_ptr->buffer_used_len) < buffer_len)
    {
        msgprocessor_ptr->buffer = realloc(msgprocessor_ptr->buffer, msgprocessor_ptr->buffer_capacity + BUFFER_SIZE);
        if (msgprocessor_ptr->buffer == NULL)
        {
            fprintf(stderr, "msgprocessor_init: Could not allocate memory for message procesor\n");
            return;
        }
        msgprocessor_ptr->buffer_capacity += BUFFER_SIZE;
        printf("msgprocessor_add_raw_bytes: reallocated buffer, capacity is %d\n", msgprocessor_ptr->buffer_capacity);
    }
    memcpy(msgprocessor_ptr->buffer + msgprocessor_ptr->buffer_used_len, buffer, buffer_len);
    msgprocessor_ptr->buffer_used_len += buffer_len;
    msgprocessor_slice_messages(msgprocessor_ptr);
}

int msgprocessor_get_available_messages_count(msgprocessor_ *msgprocessor_ptr)
{
    if (msgprocessor_ptr == NULL)
    {
        fprintf(stderr, "msgprocessor_get_available_messages_count: invalid msgprocessor_ptr\n");
        return -1;
    }
    int i = 0;
    msg_ *m = msgprocessor_ptr->msg_queue;
    while (m != NULL)
    {
        m = m->next;
        i++;
    }
    return i;
}

void msg_destroy(msg_ *msg_ptr)
{
    if (msg_ptr == NULL)
    {
        return;
    }
    free(msg_ptr->message);
    free(msg_ptr);
}

void msgprocessor_destroy(msgprocessor_ *msgprocessor_ptr)
{
    if (msgprocessor_ptr == NULL)
    {
        return;
    }
    if (msgprocessor_ptr->buffer != NULL)
    {
        free(msgprocessor_ptr->buffer);
    }
    free(msgprocessor_ptr);
}
