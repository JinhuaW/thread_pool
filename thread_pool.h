#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__
typedef int (*task_cb_t)(void *arg);
typedef struct thread_info TASK;
typedef struct thread_pool POOL;

POOL *pool_create(int size, int max_queue);
int pool_start(POOL *pool);
int pool_exit(POOL *pool);
int pool_new_task(POOL *pool, task_cb_t run, void *arg);
int pool_get_wait_tasks(POOL *pool);
#endif
