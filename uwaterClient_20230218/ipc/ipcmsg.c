
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdarg.h>

#include <unistd.h>


#include <errno.h>


#include "ipcmsg.h"


#if IPCMSG_ENABLE_CRC
static u_int CRC_Byte(u_int crc, u_char ch)
{
	u_char bb3;
	u_char i,temp,temp2;

	bb3 = 0;
	for (i = 0 ;i < 8; i++) {
		temp = (crc >> 24) & 0x000000ff;
		temp2 = ch ^ temp;

		ch = ch << 1;
		if (bb3) ch ++;

		if ((crc & 0x80000000)) bb3 = 1;
			else bb3 = 0;
		crc = crc << 1;
		if ((temp2 & 0x80)) crc ++;

		if ((temp2 & 0x80)) crc = crc ^ 0x04c11db6;
	}
	return crc;
}


static u_int generate_CRC(char *point, int length)
{
	u_int crc = 0xffffffff;
	u_int ii;

	for (ii = 0; ii < length; ii++)
		crc = CRC_Byte(crc,point[ii]);

	return crc;
}
#endif

ipc_channel_t *init_ipc_channel(int my_program_id, u_int rcv_ipckey, u_int send_ipckey, int bufsize, int ipc_type)
{
	ipc_channel_t *p_ipc_channel;
	shmem_obj_t *p_shmem;


	p_ipc_channel = malloc(sizeof(ipc_channel_t));
	if(p_ipc_channel == NULL){
		Info_Printf("malloc	ipc_channel_t is Failure, exit\n");
		exit(1);
	}

	p_ipc_channel->ipc_type = ipc_type;
		
	p_ipc_channel->rcv_shm = shmem_msg_init(shmem_init(rcv_ipckey, bufsize, ipc_type), my_program_id);
	
	//if(strcmp(rcv_shm_name, send_shm_name) == 0){
	if(strcmp((const char *)rcv_ipckey, (const char *)send_ipckey) == 0){
		p_ipc_channel->channel_qty = 1;
		p_ipc_channel->send_shm = p_ipc_channel->rcv_shm;
		
		Info_Printf("success, single channel, ipckey = %d, bufsize = %d\n"
			, rcv_ipckey, bufsize);
 	}else{
	 	p_ipc_channel->channel_qty = 2;
		p_ipc_channel->send_shm = shmem_msg_init(shmem_init(send_ipckey, bufsize, ipc_type), my_program_id);
		
		Info_Printf("success, double channel, rcv ipckey = %d, send ipckey = %d, bufsize = %d\n"
			, rcv_ipckey, send_ipckey, bufsize);
	}

	return p_ipc_channel;
}

void free_ipc_channel(ipc_channel_t *p_ipc_channel)
{

	shmem_destroy(p_ipc_channel->rcv_shm->p_shmem);
	shmem_msg_destroy(p_ipc_channel->rcv_shm);
		
	if(p_ipc_channel->channel_qty == 2){
		shmem_destroy(p_ipc_channel->send_shm->p_shmem);
		shmem_msg_destroy(p_ipc_channel->send_shm);
	}

	free(p_ipc_channel);
}

ipc_task_mng_t *init_ipc_task_mng(int thread_qty, void (*ipc_task)(ipc_msg_t *), data_handler_info_t *data_handlers)
{
	ipc_task_mng_t *p_ipc_task_mng;

	p_ipc_task_mng = malloc(sizeof(ipc_task_mng_t));
	if(p_ipc_task_mng == NULL){
		Info_Printf("malloc ipc_task_mng_t is Failure, exit\n");
		exit(1);
	}
	
	p_ipc_task_mng->task_thread_pool = create_thread_pool(thread_qty, 0x200000);
	p_ipc_task_mng->ipc_task = ipc_task;
	p_ipc_task_mng->call_back = NULL;
	p_ipc_task_mng->data_handlers = data_handlers;

	return p_ipc_task_mng;
}

