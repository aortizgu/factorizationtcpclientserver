#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/prctl.h>

#include "thpool.h"

static volatile int threads_keepalive;
static volatile int threads_on_hold;

typedef struct bsem
{
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int v;
} bsem;

typedef struct job
{
    struct job *prev;
    void (*function)(void *arg);
    void *arg;
} job;

typedef struct jobqueue
{
    pthread_mutex_t rwmutex;
    job *front;
    job *rear;
    bsem *has_jobs;
    int len;
} jobqueue;

typedef struct thread
{
    int id;
    pthread_t pthread;
    struct thpool_ *thpool_p;
} thread;

typedef struct thpool_
{
    thread **threads;
    volatile int num_threads_alive;
    volatile int num_threads_working;
    pthread_mutex_t thcount_lock;
    pthread_cond_t threads_all_idle;
    jobqueue jobqueue;
} thpool_;

static int thread_init(thpool_ *thpool_p, struct thread **thread_p, int id);
static void *thread_do(struct thread *thread_p);
static void thread_hold(int sig_id);
static void thread_destroy(struct thread *thread_p);

static int jobqueue_init(jobqueue *jobqueue_p);
static void jobqueue_clear(jobqueue *jobqueue_p);
static void jobqueue_push(jobqueue *jobqueue_p, struct job *newjob_p);
static struct job *jobqueue_pull(jobqueue *jobqueue_p);
static void jobqueue_destroy(jobqueue *jobqueue_p);

static void bsem_init(struct bsem *bsem_p, int value);
static void bsem_reset(struct bsem *bsem_p);
static void bsem_post(struct bsem *bsem_p);
static void bsem_post_all(struct bsem *bsem_p);
static void bsem_wait(struct bsem *bsem_p);

struct thpool_ *thpool_init(int num_threads)
{

    threads_on_hold = 0;
    threads_keepalive = 1;

    if (num_threads < 0)
    {
        num_threads = 0;
    }

    thpool_ *thpool_p;
    thpool_p = (struct thpool_ *)malloc(sizeof(struct thpool_));
    if (thpool_p == NULL)
    {
        fprintf(stderr, "thpool_init: Could not allocate memory for thread pool\n");
        return NULL;
    }
    thpool_p->num_threads_alive = 0;
    thpool_p->num_threads_working = 0;

    if (jobqueue_init(&thpool_p->jobqueue) == -1)
    {
        fprintf(stderr, "thpool_init: Could not allocate memory for job queue\n");
        free(thpool_p);
        return NULL;
    }

    thpool_p->threads = (struct thread **)malloc(num_threads * sizeof(struct thread *));
    if (thpool_p->threads == NULL)
    {
        fprintf(stderr, "thpool_init: Could not allocate memory for threads\n");
        jobqueue_destroy(&thpool_p->jobqueue);
        free(thpool_p);
        return NULL;
    }

    pthread_mutex_init(&(thpool_p->thcount_lock), NULL);
    pthread_cond_init(&thpool_p->threads_all_idle, NULL);

    int n;
    for (n = 0; n < num_threads; n++)
    {
        thread_init(thpool_p, &thpool_p->threads[n], n);
    }

    while (thpool_p->num_threads_alive != num_threads)
    {
    }

    return thpool_p;
}

int thpool_add_work(thpool_ *thpool_p, void (*function_p)(void *), void *arg_p)
{
    job *newjob;

    newjob = (struct job *)malloc(sizeof(struct job));
    if (newjob == NULL)
    {
        fprintf(stderr, "thpool_add_work: Could not allocate memory for new job\n");
        return -1;
    }

    newjob->function = function_p;
    newjob->arg = arg_p;

    jobqueue_push(&thpool_p->jobqueue, newjob);

    return 0;
}

void thpool_wait(thpool_ *thpool_p)
{
    pthread_mutex_lock(&thpool_p->thcount_lock);
    while (thpool_p->jobqueue.len || thpool_p->num_threads_working)
    {
        pthread_cond_wait(&thpool_p->threads_all_idle, &thpool_p->thcount_lock);
    }
    pthread_mutex_unlock(&thpool_p->thcount_lock);
}

void thpool_destroy(thpool_ *thpool_p)
{
    if (thpool_p == NULL)
    {
        return;
    }

    volatile int threads_total = thpool_p->num_threads_alive;

    threads_keepalive = 0;

    double TIMEOUT = 1.0;
    time_t start, end;
    double tpassed = 0.0;
    time(&start);
    while (tpassed < TIMEOUT && thpool_p->num_threads_alive)
    {
        bsem_post_all(thpool_p->jobqueue.has_jobs);
        time(&end);
        tpassed = difftime(end, start);
    }

    while (thpool_p->num_threads_alive)
    {
        bsem_post_all(thpool_p->jobqueue.has_jobs);
        sleep(1);
    }

    jobqueue_destroy(&thpool_p->jobqueue);
    int n;
    for (n = 0; n < threads_total; n++)
    {
        thread_destroy(thpool_p->threads[n]);
    }
    free(thpool_p->threads);
    free(thpool_p);
}

int thpool_num_threads_working(thpool_ *thpool_p)
{
    return thpool_p->num_threads_working;
}

static int thread_init(thpool_ *thpool_p, struct thread **thread_p, int id)
{

    *thread_p = (struct thread *)malloc(sizeof(struct thread));
    if (*thread_p == NULL)
    {
        fprintf(stderr, "thread_init: Could not allocate memory for thread\n");
        return -1;
    }

    (*thread_p)->thpool_p = thpool_p;
    (*thread_p)->id = id;

    pthread_create(&(*thread_p)->pthread, NULL, (void *(*)(void *))thread_do, (*thread_p));
    pthread_detach((*thread_p)->pthread);
    return 0;
}

