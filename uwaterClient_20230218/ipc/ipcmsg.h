#ifndef _IPC_MSG_H_
#define _IPC_MSG_H_

#include <sys/types.h>
#include <sys/msg.h>
#include <sys/ipc.h>

#include "shmem_msg.h"
#include "../utils/thread/thread_pool.h"



#define	IPCMSG_VERSION	17


#define	IPC_SERVER_SIDE		1
#define	IPC_CLIENT_SIDE		2
#define	IPC_COMMON_SIDE		3


#define IPCMSG_REQUEST		0
#define IPCMSG_RESPONSE		1
#define IPCMSG_GET_DATA		0
#define IPCMSG_GIVE_DATA	1
#define IPCMSG_RETURN_DATA	0
#define IPCMSG_FEEDBACK		1



typedef struct _data_item_info_st data_item_t;

struct _data_item_info_st{
	char info_type;			//0 request, 1 response
	char request_type;		//0 get data, 1 give data
	char needFeedback;		//0 no feedback, 1 need feedback
	char response_type;		//0 return data , 1 action feedback
	char do_flag;			//user-defined action type;
	int data_type;
	int data_len;
	void *p_data;
	data_item_t *next;
};



typedef struct _data_catalog_s data_catalog_t;
struct _data_catalog_s{
	int type;
	int offset;
	int len;
	data_catalog_t *next;
};


typedef struct {
	char *cond_head;
	char *cur_cond_gp;
	char *cur_cond;
}request_condition_t;


typedef struct {
	int channel_qty;
	int ipc_type;
	shmem_message_t *rcv_shm;
	shmem_message_t *send_shm;
}ipc_channel_t;

typedef struct _data_handler_info_st data_handler_info_t;

typedef struct {
	char ipc_side_type;
	char shm_target_do_flag;	//0 临时使用 ，1 永久使用

	char need_feedback;

	int target_program_id;
	
	int data_offset;
	int catalog_count;
	data_catalog_t *catalog_items;
	
	request_condition_t req_cond;
	
	data_handler_info_t *data_handlers;
	ipc_channel_t *ipc_channel;
	
	int msgid;
	int msglen;
	int	program_id;
	int	process_id;
	void *send_buf;
	void *rcv_buf;
}ipc_msg_t;


typedef void (*data_handler)(ipc_msg_t *, data_item_t *);

struct _data_handler_info_st{
	int data_type;
	data_handler handler;
	data_handler_info_t *next;
};


typedef struct {
	data_handler_info_t *data_handlers;
	thread_pool_t *task_thread_pool;
	void (*ipc_task)(ipc_msg_t *);
	void (*call_back)(void);
}ipc_task_mng_t;



void register_data_handler(data_handler_info_t **pp_data_handlers, int data_type, data_handler handler);

void free_all_data_handler(data_handler_info_t *data_handlers);


ipc_channel_t *init_ipc_channel(int my_program_id, u_int rcv_ipckey, u_int send_ipckey, int bufsize, int ipc_type);
void free_ipc_channel(ipc_channel_t *p_ipc_channel);


ipc_msg_t *init_ipc_msg(int ipc_side_type, ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers);
void free_ipc_msg(ipc_msg_t *p_ipcmsg);

int assign_ipc_msg_databuf(ipc_msg_t *p_ipcmsg, void *rcv_buf, void *send_buf);

int set_ipc_shm_target_temporarily(ipc_msg_t *p_ipcmsg, int program_id, int process_id);
int set_ipc_shm_target(ipc_msg_t *p_ipcmsg, int program_id);

int rcv_ipc_msg(ipc_msg_t *p_ipcmsg, int wait_ms);

int fill_msg_data(ipc_msg_t *p_ipcmsg, data_item_t *p_data_item);
int attach_msg_data(ipc_msg_t *p_ipcmsg, void *p_data, int len);
int send_ipc_msg(ipc_msg_t *p_ipcmsg, int msgid, int need_feedback);


int process_ipcmsg_received(ipc_msg_t *p_ipcmsg);

data_item_t *get_ipcmsg_rx_dataitem_list(ipc_msg_t *p_ipcmsg);


data_item_t *create_data_item(void);
void free_data_item(data_item_t *p_data_item);
void free_all_data_item(data_item_t *data_items);

int modify_last_data_item_type(ipc_msg_t *p_ipcmsg, data_item_t *p_data_item);


ipc_task_mng_t *init_ipc_task_mng(int thread_qty, void (*ipc_task)(ipc_msg_t *), data_handler_info_t *data_handlers);
void free_ipc_task_mng(ipc_task_mng_t *p_ipc_task_mng);
void ipc_task_startup(ipc_task_mng_t *p_ipc_task_mng, ipc_channel_t *p_ipc_channel);
void startup_ipcmsg_server(ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers, int thread_qty, void (*call_back)(void));

ipc_msg_t *init_ipcmsg_client(ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers, int target_program_id);
void free_ipcmsg_client(ipc_msg_t *p_ipcmsg);



#endif



