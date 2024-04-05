
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ipcmsg_query_req.h"



int init_request_condition(ipc_msg_t *p_ipcmsg)
{
	int v_s32 = 0;
	
	if(p_ipcmsg == NULL){
		return -1;
	}
	
	p_ipcmsg->req_cond.cond_head = p_ipcmsg->send_buf + p_ipcmsg->data_offset;

	attach_msg_data(p_ipcmsg, &v_s32, 4);

	return 0;
}


int add_request_condition_group(ipc_msg_t *p_ipcmsg)
{
	int v_s32 = 0;
	
	if(p_ipcmsg == NULL){
		return -1;
	}

	*((int *)(p_ipcmsg->req_cond.cond_head)) += 1;

	p_ipcmsg->req_cond.cur_cond_gp = p_ipcmsg->send_buf + p_ipcmsg->data_offset;

	attach_msg_data(p_ipcmsg, &v_s32, 4);

	return *((int *)(p_ipcmsg->req_cond.cond_head));
}


int request_condition_addl(ipc_msg_t *p_ipcmsg, char level, char field, char cond_oper, ...)
{
	int v_s32 = 0;
	char v_s8 = 0, argc = 0;

	va_list ap;    

	
	if(p_ipcmsg == NULL){
		return -1;
	}


	//set_maskbit(p_ipcmsg->req_cond.cur_cond_gp, (1<<((level-1))*COND_FIELD_QTY) + field);
	*((int *)(p_ipcmsg->req_cond.cur_cond_gp)) += 1;

	p_ipcmsg->req_cond.cur_cond = p_ipcmsg->send_buf + p_ipcmsg->data_offset;

	v_s8 = level - 1;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = field;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = cond_oper;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = argc;
	attach_msg_data(p_ipcmsg, &v_s8, 1);


    va_start(ap, cond_oper);

	do{
		v_s32 = va_arg(ap, int);
		if(v_s32 == VA_ARG_END){
			break;
		}
		
		argc++;
		attach_msg_data(p_ipcmsg, &v_s32, 4);
		
		if(cond_oper == COND_OPER_RANGE){
			if(argc >= 2)break;
		}else if(cond_oper < COND_OPER_IN){
			break;
		}
		
	}while(v_s32 != VA_ARG_END);

	va_end(ap); 

	/*
	v_s32 = VA_ARG_END;
	attach_msg_data(p_ipcmsg, &v_s32, 4);
	argc++;
	//*/

	*(p_ipcmsg->req_cond.cur_cond + 3) = argc;

	return 0;
}

int request_condition_addv(ipc_msg_t *p_ipcmsg, char level, char field, char cond_oper, char argc, int *val_arr)
{
	int i;
	int v_s32 = 0;
	char v_s8 = 0;

	
	if(p_ipcmsg == NULL){
		return -1;
	}

	if(argc <= 0 || val_arr == NULL){
		return 0;
	}

	//set_maskbit(p_ipcmsg->req_cond.cur_cond_gp, (1<<((level-1))*COND_FIELD_QTY) + field);
	*((int *)(p_ipcmsg->req_cond.cur_cond_gp)) += 1;

	p_ipcmsg->req_cond.cur_cond = p_ipcmsg->send_buf + p_ipcmsg->data_offset;

	v_s8 = level;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = field;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = cond_oper;
	attach_msg_data(p_ipcmsg, &v_s8, 1);
	v_s8 = argc;
	attach_msg_data(p_ipcmsg, &v_s8, 1);

	for(i=0; i<argc; i++){
		attach_msg_data(p_ipcmsg, val_arr + i, 4);	
	}

	/*
	v_s32 = VA_ARG_END;
	attach_msg_data(p_ipcmsg, &v_s32, 4);
	//*/

	return 0;
}

