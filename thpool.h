#ifndef _THPOOL_
#define _THPOOL_

typedef struct thpool_ *threadpool;

/**
 * @brief  Initialize threadpool
 */
threadpool thpool_init(int num_threads);

/**
 * @brief Add work to the job queue
 */
int thpool_add_work(threadpool, void (*function_p)(void *), void *arg_p);

/**
 * @brief Wait for all queued jobs to finish
 */
void thpool_wait(threadpool);

/**
 * @brief Destroy the threadpool
 */
void thpool_destroy(threadpool);

/**
 * @brief Show currently working threads
 */
int thpool_num_threads_working(threadpool);

#endif
