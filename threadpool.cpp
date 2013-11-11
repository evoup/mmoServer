#include "threadpool.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#define DEBUG 1
func_t task_get(struct tasklist_t *list, void **args)
{
    pthread_mutex_lock(&list->lock);
    while(list->taskcnt == 0)
        pthread_cond_wait(&list->not_empty, &list->lock);
    struct task_t *tmp;
    func_t func;
    tmp = list->head->next;
    func = tmp->func;
    *args =tmp->args;
    free(list->head);
    list->head = tmp;
    list->taskcnt--;
#if 0
    printf("taskcnt = %d\n", list->taskcnt);
#endif
    pthread_cond_broadcast(&list->not_full);
    pthread_mutex_unlock(&list->lock);
    return func;
}

void task_add(struct tasklist_t *list, func_t func, void *args)
{
    pthread_mutex_lock(&list->lock);
    while(list->taskcnt == list->task_max)
        pthread_cond_wait(&list->not_full, &list->lock);
    struct task_t  * neo =(task_t *) malloc(sizeof(*neo));
    neo->func = func;
    neo->args = args;
    neo->next = NULL;
    list->tail->next = neo;
    list->tail = neo;
    list->taskcnt++;
#if 0
    printf("task_add sucess\n");
    func(NULL);
#endif
    pthread_cond_broadcast(&list->not_empty);
    pthread_mutex_unlock(&list->lock);
}

void *do_pthread(void *tp)
{
    func_t func;
    struct pthreadpool_t *tpool = (pthreadpool_t *)tp;
    void *args;
    while(1)
    {
#if 0
        printf("do_pthread\n");
#endif
        func = tpool->tasklist.task_get(&tpool->tasklist, &args);
        (void)func(args);
    }
}

void tpool_wait(struct pthreadpool_t *tpool)
{
    int i;
    for(i = 0; i < tpool->maxpnum; i++)
        pthread_join(tpool->pids[i], NULL);
}

void tpool_init(struct pthreadpool_t *tpool, int pnum, int tm)
{
    tpool->pids = (pthread_t *)malloc(pnum * sizeof(pthread_t));
    tpool->maxpnum = pnum;
    tpool->tasklist.task_max = tm;
    pthread_mutex_init(&tpool->tasklist.lock, NULL);
    pthread_cond_init(&tpool->tasklist.not_empty, NULL);
    pthread_cond_init(&tpool->tasklist.not_full, NULL);

    tpool->tasklist.head = tpool->tasklist.tail = (task_t *)malloc(sizeof(*tpool->tasklist.head));
    tpool->tasklist.taskcnt = 0;
    tpool->tasklist.task_add = task_add;
    tpool->tasklist.task_get = task_get;

    tpool->do_pthread = do_pthread;
    tpool->tpool_init = tpool_init;

    int i;
    for(i = 0; i < pnum; i++){
        //int ret = pthread_create(&tpool->pids[i],
        pthread_create(&tpool->pids[i],
                NULL,
                tpool->do_pthread,
                tpool);

    }
}