void print_query_cond(query_cond_t *p_query_cond)
{
	int i;
	
	if(p_query_cond == NULL){
		return;
	}

	printf("query_cond : idx_argc = %d \n", p_query_cond->idx_argc);
	printf("query_cond : idx_oper = %d \n", p_query_cond->idx_oper);
	for(i=0; i<p_query_cond->idx_argc; i++){
		printf("query_cond : pv_idx[%d] = %d \n", i, p_query_cond->pv_idx[i]);
	}
	printf("query_cond : type_argc = %d \n", p_query_cond->type_argc);
	printf("query_cond : type_oper = %d \n", p_query_cond->type_oper);
	for(i=0; i<p_query_cond->type_argc; i++){
		printf("query_cond : pv_type[%d] = %d \n", i, p_query_cond->pv_type[i]);
	}
}

void print_idx_query_arg(idx_query_arg_t *p_idx_arg)
{
	if(p_idx_arg == NULL){
		return;
	}

	printf("idx_query_arg : start_idx = %d \n", p_idx_arg->start_idx);
	printf("idx_query_arg : end_idx = %d \n", p_idx_arg->end_idx);
	printf("idx_query_arg : query_times = %d \n", p_idx_arg->query_times);
}


int query_and_give_data(ipc_msg_t *p_ipcmsg, char *p_data, int data_len, int (*query_callback)(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond))
{
	int i, j , k;
	int offset = 0;
	int res_count = 0;
	int cond_gp_qty, cond_qty;
	
	query_cond_t query_cond[COND_LV_QTY];
	
	u_char cond_lv, cond_field, cond_oper, cond_argc;


	//print_bytes(p_data, data_len);

	if(p_ipcmsg == NULL || query_callback == NULL){
		return -1;
	}

	if(p_data == NULL || data_len <=0){
		return query_callback(p_ipcmsg, NULL);
	}


	memset(query_cond, 0, sizeof(query_cond));
	
	offset = 0;
	while(offset<data_len){
		
		memcpy(&cond_gp_qty, p_data + offset, 4);
		offset += 4;
		
		for(i=0; i<cond_gp_qty; i++){
			
			memcpy(&cond_qty, p_data + offset, 4);
			offset += 4;
			
			for(j=0; j<cond_qty; j++){
				
				cond_lv = p_data[offset++];
				cond_field = p_data[offset++];
				cond_oper = p_data[offset++];
				cond_argc = p_data[offset++];

				//printf(" parse query cond_lv = %d, cond_field = %d, cond_oper = %d, cond_argc = %d \n", cond_lv, cond_field, cond_oper, cond_argc);

				if(cond_field == COND_FIELD_IDX){
					query_cond[cond_lv].idx_argc = cond_argc;
					query_cond[cond_lv].idx_oper = cond_oper;
					query_cond[cond_lv].pv_idx = (int *)(p_data + offset);
				}else if(cond_field == COND_FIELD_TYPE){
					query_cond[cond_lv].type_argc = cond_argc;
					query_cond[cond_lv].type_oper = cond_oper;
					query_cond[cond_lv].pv_type = (int *)(p_data + offset);
				}
				
				offset += cond_argc * 4;

			}
	
			res_count = query_callback(p_ipcmsg, query_cond);
			
		}
	}

	if(cond_gp_qty <= 0){
		return query_callback(p_ipcmsg, NULL);
	}else{
		return res_count;
	}
}

