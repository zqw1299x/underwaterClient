

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "shmem_obj.h"



int shmem_cond_init(shmem_cond_t *cond)
{
	if(cond == NULL){
		return -1;
	}

	cond->timestamp.tv_sec = 0;
	cond->timestamp.tv_usec = 0;


	cond->semaphore = 0;
	cond->signal_type = 0;

	return 0;
}

int shmem_cond_signal(shmem_cond_t *cond)
{
	if(cond == NULL){
		return -1;
	}
	

	cond->signal_type = 1;
	cond->semaphore = 1;

	/*
	if(gettimeofday(&cond->timestamp, NULL) != 0){
		return -1;
	}
	//*/

	Debug_Printf("signal_type = %d, semaphore = %d  \n", cond->signal_type, cond->semaphore);

	return 0;
}

int shmem_cond_broadcast(shmem_cond_t *cond)
{
	if(cond == NULL){
		return -1;
	}
	

	cond->signal_type = 0;

	if(gettimeofday(&cond->timestamp, NULL) != 0){
		return -1;
	}

	Debug_Printf("signal_type = %d , tv_sec = %d, tv_usec = %d\n"
		, cond->signal_type, cond->timestamp.tv_sec, cond->timestamp.tv_usec);


	return 0;
}

int shmem_lock_init(shmem_rwlock_t *rw_lock)
{

	shmem_cond_init(&(rw_lock->cond_reader));
	shmem_cond_init(&(rw_lock->cond_writer));

	rw_lock->rw_count = 0;

	return 0;
}

int shmem_rw_lock(shmem_obj_t *p_shmem, int rw_flag)
{
	
	if(rw_flag == SHM_READ_FLAG) {
		Debug_Printf("read lock  --  \n");
		
		while(p_shmem->rw_lock.rw_count < 0){

			usleep(1000);

		}
		p_shmem->rw_lock.rw_count++;


		Debug_Printf("read lock,  rw_count  %d \n", p_shmem->rw_lock.rw_count);
	}else if(rw_flag == SHM_WRITE_FLAG){
	

		Debug_Printf("write lock  -- \n");

		
		while(p_shmem->rw_lock.rw_count != 0){

			usleep(1000);

		}
		
		p_shmem->rw_lock.rw_count = -1;
		
		Debug_Printf("write lock,  rw_count  %d \n", p_shmem->rw_lock.rw_count);

	}
	
	return 0;
}

int shmem_rw_unlock(shmem_obj_t *p_shmem, int rw_flag)
{

	if(rw_flag == SHM_READ_FLAG){

		
		Debug_Printf("read unlock  ==  \n");


		if(p_shmem->rw_lock.rw_count > 0){
			p_shmem->rw_lock.rw_count--;
		}

		
		Debug_Printf("read unlock,  rw_count  %d \n", p_shmem->rw_lock.rw_count);
		
	}else if(rw_flag == SHM_WRITE_FLAG){
			
		Debug_Printf(" write unlock  ==  \n");
		
		if(p_shmem->rw_lock.rw_count < 0){
			p_shmem->rw_lock.rw_count = 0;
		}

		Debug_Printf("write unlock,  rw_count  %d \n", p_shmem->rw_lock.rw_count);

	}

	
	return 0;
}

void shmem_set_name(shmem_obj_t *p_shmem, char *name)
{
	strcpy(p_shmem->shm_name, name);
}


shmem_obj_t *posix_shmem_init(char *shm_name, int shm_size);
int posix_shmem_destroy(shmem_obj_t *p_shmem);

shmem_obj_t *systemv_shmem_init(u_int ipc_key, int shm_size);
int systemv_shmem_destroy(shmem_obj_t *p_shmem);


shmem_obj_t *shmem_init(u_int ipc_key, int shm_size, int shm_if)
{
	shmem_obj_t *p_shmem = NULL;
	
	if((shm_if)==SHM_POSIX_IF) 
		p_shmem =  posix_shmem_init((char *)ipc_key, shm_size); 
	else if((shm_if)==SHM_SYSTEMV_IF) 
		p_shmem =  systemv_shmem_init(ipc_key, shm_size); 
	else
		return NULL;

	p_shmem->shm_if = shm_if & 0xff;

	return p_shmem;
}
int shmem_destroy(shmem_obj_t *p_shmem)
{
	char shm_if = p_shmem->shm_if;
		
	if((shm_if)==SHM_POSIX_IF) 
		return posix_shmem_destroy(p_shmem); 
	else if((shm_if)==SHM_SYSTEMV_IF) 
		return systemv_shmem_destroy(p_shmem); 
	else 
		return 0;
}


const char *shmem_if_type_name[] = {
	"none",
	"posix",
	"system V"
};

void shmem_show_head(shmem_obj_t *p_shmem)
{
	printf("%s [%d] share memery : %s size %#x bytes , reference count %d\n", shmem_if_type_name[p_shmem->shm_if]
				, p_shmem->shmid, p_shmem->shm_name, p_shmem->shm_size, p_shmem->ref_count);
	printf("share memery message : from program id %d, process id %d \n", p_shmem->ref_from.program_id, p_shmem->ref_from.process_id);
	printf("share memery message : to program id %d, process id %d \n", p_shmem->ref_to.program_id, p_shmem->ref_to.process_id);
	printf("share memery message : read condition : semaphore %d, timestamp %d.%.6d s \n", p_shmem->rw_lock.cond_reader.semaphore
				, p_shmem->rw_lock.cond_reader.timestamp.tv_sec, p_shmem->rw_lock.cond_reader.timestamp.tv_usec);
	printf("share memery message : write condition : semaphore %d, timestamp %d.%.6d s \n", p_shmem->rw_lock.cond_writer.semaphore
				, p_shmem->rw_lock.cond_writer.timestamp.tv_sec, p_shmem->rw_lock.cond_writer.timestamp.tv_usec);
	printf("share memery message : data length %d, %#.x %#.x %#.x %#.x \n", p_shmem->data_len, p_shmem->data_buf[0]
				, p_shmem->data_buf[1], p_shmem->data_buf[2], p_shmem->data_buf[3]);
}