void free_ipc_task_mng(ipc_task_mng_t *p_ipc_task_mng)
{
	if(p_ipc_task_mng == NULL){
		return;
	}

	free_all_data_handler(p_ipc_task_mng->data_handlers);

	thread_pool_destroy(p_ipc_task_mng->task_thread_pool);

	free(p_ipc_task_mng);
}


ipc_msg_t *init_ipc_msg(int ipc_side_type, ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers)
{
	
	
	ipc_msg_t *p_ipcmsg;

	if(p_ipc_channel == NULL){
		NULL;
	}
	
	p_ipcmsg = malloc(sizeof(ipc_msg_t));
	if(p_ipcmsg == NULL){
		Info_Printf("malloc ipc_msg_t is Failure, exit\n");
		exit(1);
	}

	p_ipcmsg->shm_target_do_flag = 1;
	p_ipcmsg->ipc_side_type = ipc_side_type;

	p_ipcmsg->ipc_channel = p_ipc_channel;


	#if 0
	p_ipcmsg->send_buf = malloc(bufsize);
	if(p_ipcmsg->send_buf == NULL){
		printf("malloc ipc_msg send_buf is Failure, exit\n");
		exit(1);
	}
	p_ipcmsg->rcv_buf = malloc(bufsize);
	if(p_ipcmsg->rcv_buf == NULL){
		printf("malloc ipc_msg rcv_buf is Failure, exit\n");
		exit(1);
	}

	
	#else
	p_ipcmsg->rcv_buf = NULL;
	p_ipcmsg->send_buf = NULL;
	#endif


	p_ipcmsg->need_feedback = 0;
	p_ipcmsg->msgid = 0;
	p_ipcmsg->msglen = 0;
	p_ipcmsg->process_id = -1;
	p_ipcmsg->program_id = -1;

	
	p_ipcmsg->catalog_count = 0;
	p_ipcmsg->data_offset = sizeof(int) * 2;
	
	p_ipcmsg->data_handlers = data_handlers;
	p_ipcmsg->catalog_items = NULL;

	p_ipcmsg->req_cond.cond_head = NULL;
	p_ipcmsg->req_cond.cur_cond_gp = NULL;
	p_ipcmsg->req_cond.cur_cond = NULL;
	
	
	Info_Printf(" success  !\n");

	return p_ipcmsg;
}


void free_ipc_msg(ipc_msg_t *p_ipcmsg)         //
{

	data_catalog_t *p_catalog_item, *next_catalog_item;

	if(p_ipcmsg == NULL){
		return;
	}
	
	Info_Printf(" ----------\n");
	
	
	p_catalog_item = p_ipcmsg->catalog_items;
	while(p_catalog_item){
		next_catalog_item = p_catalog_item->next;
		free(p_catalog_item);
		p_catalog_item = next_catalog_item;
	}
	Debug_Printf(" ------end----\n");


	free(p_ipcmsg);

}


int rcv_ipc_msg(ipc_msg_t *p_ipcmsg, int wait_ms)
{
	int msgid;
	int result;
	u_int crc32;
	msg_buf_t msg_buf;
	
	msg_buf.msgdata = p_ipcmsg->rcv_buf;
	
	result = shmem_msg_rcv(p_ipcmsg->ipc_channel->rcv_shm, &msg_buf, wait_ms);
	if(result>0){

		p_ipcmsg->process_id = msg_buf.process_id;
		p_ipcmsg->program_id = msg_buf.program_id;

		msgid = msg_buf.msgid;
		p_ipcmsg->msglen = msg_buf.msglen;

		p_ipcmsg->msgid = msgid >> 8;
		p_ipcmsg->need_feedback = msgid & 0xff;


		Info_Printf("need_feedback = %#x\n", p_ipcmsg->need_feedback);

		Info_Printf("msgid = %d\n", p_ipcmsg->msgid);
		Info_Printf("len = %d\n", p_ipcmsg->msglen);
		
		#if IPCMSG_ENABLE_CRC
		crc32 = generate_CRC(p_ipcmsg->rcv_buf, p_ipcmsg->msglen);
		Info_Printf("crc32 = %#x\n", crc32);
		#endif


		#if IPCMSG_DEBUG
		print_bytes(p_ipcmsg->rcv_buf, p_ipcmsg->msglen);
		#endif

		#if IPCMSG_ENABLE_CRC

		if(crc32 == 0){
			return msgid;
		}

		#else
		return msgid;
		#endif
		
	}

	return result;

}