query_cond_gp_t *parse_query_cond(ipc_msg_t *p_ipcmsg, char *p_data, int data_len)
{
	int i, j , k;
	int offset = 0;
	int res_count = 0;
	int cond_gp_qty, cond_qty;

	query_cond_gp_t *head_cond_gp = NULL, *cur_cond_gp, *p_cond_gp;
	
	query_cond_t query_cond[COND_LV_QTY];
	
	u_char cond_lv, cond_field, cond_oper, cond_argc;


	//print_bytes(p_data, data_len);

	if(p_ipcmsg == NULL || p_data == NULL || data_len <=0){
		return NULL;
	}


	memset(query_cond, 0, sizeof(query_cond));
	
	offset = 0;
	while(offset<data_len){
		
		memcpy(&cond_gp_qty, p_data + offset, 4);
		offset += 4;

		for(i=0; i<cond_gp_qty; i++){

			p_cond_gp = malloc(sizeof(query_cond_gp_t));
			if(p_cond_gp == NULL){
				Info_Printf("malloc query_cond_gp failed, exit !\n");
				exit(1);
			}
			memset(p_cond_gp, 0, sizeof(query_cond_gp_t));

			
			if(head_cond_gp == NULL){
				head_cond_gp = p_cond_gp;
			}else{
				cur_cond_gp->next = p_cond_gp;
			}
			
			cur_cond_gp = p_cond_gp;
				
			memcpy(&cond_qty, p_data + offset, 4);
			offset += 4;
			
			for(j=0; j<cond_qty; j++){
				
				cond_lv = p_data[offset++];
				cond_field = p_data[offset++];
				cond_oper = p_data[offset++];
				cond_argc = p_data[offset++];

				//printf(" parse query cond_lv = %d, cond_field = %d, cond_oper = %d, cond_argc = %d \n", cond_lv, cond_field, cond_oper, cond_argc);

				if(cond_field == COND_FIELD_IDX){
					p_cond_gp->query_cond[cond_lv].idx_argc = cond_argc;
					p_cond_gp->query_cond[cond_lv].idx_oper = cond_oper;
					p_cond_gp->query_cond[cond_lv].pv_idx = (int *)(p_data + offset);
				}else if(cond_field == COND_FIELD_TYPE){
					p_cond_gp->query_cond[cond_lv].type_argc = cond_argc;
					p_cond_gp->query_cond[cond_lv].type_oper = cond_oper;
					p_cond_gp->query_cond[cond_lv].pv_type = (int *)(p_data + offset);
				}
				
				offset += cond_argc * 4;

			}
	
		}
	}

	return head_cond_gp;

}


int get_idx_query_arg(idx_query_arg_t *p_idx_arg, query_cond_t *query_cond, int *idx_th, int min_idx, int max_idx)
{
	//int idx_th = 0;
	
	if(p_idx_arg == NULL){
		return -1;
	}
	
	p_idx_arg->query_times = 0;

	//printf("get_idx_query_arg : max_idx = %d \n", max_idx);


	if(query_cond == NULL){
		p_idx_arg->start_idx = min_idx;
		p_idx_arg->end_idx = max_idx;
		return 0;
	}

	switch(query_cond[0].idx_oper){
		case COND_OPER_EQ :
			p_idx_arg->start_idx = query_cond[0].pv_idx[0];
			p_idx_arg->end_idx = p_idx_arg->start_idx;
			break;
		case COND_OPER_GT :
			p_idx_arg->start_idx = query_cond[0].pv_idx[0] + 1;
			p_idx_arg->end_idx = max_idx;
			break;
		case COND_OPER_GE :
			p_idx_arg->start_idx = query_cond[0].pv_idx[0];
			p_idx_arg->end_idx = max_idx;
			break;
		case COND_OPER_LT :
			p_idx_arg->start_idx = min_idx;
			p_idx_arg->end_idx = query_cond[0].pv_idx[0] - 1;
			break;
		case COND_OPER_LE :
			p_idx_arg->start_idx = min_idx;
			p_idx_arg->end_idx = query_cond[0].pv_idx[0];
			break;
		case COND_OPER_RANGE :
			p_idx_arg->start_idx = query_cond[0].pv_idx[0];
			p_idx_arg->end_idx = query_cond[0].pv_idx[1];
			break;
		case COND_OPER_IN :
			if(idx_th){
				p_idx_arg->start_idx = query_cond[0].pv_idx[(*idx_th)++];
				p_idx_arg->end_idx = p_idx_arg->start_idx;
				p_idx_arg->query_times = query_cond[0].idx_argc - *idx_th;
			}
			break;
		default :
			p_idx_arg->end_idx = -1;
			p_idx_arg->query_times = 0;
			break;
		
	}

	if(p_idx_arg->end_idx > max_idx){
		p_idx_arg->end_idx = max_idx;
	}

	//print_query_cond(query_cond);
	//print_idx_query_arg(p_idx_arg);
	
	
	return 1;
}

