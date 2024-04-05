
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "shmem_obj.h"


shmem_obj_t *systemv_shmem_init(u_int ipc_key, int shm_size)
{
	int exist_flag = 0;
	int shmid;
	//key_t ipc_key;
	
	shmem_obj_t *p_shmem;
	
	struct shmid_ds shm_ds;

	if(shm_size <= 0){
		return NULL;
	}

	/*
	if(strlen(shm_name)>SHM_NAME_LEN){
		printf("shmem_init : %s name length is top %d \n ", shm_name, SHM_NAME_LEN);
	}

	ipc_key = ftok(shm_name, 1);
	//*/
	shmid = shmget(ipc_key, shm_size, IPC_CREAT | IPC_EXCL | 0666);
	
	if(shmid<0){
		if(errno == EEXIST){
			exist_flag = 1;
			shmid = shmget(ipc_key, 0, 0666);
			Info_Printf("shmget [%d] : shared memory object already exist!\n",  ipc_key);
		}else{
			Info_Printf("shmget [%d] : [%s], exit!\n",  ipc_key, strerror(errno));
			exit(1);
		}
	}
	
	p_shmem = shmat(shmid, NULL, 0);
	if(p_shmem <= 0){
		Info_Printf("shmat : attaches %d shared memory address failed [%s], exit!\n", ipc_key, strerror(errno));
		exit(1);
	}


	Info_Printf(" ipc_key = %d, size = %d, attaches memory success ! \n",  ipc_key, shm_size);

	
	shmctl(shmid, IPC_STAT, &shm_ds);

	Info_Printf(" shmid = %d, shm_segsz = %d ,  shm_nattch = %d \n", shmid, (int)shm_ds.shm_segsz, (int)shm_ds.shm_nattch);


	if(exist_flag == 0){
		
		p_shmem->data_len = 0;
		memset(p_shmem->data_buf, 0,shm_size-sizeof(shmem_obj_t));
		
		p_shmem->ref_count= 0;
		p_shmem->shm_size = shm_size;

		shmem_lock_init(&(p_shmem->rw_lock));
		shmem_cond_signal(&(p_shmem->rw_lock.cond_writer));
		
	}

	p_shmem->ref_count++;

	p_shmem->shmid = shmid;
	//strcpy(p_shmem->shm_name, shm_name);

	return p_shmem;
}



int systemv_shmem_destroy(shmem_obj_t *p_shmem)
{
	int shmid = p_shmem->shmid;
	struct shmid_ds shm_ds;

	int shm_size = p_shmem->shm_size;
	//char shm_name[SHM_NAME_LEN];
	
	//strcpy(shm_name, p_shmem->shm_name);

	
	//char *shm_name = p_shmem->shm_name;

	//Info_Printf("%s : shm_size = %d,   ref_count = %d \n", shm_name, shm_size, p_shmem->ref_count);
	shmctl(shmid, IPC_STAT, &shm_ds);

	Info_Printf(": shmid = %d,  shm_size = %d,  shm_nattch = %d \n", shmid, shm_size, (int)shm_ds.shm_nattch);

	
	shmdt(p_shmem);
	
	if(shm_ds.shm_nattch == 1){
		shmctl(shmid, IPC_RMID, NULL);
	}

	return 0;
}