int assign_ipc_msg_databuf(ipc_msg_t *p_ipcmsg, void *rcv_buf, void *send_buf)
{
	
	if(p_ipcmsg == NULL){
		return -1;
	}

	//printf("assign_ipc_msg_rcv_databuf \n");
	p_ipcmsg->rcv_buf = rcv_buf;
	p_ipcmsg->send_buf = send_buf;

	return 0;
}


int set_ipc_shm_target(ipc_msg_t *p_ipcmsg, int program_id)
{
	if(p_ipcmsg == NULL){
		return -1;
	}

	p_ipcmsg->target_program_id = program_id;

	return 0;
}

int set_ipc_shm_target_temporarily(ipc_msg_t *p_ipcmsg, int program_id, int process_id)
{
	if(p_ipcmsg == NULL){
		return -1;
	}

	p_ipcmsg->shm_target_do_flag = 0;

	p_ipcmsg->program_id = program_id;
	p_ipcmsg->process_id = process_id;

	return 0;
}

int send_ipc_msg(ipc_msg_t *p_ipcmsg, int msgid, int need_feedback)   
{
	int len;
	int status;
	u_int crc32;
	char *p_data;
	msg_buf_t msg_buf;
	
	data_catalog_t *p_catalog, *next_catalog;

	if(p_ipcmsg == NULL || p_ipcmsg->ipc_channel->send_shm == NULL){
		return -1;
	}


	if(msgid<=0){
		msgid = p_ipcmsg->msgid;
	}

	msgid <<= 8;
	
	if(need_feedback){
		msgid |= 1;
	}

	Info_Printf("catalog_count = %d, addr_offset = %d\n", p_ipcmsg->catalog_count, p_ipcmsg->data_offset);

	p_data = p_ipcmsg->send_buf;

	p_ipcmsg->data_offset += (4 - p_ipcmsg->data_offset%4) % 4;

	*((int *)(p_data)) = p_ipcmsg->catalog_count;
	*((int *)(p_data + sizeof(int))) = p_ipcmsg->data_offset;

	
	p_catalog = p_ipcmsg->catalog_items;
	len = sizeof(int) * 3;
	while(p_catalog){
		next_catalog = p_catalog->next;
		memcpy(p_data+p_ipcmsg->data_offset, p_catalog, len);
		p_ipcmsg->data_offset += len;
		free(p_catalog);
		p_catalog = next_catalog;
	}
	
	len = p_ipcmsg->data_offset;
	
	p_ipcmsg->catalog_items = NULL;
	p_ipcmsg->catalog_count = 0;
	p_ipcmsg->data_offset = sizeof(int) * 2;


	#if IPCMSG_ENABLE_CRC

	crc32 = generate_CRC(p_data, len);

	p_data[len++] = (crc32 >> 24) & 0xff;
	p_data[len++] = (crc32 >> 16) & 0xff;
	p_data[len++] = (crc32 >> 8) & 0xff;
	p_data[len++] = (crc32) & 0xff;

	#endif
	

	Info_Printf("msgid = %d, len = %d, crc32 = %#x\n", msgid, len, crc32);

	#if IPCMSG_DEBUG
	//print_bytes(p_data, len);
	#endif

	
	if(p_ipcmsg->shm_target_do_flag == 1){

		if(p_ipcmsg->ipc_side_type == IPC_CLIENT_SIDE){
			p_ipcmsg->program_id = p_ipcmsg->target_program_id;
			p_ipcmsg->process_id = 0;
		}
		
	}else if(p_ipcmsg->shm_target_do_flag == 0){
		p_ipcmsg->shm_target_do_flag = 1;
	}

	Info_Printf("program_id = %d, process_id = %d \n", p_ipcmsg->program_id, p_ipcmsg->process_id);

	msg_buf.msgdata = p_ipcmsg->send_buf;
	msg_buf.msgid = msgid;
	msg_buf.msglen = len;
	msg_buf.program_id = p_ipcmsg->program_id;
	msg_buf.process_id = p_ipcmsg->process_id;
	
	status = shmem_msg_send(p_ipcmsg->ipc_channel->send_shm, &msg_buf, SHM_WRITE_TIME_OUT_MS);
	set_shmem_msg_rcv_basetime(p_ipcmsg->ipc_channel->rcv_shm);
	
	if(status < 0){
		Info_Printf("shmsg_send failure, %d\n", status);
	}else{
		//printf("send %d\n", status);
	}

	return status;
}