int check_match_the_type(query_cond_t *query_cond, int type)
{
	int i = 0;

	if(query_cond == NULL){
		return 1;
	}

	
	//printf("check_match_the_type : type = %d \n", type);
	//print_query_cond(query_cond);

	switch(query_cond[0].type_oper){
		case COND_OPER_EQ :
			if(type == query_cond[0].pv_type[0]){
				return 1;
			}else{
				return 0;
			}
			break;
		case COND_OPER_GT :
			if(type > query_cond[0].pv_type[0]){
				return 1;
			}else{
				return 0;
			}
			break;
		case COND_OPER_GE :
			if(type >= query_cond[0].pv_type[0]){
				return 1;
			}else{
				return 0;
			}
		case COND_OPER_LT :
			if(type < query_cond[0].pv_type[0]){
				return 1;
			}else{
				return 0;
			}
		case COND_OPER_LE :
			if(type <= query_cond[0].pv_type[0]){
				return 1;
			}else{
				return 0;
			}
		case COND_OPER_RANGE :
			if((type >= query_cond[0].pv_type[0]) && (type <= query_cond[0].pv_type[1])){
				return 1;
			}else{
				return 0;
			}
		case COND_OPER_IN :
			for(i=0; i<query_cond[0].type_argc; i++){
				if(type == query_cond[0].pv_type[i]){
					return 1;
				}
			}
			return 0;
		default :
			return 0;
		
	}
	
	return 0;
}


void free_all_cond_group(query_cond_gp_t *head_cond_gp)
{
	query_cond_gp_t *cur_cond_gp, *next_cond_gp;

	cur_cond_gp = head_cond_gp;
	while(cur_cond_gp){
		next_cond_gp = cur_cond_gp->next;
		free(cur_cond_gp);
		cur_cond_gp = next_cond_gp;
	}
}



int query_level_1_idx(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond, void (*query_handler)(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond, int idx), int max_idx)
{
	int i, count = 0;
	int idx_th = 0;
	
	idx_query_arg_t idx_arg;

	
	if(p_ipcmsg == NULL){
		return -1;
	}
	//print_query_cond(query_cond);

	do{

		if(query_cond == NULL){
			idx_arg.start_idx = 0;
			idx_arg.end_idx = max_idx;
			idx_arg.query_times = 0;
		}else{
			get_idx_query_arg(&idx_arg, &query_cond[0], &idx_th, 0, max_idx);
		}

		for(i=idx_arg.start_idx; i<=idx_arg.end_idx; i++){

			query_handler(p_ipcmsg, query_cond, i);

			count++;

		}
		
	}while(idx_arg.query_times>0);

	return count;
}


int query_and_give_data_by_idx(ipc_msg_t *p_ipcmsg, char *p_data, int data_len, void (*query_handler)(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond, int idx), int max_idx)
{
	int record_count = 0;
	query_cond_gp_t *head_cond_gp, *p_cond_gp;
	
	head_cond_gp = parse_query_cond(p_ipcmsg, p_data, data_len);

	if(head_cond_gp){
		p_cond_gp = head_cond_gp;
		while(p_cond_gp){
			record_count += query_level_1_idx(p_ipcmsg, p_cond_gp->query_cond, query_handler, max_idx);
			p_cond_gp = p_cond_gp->next;
		}

		free_all_cond_group(head_cond_gp);
	}else{
		record_count = query_level_1_idx(p_ipcmsg, NULL, query_handler, max_idx);
	}

	return record_count;
}




