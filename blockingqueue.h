#ifndef _BLOCKINGQUEUE_
#define _BLOCKINGQUEUE_

#include <stdio.h>

typedef struct _data
{
    void *content;
    struct _data *next;
} data;

typedef struct bqueue_ *blockingqueue;

blockingqueue bqueue_init(size_t content_size);
void enqueue(blockingqueue, void *data_ptr);
void dequeue(blockingqueue, void *data_ptr);
size_t get_size(blockingqueue);
void bqueue_destroy(blockingqueue);

#endif