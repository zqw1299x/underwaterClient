
#ifndef _SYSTEM_V_SHMEM_H_
#define _SYSTEM_V_SHMEM_H_

#include <sys/types.h>
#include <sys/time.h>
#include <pthread.h>

#include "ipcdef.h"




#define	SHM_WAIT_TIMEOUT	-9

#define SHM_READ_FLAG	1
#define SHM_WRITE_FLAG	2
#define SHM_RDWR_FLAG	3
#define SHM_EXEC_FLAG	4

#define SHM_NAME_LEN	20

#define SHM_POSIX_IF	1
#define SHM_SYSTEMV_IF	2

typedef struct _shmem_ref_s{
	int	process_id;		//用以区分不同的进程，用pid标识
	int	program_id;		//用以区分不同的程序，要预先定义好
}shmem_ref_t;



typedef struct _shmem_cond_s{
	struct timeval timestamp;
	int semaphore;
	int signal_type;   // 1 single, 0 broadcast;
} shmem_cond_t;


typedef struct _shmem_rwlock_s{

	shmem_cond_t	cond_reader;
	shmem_cond_t	cond_writer;
	
	int rw_count;
}shmem_rwlock_t;

typedef struct _shmem_obj_s{
	char shm_name[SHM_NAME_LEN];
	int	shmid;
	int	shm_size;
	short	ref_count;
	char	shm_if;
	char	reserve;
	shmem_rwlock_t	rw_lock;
	shmem_ref_t	ref_from;
	shmem_ref_t	ref_to;
	int	data_len;
	char	data_buf[];
}shmem_obj_t;


shmem_obj_t *shmem_init(u_int ipc_key, int shm_size, int shm_if);
int shmem_destroy(shmem_obj_t *p_shmem);


void shmem_set_name(shmem_obj_t *p_shmem, char *name);



int shmem_cond_signal(shmem_cond_t *cond);
int shmem_cond_broadcast(shmem_cond_t *cond);

int shmem_lock_init(shmem_rwlock_t *rw_lock);

int shmem_rw_lock(shmem_obj_t *p_shmem, int rw_flag);
int shmem_rw_unlock(shmem_obj_t *p_shmem, int rw_flag);

void shmem_show_head(shmem_obj_t *p_shmem);


#endif
