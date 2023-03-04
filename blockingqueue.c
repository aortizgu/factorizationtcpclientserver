#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "blockingqueue.h"

typedef struct bqueue_
{
    pthread_mutex_t lock;
    pthread_cond_t new_data;
    size_t content_size;
    data *head;
} bqueue_;

bqueue_ *bqueue_init(size_t content_size)
{
    bqueue_ *q = (bqueue_ *)malloc(sizeof(bqueue_));
    if (q == NULL)
    {
        fprintf(stderr, "canot allocate bqueue_ object\n");
        return NULL;
    }
    q->content_size = content_size;
    q->head = NULL;
    pthread_mutex_init(&(q->lock), NULL);
    pthread_cond_init(&q->new_data, NULL);
    return q;
}

void enqueue(bqueue_ *bqueue_ptr, void *data_ptr)
{
    data *current = NULL;
    data *new_data = (data *)malloc(sizeof(data));
    if (new_data == NULL)
    {
        fprintf(stderr, "canot allocate new data object\n");
        return;
    }

    new_data->content = malloc(bqueue_ptr->content_size);
    if (new_data->content == NULL)
    {
        fprintf(stderr, "canot allocate new content\n");
        return;
    }

    new_data->next = NULL;
    memcpy(new_data->content, data_ptr, bqueue_ptr->content_size);
    pthread_mutex_lock(&bqueue_ptr->lock);
    if (bqueue_ptr->head == NULL)
    {
        bqueue_ptr->head = new_data;
    }
    else
    {
        current = bqueue_ptr->head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        current->next = new_data;
    }
    pthread_cond_signal(&bqueue_ptr->new_data);
    pthread_mutex_unlock(&bqueue_ptr->lock);
}

void dequeue(bqueue_ *bqueue_ptr, void *data_ptr)
{
    pthread_mutex_lock(&bqueue_ptr->lock);
    while (bqueue_ptr->head == NULL)
    {
        pthread_cond_wait(&bqueue_ptr->new_data, &bqueue_ptr->lock);
    }
    data *tmp = bqueue_ptr->head;
    bqueue_ptr->head = bqueue_ptr->head->next;
    pthread_mutex_unlock(&bqueue_ptr->lock);

    memcpy(data_ptr, tmp->content, bqueue_ptr->content_size);
    free(tmp->content);
    free(tmp);
    return;
}

int get_size(bqueue_ *bqueue_ptr)
{
    pthread_mutex_lock(&bqueue_ptr->lock);
    int i = 0;
    data *m = bqueue_ptr->head;
    while (m != NULL)
    {
        m = m->next;
        i++;
    }
    pthread_mutex_unlock(&bqueue_ptr->lock);
    return i;
}

void bqueue_destroy(bqueue_ *bqueue_ptr)
{
    pthread_mutex_lock(&bqueue_ptr->lock);
    data *tmp = NULL;
    data *m = bqueue_ptr->head;
    while (m != NULL)
    {
        tmp = m;
        m = m->next;
        free(tmp->content);
        free(tmp);
    }
    pthread_mutex_unlock(&bqueue_ptr->lock);
    free(bqueue_ptr);
}