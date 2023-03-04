#ifndef _BLOCKINGQUEUE_
#define _BLOCKINGQUEUE_

#include <stdio.h>

typedef struct _data
{
    void *content;
    struct _data *next;
} data;

typedef struct bqueue_ *blockingqueue;

/**
 * @brief Initialize internal resources of the object
 */
blockingqueue bqueue_init(size_t content_size);
/**
 * @brief enqueue data into the queue
 */
void enqueue(blockingqueue, void *data_ptr);
/**
 * @brief dequeue data from the queue or blocks until available data
 */
void dequeue(blockingqueue, void *data_ptr);
/**
 * @brief Get the size of the queue
 */
int get_size(blockingqueue);
/**
 * @brief Release internal resources
 */
void bqueue_destroy(blockingqueue);

#endif