
#ifndef _SHMEM_MESSAGE_H_
#define _SHMEM_MESSAGE_H_


#include "shmem_obj.h"
#include "../utils/thread/thread_pool.h"

typedef struct {
	int	process_id;
	int	program_id;
	int	msgid;
	int	msglen;
	void *msgdata;
	int	buf_size;
	void *data_buf;
}msg_buf_t;



typedef struct {
	shmem_cond_t *cond;
	struct timeval lasttime;
	struct timeval timebase;
} shmem_msg_signal_t;


typedef struct _shmem_message_s shmem_message_t;

struct _shmem_message_s{
	char *shm_name;
	shmem_obj_t *p_shmem;
	int shm_size;
	int my_program_id;
	int my_process_id;
	shmem_msg_signal_t read_signal;
	shmem_msg_signal_t write_signal;

	//msg_buf_t *msg_buf;
};


typedef struct {
	int run_flag;
	thread_pool_t *task_thread_pool;
	void (*shm_task)(shmem_message_t *, msg_buf_t *);
}shm_task_mng_t;


void init_msg_buf(msg_buf_t *msg_buf, void *data_buf);
msg_buf_t *create_msg_buf(int buf_size);
void free_msg_buf(msg_buf_t *msg_buf);
void *msg_buf_alloc(msg_buf_t *msg_buf, int buf_size);

shmem_message_t *shmem_msg_init(shmem_obj_t *p_shmem, int my_program_id);
int shmem_msg_destroy(shmem_message_t *p_shm_msg);


int set_shmem_msg_rcv_basetime(shmem_message_t *p_shm_msg);

int shmem_msg_rcv(shmem_message_t *p_shm_msg, msg_buf_t *msg_buf, int wait_ms);
int shmem_msg_send(shmem_message_t *p_shm_msg, msg_buf_t *msg_buf, int wait_ms);

shm_task_mng_t *init_shm_task_mng(int thread_qty, void (*shm_task_handler)(shmem_message_t *, msg_buf_t *), int stack_size);
void free_shm_task_mng(shm_task_mng_t *p_shm_task_mng);
void shm_task_startup(shm_task_mng_t *p_shm_task_mng, shmem_obj_t *p_shmem, int my_program_id);
void end_shm_task(shm_task_mng_t *p_shm_task_mng);



#endif