static void thread_hold(int sig_id)
{
    (void)sig_id;
    threads_on_hold = 1;
    while (threads_on_hold)
    {
        sleep(1);
    }
}

static void *thread_do(struct thread *thread_p)
{

    char thread_name[16] = {0};
    snprintf(thread_name, 16, "thpool-%d", thread_p->id);

    prctl(PR_SET_NAME, thread_name);

    thpool_ *thpool_p = thread_p->thpool_p;

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = thread_hold;
    if (sigaction(SIGUSR1, &act, NULL) == -1)
    {
        fprintf(stderr, "thread_do: cannot handle SIGUSR1");
    }

    pthread_mutex_lock(&thpool_p->thcount_lock);
    thpool_p->num_threads_alive += 1;
    pthread_mutex_unlock(&thpool_p->thcount_lock);

    while (threads_keepalive)
    {

        bsem_wait(thpool_p->jobqueue.has_jobs);

        if (threads_keepalive)
        {

            pthread_mutex_lock(&thpool_p->thcount_lock);
            thpool_p->num_threads_working++;
            pthread_mutex_unlock(&thpool_p->thcount_lock);

            void (*func_buff)(void *);
            void *arg_buff;
            job *job_p = jobqueue_pull(&thpool_p->jobqueue);
            if (job_p)
            {
                func_buff = job_p->function;
                arg_buff = job_p->arg;
                func_buff(arg_buff);
                free(job_p);
            }

            pthread_mutex_lock(&thpool_p->thcount_lock);
            thpool_p->num_threads_working--;
            if (!thpool_p->num_threads_working)
            {
                pthread_cond_signal(&thpool_p->threads_all_idle);
            }
            pthread_mutex_unlock(&thpool_p->thcount_lock);
        }
    }
    pthread_mutex_lock(&thpool_p->thcount_lock);
    thpool_p->num_threads_alive--;
    pthread_mutex_unlock(&thpool_p->thcount_lock);

    return NULL;
}

static void thread_destroy(thread *thread_p)
{
    free(thread_p);
}

static int jobqueue_init(jobqueue *jobqueue_p)
{
    jobqueue_p->len = 0;
    jobqueue_p->front = NULL;
    jobqueue_p->rear = NULL;

    jobqueue_p->has_jobs = (struct bsem *)malloc(sizeof(struct bsem));
    if (jobqueue_p->has_jobs == NULL)
    {
        return -1;
    }

    pthread_mutex_init(&(jobqueue_p->rwmutex), NULL);
    bsem_init(jobqueue_p->has_jobs, 0);

    return 0;
}

static void jobqueue_clear(jobqueue *jobqueue_p)
{

    while (jobqueue_p->len)
    {
        free(jobqueue_pull(jobqueue_p));
    }

    jobqueue_p->front = NULL;
    jobqueue_p->rear = NULL;
    bsem_reset(jobqueue_p->has_jobs);
    jobqueue_p->len = 0;
}

static void jobqueue_push(jobqueue *jobqueue_p, struct job *newjob)
{

    pthread_mutex_lock(&jobqueue_p->rwmutex);
    newjob->prev = NULL;

    switch (jobqueue_p->len)
    {

    case 0:
        jobqueue_p->front = newjob;
        jobqueue_p->rear = newjob;
        break;

    default:
        jobqueue_p->rear->prev = newjob;
        jobqueue_p->rear = newjob;
    }
    jobqueue_p->len++;

    bsem_post(jobqueue_p->has_jobs);
    pthread_mutex_unlock(&jobqueue_p->rwmutex);
}

static struct job *jobqueue_pull(jobqueue *jobqueue_p)
{

    pthread_mutex_lock(&jobqueue_p->rwmutex);
    job *job_p = jobqueue_p->front;

    switch (jobqueue_p->len)
    {

    case 0:
        break;

    case 1:
        jobqueue_p->front = NULL;
        jobqueue_p->rear = NULL;
        jobqueue_p->len = 0;
        break;

    default:
        jobqueue_p->front = job_p->prev;
        jobqueue_p->len--;
        bsem_post(jobqueue_p->has_jobs);
    }

    pthread_mutex_unlock(&jobqueue_p->rwmutex);
    return job_p;
}

static void jobqueue_destroy(jobqueue *jobqueue_p)
{
    jobqueue_clear(jobqueue_p);
    free(jobqueue_p->has_jobs);
}

static void bsem_init(bsem *bsem_p, int value)
{
    if (value < 0 || value > 1)
    {
        fprintf(stderr, "bsem_init: Binary semaphore can take only values 1 or 0");
        exit(1);
    }
    pthread_mutex_init(&(bsem_p->mutex), NULL);
    pthread_cond_init(&(bsem_p->cond), NULL);
    bsem_p->v = value;
}

static void bsem_reset(bsem *bsem_p)
{
    bsem_init(bsem_p, 0);
}

static void bsem_post(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_signal(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

static void bsem_post_all(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);
    bsem_p->v = 1;
    pthread_cond_broadcast(&bsem_p->cond);
    pthread_mutex_unlock(&bsem_p->mutex);
}

static void bsem_wait(bsem *bsem_p)
{
    pthread_mutex_lock(&bsem_p->mutex);
    while (bsem_p->v != 1)
    {
        pthread_cond_wait(&bsem_p->cond, &bsem_p->mutex);
    }
    bsem_p->v = 0;
    pthread_mutex_unlock(&bsem_p->mutex);
}
