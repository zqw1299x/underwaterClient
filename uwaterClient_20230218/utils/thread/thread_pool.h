#ifndef __THREAD_POOL_H__
#define __THREAD_POOL_H__

#include <pthread.h>
#include <semaphore.h>


extern pthread_key_t thread_key;

typedef void *(*proc_fun_t)(void *);

typedef struct {
	int mask_bit;
	int arg_size;
	void *arg;
}task_arg_t;

typedef struct _thread_task_s{ 
    proc_fun_t process;
    task_arg_t task_arg;
    struct _thread_task_s *next;
} thread_task_t;
  

typedef struct{

	int pool_id;
	
	pthread_mutex_t queue_lock;
	//pthread_cond_t queue_ready;

	sem_t queue_sem;

    thread_task_t *queue_head;
	thread_task_t *queue_tail;
    pthread_t *threadid;
    int thread_qty;
    int cur_queue_size;

	int sleep_us;

	void (*wait_handler)(void *);
	void *wait_arg;
	
    char doflag;
	char detach_status;
	char run_status;
	char reserved;


	unsigned int task_mask[32];

} thread_pool_t;

typedef struct {
	thread_pool_t *pool_head;
	int pool_count;
}thread_pool_manager_t;

thread_pool_t *create_thread_pool(int thread_qty, int stack_size);
int thread_pool_add_task(thread_pool_t *pool, proc_fun_t process, task_arg_t *task_arg);
int thread_pool_destroy(thread_pool_t *pool);
int thread_pool_wait(thread_pool_t *pool);
void thread_pool_set_wait_handler(thread_pool_t *pool, void (*handler)(void *), void *arg);
void thread_pool_set_usleep(thread_pool_t *pool, int us);

#endif
