#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "common.h"
#include "messageprocessor.h"

#define BUFFER_SIZE 64
#define MSG_LEN_SIZE sizeof(uint32_t)
#define MSG_TYPE_SIZE sizeof(uint8_t)
#define MSG_HEADER_SIZE MSG_LEN_SIZE + MSG_TYPE_SIZE

#define REQUEST_MSG_LEN_SIZE sizeof(uint8_t)
#define REQUEST_MSG_FIXED_SIZE REQUEST_MSG_LEN_SIZE

#define RESPONSE_MSG_NUMBER_SIZE sizeof(uint16_t)
#define RESPONSE_MSG_LEN_SIZE sizeof(uint8_t)
#define RESPONSE_MSG_FIXED_SIZE RESPONSE_MSG_NUMBER_SIZE + RESPONSE_MSG_LEN_SIZE

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

rqtmsg_ *rqtmsg_init(unsigned short *numbers, int numbers_len)
{
    rqtmsg_ *rqtmsg_ptr = NULL;
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

rqtmsg_ *rqtmsg_init_from_msg(msg_ *msg_ptr)
{
    rqtmsg_ *rqtmsg_ptr = NULL;
    uint8_t message_type = 0, numbers_len = 0;
    uint16_t network_number = 0;
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
    if (rqtmsg_ptr == NULL)
    {
        fprintf(stderr, "rqtmsg_init_from_msg: Could not allocate memory for rqtmsg_ptr\n");
        return NULL;
    }

    rqtmsg_ptr->numbers_len = numbers_len;
    rqtmsg_ptr->numbers = calloc(numbers_len, sizeof(unsigned short));
    if (rqtmsg_ptr->numbers == NULL)
    {
        fprintf(stderr, "rqtmsg_init_from_msg: Could not allocate memory for message\n");
        free(rqtmsg_ptr);
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

int rqtmsg_serialize(rqtmsg_ *rqtmsg_ptr, char **buffer, int *buff_len)
{
    int pos = 0;
    uint32_t network_message_len = 0;
    uint16_t network_number = 0;
    uint8_t message_type = 0, network_numbers_len = 0;
    const int message_len = MSG_HEADER_SIZE + REQUEST_MSG_FIXED_SIZE + rqtmsg_ptr->numbers_len * sizeof(uint16_t);

    char *message_serialized = malloc(message_len);
    if (message_serialized == NULL)
    {
        fprintf(stderr, "rqtmsg_serialize: Could not allocate memory for message\n");
        return 1;
    }

    network_message_len = htonl(message_len);
    memcpy(message_serialized, &network_message_len, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    message_type = REQUEST_MSG_ID;
    memcpy(message_serialized + pos, &message_type, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    network_numbers_len = rqtmsg_ptr->numbers_len;
    memcpy(message_serialized + pos, &network_numbers_len, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    for (size_t i = 0; i < rqtmsg_ptr->numbers_len; i++)
    {
        network_number = htons(rqtmsg_ptr->numbers[i]);
        memcpy(message_serialized + pos, &network_number, sizeof(uint16_t));
        pos += sizeof(uint16_t);
    }
    if (pos != message_len)
    {
        fprintf(stderr, "rqtmsg_serialize:  check your code\n");
    }
    *buffer = message_serialized;
    *buff_len = message_len;
    return 0;
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

rspmsg_ *rspmsg_init(unsigned short number, unsigned short *factors, int factors_len)
{
    rspmsg_ *rspmsg_ptr = NULL;
    rspmsg_ptr = (struct rspmsg_ *)malloc(sizeof(struct rspmsg_));
    rspmsg_ptr->number = number;
    rspmsg_ptr->factors_len = factors_len;
    rspmsg_ptr->factors = calloc(factors_len, sizeof(unsigned short));
    if (rspmsg_ptr->factors == NULL)
    {
        fprintf(stderr, "rspmsg_init: Could not allocate memory for message\n");
        return NULL;
    }
    memcpy(rspmsg_ptr->factors, factors, sizeof(unsigned short) * factors_len);
    return rspmsg_ptr;
}

rspmsg_ *rspmsg_init_from_msg(msg_ *msg_ptr)
{
    rspmsg_ *rspmsg_ptr = NULL;
    uint8_t message_type = 0, factors_len = 0;
    uint16_t network_number = 0, network_factor = 0;
    int pos = MSG_LEN_SIZE;

    if (msg_ptr == NULL)
    {
        return NULL;
    }

    memcpy(&message_type, msg_ptr->message + pos, sizeof(uint8_t));
    pos += sizeof(uint8_t);
    if (message_type != RESPONSE_MSG_ID)
    {
        fprintf(stderr, "rqtmsg_init_from_msg: invalid message type\n");
        return NULL;
    }
    rspmsg_ptr = (struct rspmsg_ *)malloc(sizeof(struct rspmsg_));
    if (rspmsg_ptr == NULL)
    {
        fprintf(stderr, "rspmsg_init_from_msg: Could not allocate memory for rspmsg_ptr\n");
        return NULL;
    }

    memcpy(&network_number, msg_ptr->message + pos, sizeof(uint16_t));
    pos += sizeof(uint16_t);
    rspmsg_ptr->number = ntohs(network_number);

    memcpy(&factors_len, msg_ptr->message + pos, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    rspmsg_ptr->factors_len = factors_len;
    rspmsg_ptr->factors = calloc(factors_len, sizeof(unsigned short));
    if (rspmsg_ptr->factors == NULL)
    {
        fprintf(stderr, "rspmsg_init_from_msg: Could not allocate memory for message\n");
        free(rspmsg_ptr);
        return NULL;
    }
    for (size_t i = 0; i < factors_len; i++)
    {
        memcpy(&network_factor, msg_ptr->message + pos, sizeof(uint16_t));
        pos += sizeof(uint16_t);
        rspmsg_ptr->factors[i] = ntohs(network_factor);
    }
    return rspmsg_ptr;
}

int rspmsg_serialize(rspmsg_ *rspmsg_ptr, char **buffer, int *buff_len)
{
    int pos = 0;
    uint32_t network_message_len = 0;
    uint16_t network_number = 0, network_factor = 0;
    uint8_t message_type = 0, network_factors_len = 0;
    const int message_len = MSG_HEADER_SIZE + RESPONSE_MSG_FIXED_SIZE + rspmsg_ptr->factors_len * sizeof(uint16_t);

    char *message_serialized = malloc(message_len);
    if (message_serialized == NULL)
    {
        fprintf(stderr, "rspmsg_serialize: Could not allocate memory for message\n");
        return 1;
    }

    network_message_len = htonl(message_len);
    memcpy(message_serialized, &network_message_len, sizeof(uint32_t));
    pos += sizeof(uint32_t);

    message_type = RESPONSE_MSG_ID;
    memcpy(message_serialized + pos, &message_type, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    network_number = htons(rspmsg_ptr->number);
    memcpy(message_serialized + pos, &network_number, sizeof(uint16_t));
    pos += sizeof(uint16_t);

    network_factors_len = rspmsg_ptr->factors_len;
    memcpy(message_serialized + pos, &network_factors_len, sizeof(uint8_t));
    pos += sizeof(uint8_t);

    for (size_t i = 0; i < rspmsg_ptr->factors_len; i++)
    {
        network_factor = htons(rspmsg_ptr->factors[i]);
        memcpy(message_serialized + pos, &network_factor, sizeof(uint16_t));
        pos += sizeof(uint16_t);
    }
    if (pos != message_len)
    {
        fprintf(stderr, "rspmsg_serialize:  check your code\n");
    }
    *buffer = message_serialized;
    *buff_len = message_len;
    return 0;
}

void rspmsg_destroy(rspmsg_ *rspmsg_ptr)
{
    if (rspmsg_ptr == NULL)
    {
        return;
    }
    if (rspmsg_ptr->factors != NULL)
    {
        free(rspmsg_ptr->factors);
    }
    free(rspmsg_ptr);
}

static msg_ *msg_init(char *buffer, const int buffer_len)
{
    msg_ *msg_ptr = NULL;
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

void msg_destroy(msg_ *msg_ptr)
{
    if (msg_ptr == NULL)
    {
        return;
    }
    free(msg_ptr->message);
    free(msg_ptr);
}

unsigned char msg_get_msg_type(msg_ *msg_ptr)
{
    if (msg_ptr == NULL)
    {
        fprintf(stderr, "msg_get_msg_type: msg_ptr is null\n");
    }
    unsigned char type = 0;
    if (msg_ptr->message_len < MSG_HEADER_SIZE)
    {
        return -1;
    }
    memcpy(&type, msg_ptr->message + MSG_LEN_SIZE, sizeof(unsigned char));
    return type;
}

struct msgprocessor_ *msgprocessor_init()
{
    msgprocessor_ *msgprocessor_ptr = (struct msgprocessor_ *)malloc(sizeof(struct msgprocessor_));
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

void msgprocessor_destroy(msgprocessor_ *msgprocessor_ptr)
{
    if (msgprocessor_ptr == NULL)
    {
        return;
    }
    msg_ *tmp = NULL;
    msg_ *m = msgprocessor_ptr->msg_queue;
    while (m != NULL)
    {
        tmp = m;
        m = m->next;
        msg_destroy(tmp);
    }
    if (msgprocessor_ptr->buffer != NULL)
    {
        free(msgprocessor_ptr->buffer);
    }
    free(msgprocessor_ptr);
}

static void msgprocessor_add_message(msgprocessor_ *msgprocessor_ptr, msg_ *new_message)
{
    msg_ *current = NULL;
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

static void msgprocessor_slice_messages(msgprocessor_ *msgprocessor_ptr)
{
    uint32_t expected_msg_len = 0;
    msg_ *new_message = NULL;

    memcpy(&expected_msg_len, msgprocessor_ptr->buffer, sizeof(uint32_t));
    expected_msg_len = ntohl(expected_msg_len);
    D(printf("msgprocessor_slice_messages: expected_msg_len %d\n", expected_msg_len));
    D(printf("msgprocessor_slice_messages: buffer_used_len %d\n", msgprocessor_ptr->buffer_used_len));
    if (expected_msg_len <= msgprocessor_ptr->buffer_used_len)
    {
        // create new message from available data
        new_message = msg_init(msgprocessor_ptr->buffer, expected_msg_len);
        if (new_message == NULL)
        {
            fprintf(stderr, "msgprocessor_init: Could not allocate memory for message\n");
            return;
        }
        D(printf("msgprocessor_slice_messages: created new message\n"));

        // enqueue message into msgprocessor_ptr
        msgprocessor_add_message(msgprocessor_ptr, new_message);

        D(printf("msgprocessor_slice_messages: available messges %d\n", msgprocessor_get_available_messages_count(msgprocessor_ptr)));
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
        D(printf("msgprocessor_slice_messages: no messages available in msgprocessor_ptr\n"));
    }
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
        D(printf("msgprocessor_add_raw_bytes: reallocated buffer, capacity is %d\n", msgprocessor_ptr->buffer_capacity));
    }
    memcpy(msgprocessor_ptr->buffer + msgprocessor_ptr->buffer_used_len, buffer, buffer_len);
    msgprocessor_ptr->buffer_used_len += buffer_len;
    msgprocessor_slice_messages(msgprocessor_ptr);
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
