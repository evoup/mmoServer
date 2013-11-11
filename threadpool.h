#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <pthread.h>
typedef void (*func_t)(void *args);//注意前面不是强制转换。是个函数名字,将函数定义为类型，这种语法太晦涩了！算不算函数指针？
struct task_t
{
	struct task_t *next;
	func_t func;
	void *args;
};
/**
 *任务链表结构
 */
struct tasklist_t
{
	struct task_t *head;
	struct task_t *tail;
	pthread_mutex_t lock;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
	int taskcnt;
	int task_max;
	void (*task_add)(struct tasklist_t *, func_t func, void *args);
	func_t (*task_get)(struct tasklist_t *, void **args);
};

/**
 *线程池结构
 */
struct pthreadpool_t
{
	pthread_t *pids;
	int maxpnum;

	struct tasklist_t tasklist;

	void (*tpool_init)(struct pthreadpool_t *tpool, int pnum, int tm);
	void *(*do_pthread)(void *);
	void (*tpool_wait)(struct pthreadpool_t *);

};

#endif
