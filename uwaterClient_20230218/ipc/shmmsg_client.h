#ifndef ___MY_SHM_MSG_CLIENT_H___
#define ___MY_SHM_MSG_CLIENT_H___

#include "shmem_msg.h"


typedef struct {
	shmem_message_t *shm_msg;
	msg_buf_t msg_buf;
} shmclient_st;

char *get_shm_rcvdata(shmclient_st *shm_client);

shmclient_st *create_shmclient(u_int ipc_key, int shm_size, int my_program_id, int opp_program_id);
void free_shmclient(shmclient_st *shm_client);

int send_shmmsg(shmclient_st *shm_client, int msgid, char *data, int len);
int rcv_shmmsg(shmclient_st *shm_client, char *data);

#endif
