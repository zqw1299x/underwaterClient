
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


#include "shmem_obj.h"



shmem_obj_t *posix_shmem_init(char *shm_name, int shm_size)
{
	int exist_flag = 0;
	int fd_shm;
	struct stat shmem_stat;
	shmem_obj_t *p_shmem;
	
	if(shm_size <= 0){
		return NULL;
	}

	if(strlen(shm_name) > SHM_NAME_LEN){
		printf("shmem_init : %s name length is top %d \n ", shm_name, SHM_NAME_LEN);
	}

	fd_shm = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
	if(fd_shm<0){
		if(errno == EEXIST){
			exist_flag = 1;
			fd_shm = shm_open(shm_name, O_RDWR, 0666);
			printf("shm_open %s  :  shared memory object already exist!\n", shm_name);
		}else{
			printf("shm_open %s  : [ %s ], exit!\n", shm_name, strerror(errno));
			exit(1);
		}
	}else{
		
		exist_flag = 0;
		ftruncate(fd_shm, shm_size);

	}


	
	if(fstat(fd_shm, &shmem_stat) < 0){
		printf("fstat %d, failed \n", fd_shm);
	}

	Info_Printf(" fstat , st_size = %d \n", (int)shmem_stat.st_size);

	p_shmem = mmap(NULL, shmem_stat.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, fd_shm, 0);
	if(p_shmem == MAP_FAILED){
		printf("mmap  shared memory failed, exit!\n");
		exit(1);
	}
	
	close(fd_shm);


	if(exist_flag == 0){

		p_shmem->data_len = 0;
		memset(p_shmem->data_buf, 0,shm_size-sizeof(shmem_obj_t));
		
		p_shmem->ref_count= 0;
		p_shmem->shm_size = shm_size;

		shmem_lock_init(&(p_shmem->rw_lock));
		shmem_cond_signal(&(p_shmem->rw_lock.cond_writer));	
	}

	p_shmem->ref_count++;

	p_shmem->shmid = fd_shm;
	strcpy(p_shmem->shm_name, shm_name);

	return p_shmem;
}



int posix_shmem_destroy(shmem_obj_t *p_shmem)
{
	int shm_size = p_shmem->shm_size;
	int ref_count;
	char shm_name[SHM_NAME_LEN];
	
	strcpy(shm_name, p_shmem->shm_name);

	Info_Printf("%s : shm_size = %d,   ref_count = %d \n", shm_name, shm_size, p_shmem->ref_count);
	
	p_shmem->ref_count--;
	ref_count = p_shmem->ref_count;
	munmap(p_shmem, shm_size);	
	
	if(ref_count <= 0){
		//shm_unlink(shm_name);
	}
	
	return 0;
}