void register_data_handler(data_handler_info_t **pp_data_handlers, int data_type, data_handler handler)
{
	data_handler_info_t *p_handler_info;

	if(handler == NULL){
		return;
	}

	p_handler_info = malloc(sizeof(data_handler_info_t));
	if(p_handler_info == NULL){
		Info_Printf("malloc data_handler_info failed, exit\n");
		exit(1);
	}

	p_handler_info->data_type = data_type;
	p_handler_info->handler = handler;

	p_handler_info->next = *pp_data_handlers;
	*pp_data_handlers = p_handler_info;
}

void free_all_data_handler(data_handler_info_t *data_handlers)
{
	data_handler_info_t *p_data_handler, *next_data_handler;

	Debug_Printf("start !\n");

	p_data_handler = data_handlers;
	while(p_data_handler){
		next_data_handler = p_data_handler->next;
		free(p_data_handler);
		p_data_handler = next_data_handler;
	}
	Debug_Printf("finished !\n");

}


data_catalog_t *create_data_catalog(void)
{
	data_catalog_t *p_catalog;

	p_catalog = malloc(sizeof(data_catalog_t));
	if(p_catalog == NULL){
		Info_Printf("malloc data_catalog failed, exit\n");
		exit(1);
	}

	p_catalog->len = 0;
	p_catalog->offset = 0;
	p_catalog->type = 0;
	p_catalog->next = NULL;

	return p_catalog;
}


data_item_t *create_data_item(void)
{
	data_item_t *p_data_item;

	p_data_item = malloc(sizeof(data_item_t));
	if(p_data_item == NULL){
		Info_Printf("malloc data_item_info failed, exit\n");
		exit(1);
	}

	memset(p_data_item, 0, sizeof(data_item_t));

	return p_data_item;
}

void free_data_item(data_item_t *p_data_item)
{
	//free(p_data_item->p_data);
	free(p_data_item);
}

void free_all_data_item(data_item_t *data_items)
{
	data_item_t *cur_item, *next_item;

	cur_item = data_items;

	while(cur_item){
		next_item = cur_item->next;
		free_data_item(cur_item);
		cur_item = next_item;
	}
}

int modify_last_data_item_type(ipc_msg_t *p_ipcmsg, data_item_t *p_data_item)
{
	data_catalog_t *p_catalog;

	char do_flag;
	
	if(p_ipcmsg == NULL || p_ipcmsg->catalog_items == NULL || p_data_item == NULL){
		return -1;
	}

	do_flag = (p_data_item->info_type) & 1;
	do_flag |= (p_data_item->request_type & 1) << 1;
	do_flag |= (p_data_item->needFeedback & 1) << 2;
	do_flag |= (p_data_item->response_type & 1) << 3;
	do_flag |= (p_data_item->do_flag & 0xf) << 4;

	Info_Printf("data_type = %#x, do_flag = %#x\n", p_data_item->data_type, do_flag);
	
	p_catalog = p_ipcmsg->catalog_items;

	p_catalog->type = (p_data_item->data_type<<8) | do_flag;

	return 	p_catalog->type;
}

