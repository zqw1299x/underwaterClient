

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>

#include "shmem_msg.h"

void *msg_buf_alloc(msg_buf_t *msg_buf, int buf_size)
{

	if(msg_buf && (buf_size>0)){
		msg_buf->data_buf = malloc(buf_size);
		if(msg_buf->data_buf == NULL){
			Info_Printf("malloc msg_buf_t data_buf is Failure, exit\n");
			exit(1);
		}

		msg_buf->buf_size = buf_size;
		msg_buf->msgdata = msg_buf->data_buf;

		return msg_buf->msgdata;
	}else{
		return NULL;
	}
}


msg_buf_t *create_msg_buf(int buf_size)
{
	msg_buf_t *msg_buf;

	msg_buf = malloc(sizeof(msg_buf_t));
	if(msg_buf == NULL){
		Info_Printf("malloc msg_buf_t is Failure, exit\n");
		exit(1);
	}

	if(buf_size>0){
		msg_buf->data_buf = malloc(buf_size);
		if(msg_buf->data_buf == NULL){
			Info_Printf("malloc msg_buf_t data_buf is Failure, exit\n");
			exit(1);
		}
	}else{
		msg_buf->data_buf = NULL;
		//buf_size = 0;
	}

	msg_buf->msgid = 0;
	msg_buf->msglen = 0;
	msg_buf->process_id = -1;
	msg_buf->program_id = -1;

	msg_buf->buf_size = buf_size;
	msg_buf->msgdata = msg_buf->data_buf;

	return msg_buf;
}

void init_msg_buf(msg_buf_t *msg_buf, void *data_buf)
{

	if(msg_buf == NULL){
		return;
	}

	msg_buf->msgdata = data_buf;

	msg_buf->msgid = 0;
	msg_buf->msglen = 0;
	msg_buf->process_id = -1;
	msg_buf->program_id = -1;

	msg_buf->buf_size = 0;
	msg_buf->data_buf = NULL;
}


void free_msg_buf(msg_buf_t *msg_buf)
{

	if(msg_buf){
		if(msg_buf->data_buf && msg_buf->buf_size != 0){
			free(msg_buf->data_buf);
		}
		free(msg_buf);
	}
}


static void shmem_msg_signal_init(shmem_message_t *p_shm_msg)
{

	if(p_shm_msg == NULL){
		return;
	}

	p_shm_msg->read_signal.cond = &p_shm_msg->p_shmem->rw_lock.cond_reader;
	p_shm_msg->read_signal.lasttime.tv_sec = p_shm_msg->read_signal.cond->timestamp.tv_sec;
	p_shm_msg->read_signal.lasttime.tv_usec = p_shm_msg->read_signal.cond->timestamp.tv_usec;
	p_shm_msg->read_signal.timebase.tv_sec = 0;
	p_shm_msg->read_signal.timebase.tv_usec = 0;
	
	p_shm_msg->write_signal.cond = &p_shm_msg->p_shmem->rw_lock.cond_writer;
	p_shm_msg->write_signal.lasttime.tv_sec = p_shm_msg->write_signal.cond->timestamp.tv_sec;
	p_shm_msg->write_signal.lasttime.tv_usec = p_shm_msg->write_signal.cond->timestamp.tv_usec;
	p_shm_msg->write_signal.timebase.tv_sec = 0;
	p_shm_msg->write_signal.timebase.tv_usec = 0;
	
}


