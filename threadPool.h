#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include <sys/param.h>
#include "osqueue.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>


typedef enum status {running, stopWithoutWaiting, stopAndWaiting} Status;


typedef struct task
{
    void (*func)(void* arg);
    void* args;
}Task;


typedef struct communication
{
    pthread_mutex_t mutex;
    pthread_cond_t threadSignal;
}Communication;

typedef struct thread_pool
{
    pthread_t *threads;
    int numOfThreads;
    Communication * communication;
    OSQueue *osQueue;
    volatile Status status;
}ThreadPool;




ThreadPool* tpCreate(int numOfThreads);

void tpDestroy(ThreadPool* threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool* threadPool, void (*computeFunc) (void *), void* param);

#endif
