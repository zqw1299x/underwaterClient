#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
//#include <linux/in.h>
#include <linux/if.h>

#include <linux/sockios.h>

#include "include/globa.h"
#include "ipc/shmem_obj.h"

uart_s uart;
shmem_obj_t *global_shm;

system_parameter_t *system_para;

encrypt_parameter_t *myencrypt;

connect_parameter_t *connect_para;

void open_global_shm(void)
{
	printf("start open_global_shm\n");
	char *shm_base;
	int i,error = 0;
	global_shm = shmem_init(GLOBAL_SHM_KEY, GLOBAL_SHM_SIZE, SHM_SYSTEMV_IF);

	shm_base = (char *)global_shm;

	system_para = (system_parameter_t *)shm_base;
	myencrypt = (encrypt_parameter_t *)shm_base+sizeof(system_parameter_t);
	connect_para = (connect_parameter_t *)shm_base+sizeof(system_parameter_t)+sizeof(encrypt_parameter_t);
	printf("finish open_global_shm\n");	
}

