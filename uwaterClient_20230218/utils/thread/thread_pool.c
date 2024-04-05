

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include "thread_pool.h"
#include <sys/syscall.h>

#define gettid()	syscall(SYS_gettid)
  
#define	THREAD_POOL_SHUTDOWN	1
#define	THREAD_POOL_WAIT	2

#define USE_THREAD_KEY

#ifdef USE_THREAD_KEY
pthread_key_t thread_key;
static pthread_once_t once = PTHREAD_ONCE_INIT;


static void once_routine(void)
{
	int status;

	printf("Initializing thread_key \n");
	status = pthread_key_create(&thread_key, NULL);
	if(status != 0){
		perror("pthread_key_create");
		exit(1);	
	}
}
#endif

static int check_thread_pool_task_maskbit(thread_pool_t *pool, int num)
{
	int size = sizeof(unsigned int) * 8;

	return (pool->task_mask[num/size] & (1 << num%size));
}

static void set_thread_pool_task_maskbit(thread_pool_t *pool, int num)
{
	int size = sizeof(unsigned int) * 8;

	pool->task_mask[num/size] |= (1 << num%size);
}

static void clean_thread_pool_task_maskbit(thread_pool_t *pool, int num)
{
	int size = sizeof(unsigned int) * 8;

	pool->task_mask[num/size] &= ~(1 << num%size);
}

void thread_pool_set_wait_handler(thread_pool_t *pool, void (*handler)(void *), void *arg)
{
	if(pool==NULL){
		return;
	}

	pool->wait_arg = arg;
	pool->wait_handler = handler;
}

void thread_pool_set_usleep(thread_pool_t *pool, int us)
{
	if(pool==NULL){
		return;
	}

	pool->sleep_us = us;
}

//syscall(__NR_gettid)

static void *thread_routine(void *arg)  
{
	thread_task_t *worker;
	thread_pool_t *pool = (thread_pool_t *)arg;
	int ret;
	
	pthread_mutex_lock(&(pool->queue_lock)); 
	#ifdef USE_THREAD_KEY
	if(pthread_setspecific(thread_key, (void *)pool->thread_qty) != 0){
		perror("pthread_setspecific");
		pthread_mutex_unlock(&(pool->queue_lock));  
		return NULL;
	}
	#endif
	pool->thread_qty++;
	pthread_mutex_unlock(&(pool->queue_lock));  
	
    	printf ("starting %d thread %d\n", pool->thread_qty, gettid());  
	while (1) {  

		while((pool->doflag != THREAD_POOL_SHUTDOWN)){
			ret = sem_trywait(&(pool->queue_sem));
			if(ret == EAGAIN){
				usleep(1000);
			}else if(ret == 0){
				break;
			}else{
				usleep(1000);
			}
			
		}

		//printf ("thread_routine get semaphore : curren task qty %d \n", pool->cur_queue_size);  

		if(pool->wait_handler){
			pool->wait_handler(pool->wait_arg);
		}

		pthread_mutex_lock(&(pool->queue_lock));  
			
		if((pool->cur_queue_size == 0) && (pool->doflag == THREAD_POOL_SHUTDOWN)){
			pthread_mutex_unlock(&(pool->queue_lock));  
			 //printf("thread 0x%x will exit\n", gettid());  
			pthread_exit(NULL);  
		} 
			
		   worker = pool->queue_head;
		if(worker == NULL){
			usleep(10000);
			pthread_mutex_unlock(&(pool->queue_lock));	
			continue;
		}

		pool->queue_head = worker->next;
		if(pool->queue_head == NULL){
			pool->queue_tail = NULL;
		}
		
		pthread_mutex_unlock(&(pool->queue_lock));  

#if 0
		printf ("thread 0x%x is starting to work, pool->cur_queue_size = %d, 0x%x\n"
				, gettid (), pool->cur_queue_size, worker->arg);
#endif
		//printf ("thread_routine begin exeute the task $$$$$$$$$$$$$$$$$$ \n");  

		(*(worker->process))(worker->task_arg.arg);

		usleep(pool->sleep_us);
		
		if(worker->task_arg.mask_bit >= 0){
				clean_thread_pool_task_maskbit(pool, worker->task_arg.mask_bit);
		}
		if(worker->task_arg.arg_size>4){
			free(worker->task_arg.arg);
		}
		free(worker);
			
		pool->cur_queue_size--; 
		
		worker = NULL;

		sched_yield();

	}
	
    pthread_exit(NULL);  
	
}  
  
 