static int shmem_msg_wait(shmem_msg_signal_t *p_msg_signal, int wait_ms)
{
	int result = 0;
	shmem_cond_t *cond;
	struct timeval *timebase;
	
	struct timeval *last_time;


	struct timeval now_time = {
		.tv_sec = 0,
		.tv_usec = 0,
	};

	if(p_msg_signal == NULL){
		return -1;
	}

	cond = p_msg_signal->cond;
	timebase = &p_msg_signal->timebase;
	last_time = &p_msg_signal->lasttime;


	Debug_Printf("---cond_wait, signal_type = %d, semaphore = %d,last_time tv_sec = %u, tv_usec = %u ---\n"
		, cond->signal_type, cond->semaphore, (u_int)last_time->tv_sec, (u_int)last_time->tv_usec);
	Debug_Printf("---cond_wait, timebase : tv_sec = %u, tv_usec = %u : timeout = %d ---\n", (u_int)timebase->tv_sec, (u_int)timebase->tv_usec, wait_ms);
			


	if(cond->signal_type){

		while(cond->semaphore <= 0){

			if(wait_ms>0){
				if(gettimeofday(&now_time, NULL) == 0){
					if(((now_time.tv_sec - timebase->tv_sec)*1000 + (now_time.tv_usec - timebase->tv_usec)/1000)>wait_ms){
						Info_Printf("cond_wait timeout, signal_type = %d, sec = %d, usec = %d\n", cond->signal_type, now_time.tv_sec, now_time.tv_usec);
						result = SHM_WAIT_TIMEOUT;

						break;
					}
				}
			}
			usleep(1000);

		}


		cond->semaphore = 0;
		
	}else{


		while((cond->timestamp.tv_sec == last_time->tv_sec) && (cond->timestamp.tv_usec == last_time->tv_usec)){


			if(wait_ms>0){
				if(gettimeofday(&now_time, NULL) == 0){
					if(((now_time.tv_sec - timebase->tv_sec)*1000 + (now_time.tv_usec - timebase->tv_usec)/1000)>wait_ms){
						Info_Printf("cond_wait timeout, signal_type = %d, sec = %d, usec = %d\n", cond->signal_type, now_time.tv_sec, now_time.tv_usec);
						result = SHM_WAIT_TIMEOUT;

						break;
					}
				}
			}
			
			usleep(1000);

		}

		
		last_time->tv_sec = cond->timestamp.tv_sec;
		last_time->tv_usec = cond->timestamp.tv_usec;

	}

	Debug_Printf("===end_wait , signal_type = %d, semaphore = %d, tv_sec = %u, tv_usec = %u ===\n"
		, cond->signal_type, cond->semaphore, (u_int)last_time->tv_sec, (u_int)last_time->tv_usec);
	
	return result;
}

shmem_message_t *shmem_msg_init(shmem_obj_t *p_shmem, int my_program_id)
{

	shmem_message_t *p_shm_msg;

	
	if(p_shmem == NULL){
		return NULL;
	}
	

	p_shm_msg = malloc(sizeof(shmem_message_t));
	if(p_shm_msg == NULL){
		Info_Printf("malloc shmem_message failed, exit\n");
		exit(1);
	}

	//p_shmem = shmem_init(ipc_key, shm_size);

	p_shm_msg->shm_name = p_shmem->shm_name;
	p_shm_msg->shm_size = p_shmem->shm_size;

	p_shm_msg->my_program_id = my_program_id;
	p_shm_msg->my_process_id = getpid();

	Info_Printf("attaches memory success , my program_id = %d, my process_id = %d! \n"
		, p_shm_msg->my_program_id, p_shm_msg->my_process_id);

	//p_shm_msg->msg_buf = NULL;

	p_shm_msg->p_shmem = p_shmem;

	
	shmem_msg_signal_init(p_shm_msg);


	return p_shm_msg;
}


int shmem_msg_destroy(shmem_message_t *p_shm_msg)
{

	shmem_obj_t *p_shmem = p_shm_msg->p_shmem;

	//shmem_destroy(p_shmem);

	Info_Printf(" %s : %d \n", p_shm_msg->shm_name, p_shmem->shm_size);
	free(p_shm_msg);

	return 0;
}


int set_shmem_msg_rcv_basetime(shmem_message_t *p_shm_msg)
{
	if(p_shm_msg == NULL){
		return -1;
	}
	
	return gettimeofday(&p_shm_msg->read_signal.timebase, NULL);
}


