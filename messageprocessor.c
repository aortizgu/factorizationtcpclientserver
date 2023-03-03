#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "messageprocessor.h"

#define BUFFER_SIZE 1024

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

static msg_ *msg_init(char *buffer, const int buffer_len)
{
    msg_ *msg_ptr;
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

static void msgprocessor_slice_messages(msgprocessor_ *msgprocessor_ptr)
{
    uint32_t expected_msg_len;
    msg_ *new_message, *prev_message;

    memcpy(&expected_msg_len, &msgprocessor_ptr->buffer, sizeof(uint32_t));
    expected_msg_len = ntohl(expected_msg_len);
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
        prev_message = msgprocessor_ptr->msg_queue;
        while (prev_message != NULL)
        {
            prev_message = prev_message->next;
        }
        prev_message = new_message;

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