thread_pool_t *create_thread_pool(int thread_qty, int stack_size)  
{  
    int i = 0; 
	pthread_attr_t attr, *p_attr = NULL;
 	thread_pool_t *pool = calloc(sizeof(thread_pool_t), 1);  

	if(pool == NULL){
		printf("calloc thread_pool faild, exit\n");
		exit(1);
	}
	
	#ifdef USE_THREAD_KEY
	if(pthread_once(&once, once_routine) != 0){
		perror("pthread_once");
		exit(1);
	}
	#endif
	
	pthread_mutex_init(&(pool->queue_lock), NULL);  
  //  pthread_cond_init(&(pool->queue_ready), NULL);  
	 sem_init(&(pool->queue_sem), 0, 0);
  
	pool->queue_head = NULL;
	pool->queue_tail = NULL;

	pool->thread_qty = 0;  
	pool->cur_queue_size = 0;  

	pool->wait_handler = NULL;
	pool->wait_arg = NULL;
	
	pool->doflag = 0;

	pool->sleep_us = 10000;

	pool->threadid = (pthread_t *)calloc(sizeof (pthread_t), thread_qty);
	if(pool->threadid == NULL){
		printf("calloc pool->threadid faild, exit\n");
		exit(1);
	}
	
	if(stack_size > 0){
		if(pthread_attr_init(&attr) != 0){
			printf( "pthread_attr_init failure\n");
		}else{
			if(pthread_attr_setstacksize(&attr, stack_size) != 0){
				printf( "pthread_attr_setstacksize failure\n");
			}else{
				p_attr = &attr;
			}
		}
	}

	 
    for (i = 0; i < thread_qty; i++){   
        pthread_create(&(pool->threadid[i]), p_attr, thread_routine, (void *)pool);  
    }
	
	if(p_attr){
		if(pthread_attr_destroy(p_attr)){
			printf( "pthread_attr_destroy failure\n");
		}
	}

	printf("create_thread_pool : %d thread, stack size = %d \n", thread_qty, stack_size);

	return pool;
}  
  
  
int thread_pool_add_task (thread_pool_t *pool, proc_fun_t process, task_arg_t *task_arg)  
{  

    thread_task_t *newworker;

	if(pool == NULL){
		return -1;
	}

	if((task_arg) && (task_arg->mask_bit >= 0)){
		if(check_thread_pool_task_maskbit(pool, task_arg->mask_bit)){
			return 1;
		}
	}

	newworker = (thread_task_t *)malloc(sizeof(thread_task_t));
	if(newworker == NULL){
		printf("malloc thread_task faild, exit\n");
		exit(1);
	}

	newworker->process = process;  
	newworker->next = NULL;

	if(task_arg){

		newworker->task_arg.mask_bit = task_arg->mask_bit;
		newworker->task_arg.arg_size = task_arg->arg_size;
		
		if(task_arg->arg_size>4){
			
		    newworker->task_arg.arg = malloc(task_arg->arg_size);
			if(newworker->task_arg.arg == NULL){
				printf("malloc task_arg.arg faild, exit\n");
				exit(1);
			}
			memcpy(newworker->task_arg.arg, task_arg->arg, task_arg->arg_size);
		}else{
			newworker->task_arg.arg = task_arg->arg;
		}

		if(newworker->task_arg.mask_bit >= 0){
			set_thread_pool_task_maskbit(pool, newworker->task_arg.mask_bit);
		}
	}

	//printf("******************thread_pool_add_task : %p , argsize %d **** begin **************************\n", process, task_arg->arg_size);

	pthread_mutex_lock(&(pool->queue_lock));  


	if(pool->queue_head != NULL){
		pool->queue_tail->next = newworker;  
	} else {  
		pool->queue_head = newworker;
	}
	pool->queue_tail = newworker;


	pool->cur_queue_size++;  
	pthread_mutex_unlock (&(pool->queue_lock)); 

	//pthread_cond_signal (&(pool->queue_ready)); 
	sem_post(&(pool->queue_sem));

	//printf("############# thread_pool_add_task : %p , argsize %d **** end #######################\n", process, task_arg->arg_size);
		
	return 0;  
}  

