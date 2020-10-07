#include <unistd.h>
#include "threadPool.h"


void Initialize(ThreadPool *pool, int numOfThreads) {
    pool->status1 = CONNECTED;
    pool->pool_size = numOfThreads;
    pool->queue = osCreateQueue();
    if (pthread_mutex_init(&pool->pthreadMutex, NULL) != 0) {
        freeAll(pool);
        errorMes();
    }
    if (pthread_cond_init(&pool->pthreadCond, NULL) != 0) {
        freeAll(pool);
        errorMes();
    }

}


ThreadPool *tpCreate(int numOfThreads) {
    int i;
    if (numOfThreads <= 0) {
        return NULL;
    }
    // Create thread pool
    ThreadPool *pool = (ThreadPool *) calloc(sizeof(ThreadPool), 1);
    if (!pool)
        errorMes();
    Initialize(pool, numOfThreads);
    pool->threads = (pthread_t *) calloc(sizeof(pthread_t), (size_t) numOfThreads);
    if (!pool->threads)
        errorMes();

    // Start all creation
    for (i = 0; i < numOfThreads; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, run, (void *) pool) != 0) {
            tpDestroy(pool, 0);
            errorMes();
        }
    }
    return pool;
}

void errorMes() {
    write(2, ERROR_MESS, strlen(ERROR_MESS));
    exit(-1);
}

int tpInsertTask(ThreadPool *pool, void (*computeFunc)(void *), void *param) {
    if (pool->status1 != CONNECTED) {
        return -1;
    }
    task_t *task = (task_t *) calloc(sizeof(task_t), 1);
    if (!task)
        errorMes();

    task->computeFunc = computeFunc;
    task->args = param;
    pthread_mutex_lock(&(pool->pthreadMutex));

    osEnqueue(pool->queue, task);
    if (pthread_cond_broadcast(&(pool->pthreadCond)) != 0) {
        freeAll(pool);
        errorMes();
    }
    pthread_mutex_unlock(&(pool->pthreadMutex));

    return 0;
}

static void *run(void *arg) {
    ThreadPool *pool = (ThreadPool *) arg;
    //Status in STOP_HIGH_LEVEL mode
    while (pool->status1 != STOP_HIGH_LEVEL) {
        if (!osIsQueueEmpty(pool->queue) || pool->status1 == CONNECTED) {
            pthread_mutex_lock(&pool->pthreadMutex);
            while (osIsQueueEmpty(pool->queue)) {
                if (pool->status1 == CONNECTED) {
                    pthread_cond_wait(&(pool->pthreadCond), &(pool->pthreadMutex));
                } else { break; }
            }
            // Made task
            task_t *task = (task_t *) osDequeue(pool->queue);
            pthread_mutex_unlock(&pool->pthreadMutex);
            if (!task)
                continue;
            ((task->computeFunc))(task->args);
            free(task);
        } else { break; }
    }
    pthread_mutex_unlock(&(pool->pthreadMutex));
    pthread_exit(NULL);
}

void tpDestroy(ThreadPool *pool, int shouldWaitForTasks) {
    int i;
    pthread_mutex_lock(&pool->pthreadMutex);
    //update
    if (shouldWaitForTasks != 0)
        pool->status1 = STOP_LOW_LEVEL;
    else
        pool->status1 = STOP_HIGH_LEVEL;

    if (pthread_cond_broadcast(&(pool->pthreadCond)) != 0) {
        freeAll(pool);
        errorMes();
    }
    pthread_mutex_unlock(&pool->pthreadMutex);

    for (i = 0; i < pool->pool_size; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            errorMes();
        }
    }
    //free all
    freeAll(pool);
}

void freeAll(ThreadPool *pool) {
    while (!osIsQueueEmpty(pool->queue)) {
        free(osDequeue(pool->queue));
    }
    free(pool->threads);
    osDestroyQueue(pool->queue);
    pthread_mutex_destroy(&pool->pthreadMutex);
    pthread_cond_destroy(&pool->pthreadCond);
    free(pool);
}

