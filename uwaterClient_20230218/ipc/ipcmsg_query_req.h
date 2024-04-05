#ifndef	__IPCMSG_QUERY_REQ_H_
#define	__IPCMSG_QUERY_REQ_H_

#include "ipcmsg.h"

#define	COND_FIELD_QTY	2
#define COND_FIELD_IDX	0
#define COND_FIELD_TYPE	1

#define	COND_OPER_EQ	0
#define	COND_OPER_GT	1
#define	COND_OPER_GE	2
#define	COND_OPER_LT	3
#define	COND_OPER_LE	4
#define	COND_OPER_RANGE	5
#define	COND_OPER_IN	6


#define VA_ARG_END		-1

#define COND_LV_QTY		2


int init_request_condition(ipc_msg_t *p_ipcmsg);
int add_request_condition_group(ipc_msg_t *p_ipcmsg);
int request_condition_addl(ipc_msg_t *p_ipcmsg, char level, char field, char cond_oper, ...);
int request_condition_addv(ipc_msg_t *p_ipcmsg, char level, char field, char cond_oper, char argc, int *val_arr);



typedef struct{
	u_char idx_oper;
	u_char idx_argc;
	u_char type_oper;
	u_char type_argc;
	int *pv_idx;
	int *pv_type;
}query_cond_t;


typedef struct _query_cond_group_st{
	query_cond_t query_cond[COND_LV_QTY];
	struct _query_cond_group_st *next;
}query_cond_gp_t;


typedef struct {
	int start_idx;
	int end_idx;
	int query_times;
}idx_query_arg_t;


typedef int (*query_callback_func)(ipc_msg_t *, query_cond_t *);

query_cond_gp_t *parse_query_cond(ipc_msg_t *p_ipcmsg, char *p_data, int data_len);
int query_and_give_data(ipc_msg_t *p_ipcmsg, char *p_data, int data_len, int (*query_callback)(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond));
int check_match_the_type(query_cond_t *query_cond, int type);
int get_idx_query_arg(idx_query_arg_t *p_idx_arg, query_cond_t *query_cond, int *idx_th, int min_idx, int max_idx);


void free_all_cond_group(query_cond_gp_t *head_cond_gp);

int query_and_give_data_by_idx(ipc_msg_t *p_ipcmsg, char *p_data, int data_len
	, void (*query_handler)(ipc_msg_t *p_ipcmsg, query_cond_t *query_cond, int idx), int max_idx);



#endif