int thread_pool_exit(thread_pool_t *pool)  
{  
    int res, i; 
    thread_task_t *head = NULL;
	
	if(pool == NULL){
		return -1;
	}

	printf("thread_pool_destroy start: doflag = %d \n", pool->doflag);
	
	if (pool->doflag){
		return -1;
	}
	pool->doflag = THREAD_POOL_SHUTDOWN;  
   
  //  pthread_cond_broadcast(&(pool->queue_ready));    
 
    for(i = 0; i < pool->thread_qty; i++){
		//while(pthread_join(pool->threadid[i], NULL));
		 printf("thread_pool_destroy : pthread_cancel thread[%d] %#x \n", i, pool->threadid[i]);
		pthread_cancel(pool->threadid[i]);
    }
    free(pool->threadid);  

    while(pool->queue_head != NULL){  
        head = pool->queue_head;  
        pool->queue_head = pool->queue_head->next;  
        free (head);  
	 printf("thread_pool_destroy : cur_queue_size = %d \n", pool->cur_queue_size);
    }  

    pthread_mutex_destroy(&(pool->queue_lock));  
   // pthread_cond_destroy(&(pool->queue_ready));  
   sem_destroy(&(pool->queue_sem));

	  
printf("thread_pool_destroy end: cur_queue_size = %d \n", pool->cur_queue_size);
	
    free(pool); 

	
    return 0;  
}  
  
int thread_pool_destroy(thread_pool_t *pool)  
{  
    int res, i; 
    thread_task_t *head = NULL;
	
	if(pool == NULL){
		return -1;
	}

	printf("thread_pool_destroy start: doflag = %d \n", pool->doflag);
	
	if (pool->doflag){
		return -1;
	}
	pool->doflag = THREAD_POOL_SHUTDOWN;  
   
  //  pthread_cond_broadcast(&(pool->queue_ready));    
 
    for(i = 0; i < pool->thread_qty; i++){
		//while(pthread_join(pool->threadid[i], NULL));
		 printf("thread_pool_destroy : pthread_join thread[%d] %#x \n", i, pool->threadid[i]);
		pthread_join(pool->threadid[i], NULL);
    }
    free(pool->threadid);  

    while(pool->queue_head != NULL){  
        head = pool->queue_head;  
        pool->queue_head = pool->queue_head->next;  
        free (head);  
	 printf("thread_pool_destroy : cur_queue_size = %d \n", pool->cur_queue_size);
    }  

    pthread_mutex_destroy(&(pool->queue_lock));  
   // pthread_cond_destroy(&(pool->queue_ready));  
   sem_destroy(&(pool->queue_sem));

	  
printf("thread_pool_destroy end: cur_queue_size = %d \n", pool->cur_queue_size);
	
    free(pool); 

	
    return 0;  
}  

int thread_pool_wait(thread_pool_t *pool)  
{  
    int res, i; 
    thread_task_t *head = NULL;
	
	if(pool == NULL){
		return -1;
	}
	
	//printf("thread_pool_wait start: doflag = %d \n", pool->doflag);
	
    if (pool->doflag){ 
        return -1;
    }
    pool->doflag = THREAD_POOL_WAIT;  
   
   // pthread_cond_broadcast(&(pool->queue_ready));    


    while(pool->cur_queue_size>0){
	   //printf("thread_pool_wait : cur_queue_size = %d \n", pool->cur_queue_size);
       usleep(10000); 
    }  
 
 	pool->doflag = 0;
 	
	//printf("thread_pool_wait end: cur_queue_size = %d \n", pool->cur_queue_size);
	
    return 0;  
}  
