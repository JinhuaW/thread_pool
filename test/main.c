#include "thread_pool.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

int rand_int()
{
	struct timeval tpstart;
	int n;
	gettimeofday(&tpstart,NULL);
	srand(tpstart.tv_usec);
	n = (1+(int) (60.0*rand()/(RAND_MAX+1.0)));
	return n;
}

int run(void *arg)
{
	printf(".");
	fflush(stdout);
	sleep(rand_int());
}

int main(int argc, char **argv)
{
	int i, thread_count = 0;
	char ch;
	POOL *pool;

	if (argc < 2 || (thread_count = atoi(argv[1])) <= 0 || thread_count > 100000) {
		printf("Usage: %s size\n"
				"Options:\n"
				"	size	-thread pool size, between 1~100000;\n", argv[0]);
		return 1;
	}

	pool = pool_create(thread_count, 100);
	if (pool == NULL) {
		printf("Create a pool with size (%d) failed.\n", thread_count);
		return 1;
	}

	if (pool_start(pool)) {
		printf("Start pool failed\n");
		return 1;
	}

	while ((ch = getchar()) != 'e') {
		switch (ch) {
			case 'n':
				pool_new_task(pool, run, (void *)NULL);
				break;
			case 'g':
				printf("current tasks in queue is %d\n", pool_get_wait_tasks(pool)); 
				break;
			case 'q':
				printf("exiting pool!\n");
				pool_exit(pool);
				return 0;
			default:
				break;
		}
	}
	return 0;
}