int fill_msg_data(ipc_msg_t *p_ipcmsg, data_item_t *p_data_item)
{
	data_catalog_t *p_catalog;

	char do_flag;
	
	if(p_ipcmsg == NULL || p_data_item == NULL){
		return -1;
	}

	if(p_ipcmsg->ipc_side_type == IPC_SERVER_SIDE){
		p_data_item->info_type = IPCMSG_RESPONSE;
	}else if(p_ipcmsg->ipc_side_type == IPC_CLIENT_SIDE){
		p_data_item->info_type = IPCMSG_REQUEST;
	}

	do_flag = (p_data_item->info_type) & 1;
	do_flag |= (p_data_item->request_type & 1) << 1;
	do_flag |= (p_data_item->needFeedback & 1) << 2;
	do_flag |= (p_data_item->response_type & 1) << 3;
	do_flag |= (p_data_item->do_flag & 0xf) << 4;

	Info_Printf("data_type = %#x, len = %d, do_flag = %#x\n", p_data_item->data_type, p_data_item->data_len, do_flag);

	p_catalog = create_data_catalog();

	p_ipcmsg->data_offset += (4 - p_ipcmsg->data_offset%4) % 4;
	
	p_catalog->type = (p_data_item->data_type<<8) | do_flag;
	p_catalog->offset = p_ipcmsg->data_offset;
	p_catalog->len = p_data_item->data_len;
	
	if(p_data_item->p_data && p_data_item->data_len>0){
		memcpy(p_ipcmsg->send_buf+p_catalog->offset, p_data_item->p_data, p_data_item->data_len);
	}

	p_ipcmsg->data_offset += p_data_item->data_len;
	
	p_catalog->next = p_ipcmsg->catalog_items;
	p_ipcmsg->catalog_items = p_catalog;
	
	p_ipcmsg->catalog_count++;

	#if IPCMSG_DEBUG
	print_bytes(p_data_item->p_data, p_data_item->data_len);
	#endif

	Info_Printf("catalog_count = %d, data_offset = %d\n",  p_ipcmsg->catalog_count, p_ipcmsg->data_offset);
	
	return p_ipcmsg->catalog_count;
}

int attach_msg_data(ipc_msg_t *p_ipcmsg, void *p_data, int len)
{
	
	if(p_ipcmsg == NULL){
		return -1;
	}
	

	#if IPCMSG_DEBUG
	Debug_Printf("offset = %#x, len = %d \n", p_ipcmsg->data_offset, len);
	print_bytes(p_data, len);
	#endif

	memcpy(p_ipcmsg->send_buf+p_ipcmsg->data_offset, p_data, len);

	p_ipcmsg->data_offset += len;
	if(p_ipcmsg->catalog_items){
		p_ipcmsg->catalog_items->len += len;
	}

	return p_ipcmsg->data_offset;
}

data_handler find_ipcmsg_data_handler(ipc_msg_t *p_ipcmsg, int data_type)
{
	data_handler_info_t *p_handler_info;

	if(p_ipcmsg == NULL){
		return NULL;
	}

	p_handler_info = p_ipcmsg->data_handlers;
	while(p_handler_info){
		if(p_handler_info->data_type == data_type){
			return p_handler_info->handler;
		}
		p_handler_info = p_handler_info->next;
	}

	return NULL;
}