int shmem_msg_rcv(shmem_message_t *p_shm_msg, msg_buf_t *msg_buf, int wait_ms)
{
	
	shmem_obj_t *p_shmem = p_shm_msg->p_shmem;
	
	int ret, do_flag = 0;

	Debug_Printf("timeout = %d \n", wait_ms);

	do{



		ret = shmem_msg_wait(&(p_shm_msg->read_signal), wait_ms);

		if(ret == SHM_WAIT_TIMEOUT){

			shmem_cond_signal(p_shm_msg->write_signal.cond);

			return SHM_WAIT_TIMEOUT;
		}else if(ret < 0){

			return ret;
		}
			

		
		Debug_Printf("ref_to.program_id = %d, ref_to.process_id = %d\n", p_shmem->ref_to.program_id, p_shmem->ref_to.process_id);
		
		shmem_rw_lock(p_shmem, SHM_READ_FLAG);


		if(p_shmem->ref_to.process_id == p_shm_msg->my_process_id){
			do_flag = 1;
		}else if((p_shmem->ref_to.process_id == 0) && (p_shmem->ref_to.program_id == p_shm_msg->my_program_id)){
			do_flag = 1;
		}else if((p_shmem->ref_to.program_id == 0) && (p_shmem->ref_to.process_id == 0)){
			do_flag = 1;
		}else{

			shmem_rw_unlock(p_shmem, SHM_READ_FLAG);

			usleep(1000);
			continue;
			
			//do_flag = 0;
			//goto Shm_Listen_End;
		}

		//p_shmem->ref_to.process_id = -1;
		//p_shmem->ref_to.program_id = -1;

		if(msg_buf){
			
			msg_buf->program_id = p_shmem->ref_from.program_id;
			msg_buf->process_id = p_shmem->ref_from.process_id;
			memcpy(&msg_buf->msgid, p_shmem->data_buf, sizeof(msg_buf->msgid));
			msg_buf->msglen = p_shmem->data_len - sizeof(msg_buf->msgid);
			if(msg_buf->msgdata){
				memcpy(msg_buf->msgdata, p_shmem->data_buf + sizeof(msg_buf->msgid), msg_buf->msglen);
			}else if((msg_buf->buf_size < 0)){
				msg_buf->msgdata = malloc(msg_buf->msglen+1);
				if(msg_buf->msgdata==NULL){
					perror("shmem msgdata");
					exit(-1);
				}
				memcpy(msg_buf->msgdata, p_shmem->data_buf + sizeof(msg_buf->msgid), msg_buf->msglen);
				((char *)msg_buf->msgdata)[msg_buf->msglen] = 0;
				msg_buf->data_buf = msg_buf->msgdata;
			}
			
			shmem_cond_signal(p_shm_msg->write_signal.cond);
		}

	
		//Shm_Listen_End :
		shmem_rw_unlock(p_shmem, SHM_READ_FLAG);

	}while(do_flag <= 0);
	

	Debug_Printf("ref_to.program_id = %d, ref_to.process_id = %d\n", p_shmem->ref_to.program_id, p_shmem->ref_to.process_id);
	Debug_Printf("ref_from.program_id = %d, ref_from.process_id = %d\n", p_shmem->ref_from.program_id, p_shmem->ref_from.process_id);

	return do_flag;
}

int shmem_msg_send(shmem_message_t *p_shm_msg, msg_buf_t *msg_buf, int wait_ms)
{
	int ret;
	shmem_obj_t *p_shmem = NULL;

	

	if((p_shm_msg == NULL) || (msg_buf == NULL)){
		return -1;
	}

	p_shmem = p_shm_msg->p_shmem;

		
	gettimeofday(&p_shm_msg->write_signal.timebase, NULL);
	ret = shmem_msg_wait(&(p_shm_msg->write_signal), wait_ms);
	gettimeofday(&p_shm_msg->write_signal.timebase, NULL);

	if(ret < 0){
		shmem_cond_signal(p_shm_msg->write_signal.cond);
		return ret;
	}
	


	shmem_rw_lock(p_shmem, SHM_WRITE_FLAG);
	
	memcpy(p_shmem->data_buf, &msg_buf->msgid, sizeof(msg_buf->msgid));

	if(msg_buf->msglen < 0){
		msg_buf->msglen = 0;
	}

	p_shmem->data_len = msg_buf->msglen + sizeof(msg_buf->msgid);

	if((msg_buf->msgdata != NULL) && (msg_buf->msglen > 0)){
		memcpy(p_shmem->data_buf + sizeof(msg_buf->msgid), msg_buf->msgdata, msg_buf->msglen);
	}

	p_shmem->ref_to.program_id = msg_buf->program_id;
	p_shmem->ref_to.process_id = msg_buf->process_id;

	p_shmem->ref_from.program_id = p_shm_msg->my_program_id;
	p_shmem->ref_from.process_id = p_shm_msg->my_process_id;
	
	shmem_cond_broadcast(p_shm_msg->read_signal.cond);
		
	shmem_rw_unlock(p_shmem, SHM_WRITE_FLAG);

	gettimeofday(&p_shm_msg->read_signal.timebase, NULL);
	
	Debug_Printf("program_id = %d, process_id = %d , msg_id = %d, msg len = %d----\n"
		, msg_buf->program_id, msg_buf->process_id, msg_buf->msgid, msg_buf->msglen);

	
	
	return msg_buf->msglen;
}


