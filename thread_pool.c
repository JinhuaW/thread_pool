#include "thread_pool.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

struct thread_info {
	task_cb_t run;
	void *priv;
	struct thread_info *next;
};

struct thread_pool {
	int size;
	int running;
	int num_queue;
	int queue_max;
	pthread_t *thread_id;
	pthread_cond_t cond;
	pthread_mutex_t mutex;
	TASK *task_queue;
	TASK *task_tail;
};

static void *thread_body(void *arg);
static void pool_task_enqueue(POOL *pool, TASK *task);
static TASK *pool_create_task(POOL *pool, task_cb_t run, void *arg);

static TASK *get_one_task(POOL *pool)
{
	TASK *task = NULL;
	if (!pool || NULL == pool->task_queue)
		return NULL;
	task = pool->task_queue;
	if (pool->task_queue == pool->task_tail) {
		pool->task_queue = NULL;
		pool->task_tail = NULL;
	} else {
		pool->task_queue = task->next;
	}
	pool->num_queue--;
	return task;
}


static void *thread_body(void *arg)
{
	POOL *pool = (POOL *)arg;
	TASK *task;
	while (pool->running) {
		task = NULL;
		pthread_mutex_lock(&pool->mutex);
		pthread_cond_wait(&pool->cond, &pool->mutex);
		pthread_mutex_unlock(&pool->mutex);
		while (pool->running) {
			pthread_mutex_lock(&pool->mutex);
			task = get_one_task(pool);
			pthread_mutex_unlock(&pool->mutex);
			pthread_testcancel();
			if (task) {
				task->run(task->priv);
				if (task->priv)
					free(task->priv);
				free(task);
			} else {
				break;
			}
		}
	}
	pthread_exit(NULL);
}

POOL *pool_create(int size, int queue_max)
{	
	POOL *pool = NULL;
	pool = (POOL *) malloc(sizeof(POOL));
	if (NULL == pool)
		return NULL;
	pool->size = size;
	pool->queue_max = queue_max;
	pool->running = 1;
	pool->num_queue = 0;
	pool->thread_id = (pthread_t *) malloc(sizeof(pthread_t) * size);
	pthread_mutex_init(&pool->mutex, NULL);
	pthread_cond_init(&pool->cond, NULL);
	pool->task_queue = NULL;
	pool->task_tail = NULL;	
	return pool;
}

int pool_start(POOL *pool)
{
	int i = 0;
	pool->running = 1;
	for (i = 0;i < pool->size;i++) {
		if (pthread_create(&pool->thread_id[i], NULL, thread_body, pool))
			return 1;
	}
	return 0;
}

int pool_exit(POOL *pool)
{
	TASK *task;
	int i;
	if (!pool)
		return 1;
	pthread_mutex_lock(&pool->mutex);
	pool->running = 0;	
	pthread_cond_broadcast(&pool->cond);
	pthread_mutex_unlock(&pool->mutex);
	for (i = 0; i < pool->size; i++) {
		pthread_join(pool->thread_id[i], NULL);
	}
	free(pool->thread_id);
	pthread_mutex_lock(&pool->mutex);
	while (pool->task_queue) {
		task = pool->task_queue;
		if (task->priv)
			free(task->priv);
		free(task);
		pool->task_queue = pool->task_queue->next;
	}
	pthread_mutex_unlock(&pool->mutex);
	pthread_mutex_destroy(&pool->mutex);
	pthread_cond_destroy(&pool->cond);
	free(pool);
	return 0;
}

static TASK *pool_create_task(POOL *pool, task_cb_t run, void *arg)
{
	TASK *task = NULL;
	task = (TASK *) malloc(sizeof(TASK));
	if (NULL == task)
		return NULL;
	task->run = run;
	task->next = NULL;
	task->priv = arg;
	return task;
}

static void pool_task_enqueue(POOL *pool, TASK *task)
{
	if (NULL == pool->task_queue)
		pool->task_queue = task;
	else
		pool->task_tail->next = task;
	pool->task_tail = task;
	pool->num_queue++;
}

/* arg can be NULL or heap buffer*/
int pool_new_task(POOL *pool, task_cb_t run, void *arg)
{
	TASK *task;
	if (NULL == pool || NULL == run)
		return 1;
	pthread_mutex_lock(&pool->mutex);
	if (pool->num_queue >= pool->queue_max) {
		pthread_mutex_unlock(&pool->mutex);
		return 1;
	}
	pthread_mutex_unlock(&pool->mutex);
	task = pool_create_task(pool, run, arg);
	if (NULL == task)
		return 1;
	pthread_mutex_lock(&pool->mutex);
	pool_task_enqueue(pool, task);
	pthread_cond_signal(&pool->cond);
	pthread_mutex_unlock(&pool->mutex);
	return 0;
}


int pool_get_wait_tasks(POOL *pool)
{
	int num = 0;
	pthread_mutex_lock(&pool->mutex);
	num = pool->num_queue;
	pthread_mutex_unlock(&pool->mutex);
	return num;
}