void catalog_to_dataitem(char *msg_data, data_catalog_t *p_catalog, data_item_t *p_data_item)
{
	if(p_catalog == NULL || p_data_item == NULL){
		return;
	}

	p_data_item->data_type = (p_catalog->type >> 8) & 0xffffff;
	p_data_item->data_len = p_catalog->len;
	p_data_item->info_type = p_catalog->type & 1;
	p_data_item->request_type = (p_catalog->type>>1) & 1;
	p_data_item->needFeedback = (p_catalog->type>>2) & 1;
	p_data_item->response_type = (p_catalog->type>>3) & 1;
	p_data_item->do_flag = (p_catalog->type>>4) & 0xf;

	#if 1
	p_data_item->p_data = msg_data + p_catalog->offset;
	#else
	p_data_item->p_data = malloc(p_data_item->data_len);
	if(p_data_item->p_data == NULL){
		printf("malloc data_item data buf failed , exit \n");
		exit(1);
	}
	memcpy(p_data_item->p_data, p_data + p_catalog->offset, p_data_item->data_len);
	#endif
}

int process_ipcmsg_received(ipc_msg_t *p_ipcmsg)
{
	char *p_data;

	
	int i, do_count;
	int catalog_count, addr_offset;
	data_catalog_t *p_catalog;
	data_item_t data_item;
	data_handler handler;

	if(p_ipcmsg == NULL){
		return -1;
	}


	p_data = p_ipcmsg->rcv_buf;

	catalog_count = *(int *)p_data;
	addr_offset = *(int *)(p_data + sizeof(int));

	Info_Printf("catalog_count = %d, addr_offset = %d\n"
		, catalog_count, addr_offset);
	
	do_count = 0;

	for(i=catalog_count-1; i>=0; i--){
		p_catalog = (data_catalog_t *)(p_data + addr_offset + sizeof(int)*3*i);

		catalog_to_dataitem(p_data, p_catalog, &data_item);

		Info_Printf("data_type = %#x, len = %d, do_flag = %#x\n"
			, data_item.data_type, data_item.data_len, p_catalog->type & 0xff);


		handler = find_ipcmsg_data_handler(p_ipcmsg, data_item.data_type);

		if(handler){
			do_count++;
			handler(p_ipcmsg, &data_item);
		}
	}

	return do_count;
}

data_item_t *get_ipcmsg_rx_dataitem_list(ipc_msg_t *p_ipcmsg)
{
	char *p_data;

	int i, catalog_count, addr_offset;
	data_catalog_t *p_catalog;
	data_item_t *p_data_item, *head_data_item = NULL;

	if(p_ipcmsg == NULL){
		return NULL;
	}
	
	p_data = p_ipcmsg->rcv_buf;

	catalog_count = *(int *)p_data;
	addr_offset = *(int *)(p_data + sizeof(int));

	Info_Printf(" catalog_count = %d, addr_offset = %d\n"
		, catalog_count, addr_offset);


	for(i=0; i<catalog_count; i++){
		
		p_catalog = (data_catalog_t *)(p_data + addr_offset + sizeof(int)*3*i);

		p_data_item = create_data_item();

		p_data_item->next = head_data_item;
		head_data_item = p_data_item;

		catalog_to_dataitem(p_data, p_catalog, p_data_item);


		Info_Printf("data_type = %#x, len = %d, do_flag = %#x\n"
			, p_data_item->data_type, p_data_item->data_len, p_catalog->type & 0xff);

		
	}

	return head_data_item;
}

typedef struct {
	ipc_msg_t *p_ipcmsg;
	void (*ipc_task)(ipc_msg_t *);
	void (*call_back)(void);
}ipc_msg_task_args_t;

void register_ipcmsg_server_callback(void (*call_back)(void));

void ipc_msg_task_handler(ipc_msg_task_args_t *ipc_task_args)
{
	
	if(ipc_task_args == NULL){
		return;
	}

	ipc_task_args->ipc_task(ipc_task_args->p_ipcmsg);
	if(ipc_task_args->call_back){
		ipc_task_args->call_back();
	}

	free(ipc_task_args->p_ipcmsg->send_buf);
	free(ipc_task_args->p_ipcmsg->rcv_buf);

	free_ipc_msg(ipc_task_args->p_ipcmsg);

}