shm_task_mng_t *init_shm_task_mng(int thread_qty, void (*shm_task_handler)(shmem_message_t *, msg_buf_t *), int stack_size)
{
	shm_task_mng_t *p_task_msg;

	p_task_msg = malloc(sizeof(shm_task_mng_t));

	if(p_task_msg == NULL){
		Info_Printf("malloc  shm_task_mng_t is Failure, exit\n");
		exit(1);
	}
	
	p_task_msg->task_thread_pool = create_thread_pool(thread_qty, stack_size);
	p_task_msg->shm_task = shm_task_handler;

	p_task_msg->run_flag = 0;

	return p_task_msg;
}


void free_shm_task_mng(shm_task_mng_t *p_shm_task_mng)
{
	if(p_shm_task_mng == NULL){
		return;
	}

	thread_pool_destroy(p_shm_task_mng->task_thread_pool);

	free(p_shm_task_mng);
	
}

typedef struct {
	shmem_message_t *p_shm_msg;
	msg_buf_t *msg_buf;
	void (*shm_task)(shmem_message_t *, msg_buf_t *);
}shm_task_args_t;

void shmem_msg_task_handler(shm_task_args_t *shm_task_args)
{
	if(shm_task_args == NULL){
		return;
	}

	shm_task_args->shm_task(shm_task_args->p_shm_msg, shm_task_args->msg_buf);
	free_msg_buf(shm_task_args->msg_buf);
	
	//shmem_msg_destroy(shm_task_args->p_shm_msg);
	
}


void end_shm_task(shm_task_mng_t *p_shm_task_mng)
{
	p_shm_task_mng->run_flag  = 0;
}

void shm_task_startup(shm_task_mng_t *p_shm_task_mng, shmem_obj_t *p_shmem, int my_program_id)
{
	task_arg_t task_arg;
	shmem_message_t *p_shm_msg;
	msg_buf_t *msg_buf;
	shm_task_args_t shm_task_args;

	if((p_shm_task_mng == NULL) || (p_shmem == NULL)){
		return;
	}
	
	p_shm_msg = shmem_msg_init(p_shmem, my_program_id);

	shm_task_args.p_shm_msg = p_shm_msg;
	
	shm_task_args.shm_task = p_shm_task_mng->shm_task;

	task_arg.arg_size = sizeof(shm_task_args_t);
	task_arg.mask_bit = -1;
	task_arg.arg = &shm_task_args;

	p_shm_task_mng->run_flag = 1;
	
	while(p_shm_task_mng->run_flag){

	
		//msg_buf = create_msg_buf(p_shm_msg->shm_size - sizeof(shmem_obj_t));
		msg_buf = create_msg_buf(-1);
		
		shm_task_args.msg_buf = msg_buf;

		if(shmem_msg_rcv(p_shm_msg, msg_buf, 0) > 0){
			thread_pool_add_task(p_shm_task_mng->task_thread_pool, (proc_fun_t)shmem_msg_task_handler, &task_arg);
		}
		
		usleep(10000);
	}	

	shmem_msg_destroy(p_shm_msg);

}



