
#include <stdio.h>
#include <stdlib.h>

#include "shmmsg_client.h"




#define MYSELF_SHMEM_PROGRAM_ID 	284
#define OPPOSITE_SHMEM_PROGRAM_ID	220
#define MSG_BUF_SIZE		0x800000

//char shmclient_databuf[MSG_BUF_SIZE];

char *get_shm_rcvdata(shmclient_st *shm_client)
{
	return shm_client->msg_buf.msgdata;
}

int send_shmmsg(shmclient_st *shm_client, int msgid, char *data, int len)
{
	shm_client->msg_buf.msgid = msgid;
		
	shm_client->msg_buf.msglen = len;
	shm_client->msg_buf.msgdata = data;

	shm_client->msg_buf.process_id = 0;

	return shmem_msg_send(shm_client->shm_msg, &shm_client->msg_buf, SHM_WRITE_TIME_OUT_MS);
}

int rcv_shmmsg(shmclient_st *shm_client, char *data)
{
	int ret;

	//if(data){
		shm_client->msg_buf.msgdata = data;
	//}else{
	//	shm_client->msg_buf.msgdata = shmclient_databuf;
	//}

	ret = shmem_msg_rcv(shm_client->shm_msg, &shm_client->msg_buf, 20000);
	if(ret > 0){
		/*
		printf("shm msg rcv : msgid = %d \n", shm_client->msg_buf.msgid);
		printf("shm msg rcv : program_id = %d \n", shm_client->msg_buf.program_id);
		printf("shm msg rcv : process_id = %d \n", shm_client->msg_buf.process_id);
		print_bytes(shm_client->msg_buf.msgdata, shm_client->msg_buf.msglen);
		//*/
		return shm_client->msg_buf.msglen;
	}else{
		printf("shm msg rcv : failed \n");
		return 0;
	}
}


shmclient_st *create_shmclient(u_int ipc_key, int shm_size, int my_program_id, int opp_program_id)
{

	shmem_obj_t *p_shmem;
	shmclient_st *shm_client;


	shm_client = malloc(sizeof(shmclient_st));
	if(shm_client == NULL){
		printf("shm_client malloc failed, exit");
		exit(1);
	}

	//char *shm_name = "my_shm";
	p_shmem = shmem_init(ipc_key, shm_size, SHM_SYSTEMV_IF);
	shm_client->shm_msg = shmem_msg_init(p_shmem, my_program_id);

		
	init_msg_buf(&shm_client->msg_buf, NULL);
	
	shm_client->msg_buf.program_id = opp_program_id;
	shm_client->msg_buf.process_id = 0;

	shm_client->msg_buf.buf_size = -1;		// buf_size < 0, 假若msgdata == NULL, 则自动分配接收的数据空间

	return shm_client;
}


void free_shmclient(shmclient_st *shm_client)
{
	shmem_obj_t *p_shmem = shm_client->shm_msg->p_shmem;
	
	shmem_msg_destroy(shm_client->shm_msg);
	shmem_destroy(p_shmem);

	free(shm_client);
}