void ipc_task_startup(ipc_task_mng_t *p_ipc_task_mng, ipc_channel_t *p_ipc_channel)
{

	task_arg_t task_arg;
	ipc_msg_t *p_ipcmsg;

	ipc_msg_task_args_t ipc_task_args;

	if(p_ipc_task_mng == NULL){
		return;
	}

	
	ipc_task_args.ipc_task = p_ipc_task_mng->ipc_task;
	ipc_task_args.call_back = p_ipc_task_mng->call_back;

	task_arg.arg_size = sizeof(ipc_msg_task_args_t);
	task_arg.mask_bit = -1;
	task_arg.arg = &ipc_task_args;
	
	while(1){

		p_ipcmsg = init_ipc_msg(IPC_SERVER_SIDE, p_ipc_channel, p_ipc_task_mng->data_handlers);
		

		p_ipcmsg->rcv_buf = malloc(p_ipc_channel->rcv_shm->shm_size);
		if(p_ipcmsg->rcv_buf == NULL){
			printf("malloc ipc_msg rcv_buf is Failure, exit\n");
			exit(1);
		}

		p_ipcmsg->send_buf = malloc(p_ipc_channel->send_shm->shm_size);
		if(p_ipcmsg->send_buf == NULL){
			printf("malloc ipc_msg send_buf is Failure, exit\n");
			exit(1);
		}
		
		ipc_task_args.p_ipcmsg = p_ipcmsg;


		if(rcv_ipc_msg(p_ipcmsg, 0) > 0){
			thread_pool_add_task(p_ipc_task_mng->task_thread_pool, (proc_fun_t)ipc_msg_task_handler, &task_arg);
		}
		
		usleep(10000);
    }
}


static void ipcmsg_task_handler(ipc_msg_t *p_ipcmsg)
{


	if(p_ipcmsg == NULL){
		return;
	}

	process_ipcmsg_received(p_ipcmsg);

	if(p_ipcmsg->need_feedback){
		send_ipc_msg(p_ipcmsg, 1, 0);
	}

}


void startup_ipcmsg_server(ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers, int thread_qty, void (*call_back)(void))
{

	ipc_task_mng_t *p_ipc_task_mng;
	
	p_ipc_task_mng = init_ipc_task_mng(thread_qty, ipcmsg_task_handler, data_handlers);
	p_ipc_task_mng->call_back = call_back;

	ipc_task_startup(p_ipc_task_mng, p_ipc_channel);

	free_ipc_task_mng(p_ipc_task_mng);
		
}

ipc_msg_t *init_ipcmsg_client(ipc_channel_t *p_ipc_channel, data_handler_info_t *data_handlers, int target_program_id)
{

	ipc_msg_t *p_ipcmsg;


	p_ipcmsg = init_ipc_msg(IPC_CLIENT_SIDE, p_ipc_channel, data_handlers);
	set_ipc_shm_target(p_ipcmsg, target_program_id);

	p_ipcmsg->rcv_buf = malloc(p_ipc_channel->rcv_shm->shm_size);
	if(p_ipcmsg->rcv_buf == NULL){
		printf("malloc ipc_msg rcv_buf is Failure, exit\n");
		exit(1);
	}

	p_ipcmsg->send_buf = malloc(p_ipc_channel->send_shm->shm_size);
	if(p_ipcmsg->send_buf == NULL){
		printf("malloc ipc_msg send_buf is Failure, exit\n");
		exit(1);
	}

	return p_ipcmsg;
}

void free_ipcmsg_client(ipc_msg_t *p_ipcmsg)
{
	free(p_ipcmsg->rcv_buf);
	free(p_ipcmsg->send_buf);

	free_all_data_handler(p_ipcmsg->data_handlers);
	free_ipc_msg(p_ipcmsg);
}


