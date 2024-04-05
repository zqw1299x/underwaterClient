
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sched.h> 

#include "../include/globa.h"


void init_login(void)
{
	int i = 0;
	for(i=0;i<8;i++){
		myencrypt->key[i] = '0';
	}
	memset(myencrypt->username,0,sizeof(myencrypt->username));
	memset(myencrypt->password,0,sizeof(myencrypt->password));
	memcpy(myencrypt->username,"root",4);
	memcpy(myencrypt->password,"root",4);
	
}

int main(int argc, char *argv[])
{	
	unsigned int key1,key2,key3,key4,key5,key6,key7,key8;
	unsigned char buf[256];
	unsigned char tmp[500];
	char a;
	char *version;
	FILE *fp;
	int num = 0;
	int times = 0,i;
	char username[20]={0};
	char password[20]={0};
	open_global_shm();
	init_login();
	//printf("please input key:\n");
	while(1){
		printf("/***************************************************/\n");
		printf("/*1.change key;                                    */\n");
		printf("/*2.delete key;                                    */\n");
		printf("/*3.get version;                                   */\n");
		printf("/*4.change login username and password;            */\n");
		printf("/*5.exit                                           */\n");
		printf("/*please input num:                                */\n");
		printf("/***************************************************/\n");
		
		//memset(buf,0,sizeof(buf));
		scanf("%d",&num);
		if(num == 3){
			version = get_encrypt_version();
			printf("version:%s\n",version);
			continue;
		}else if(num == 5)
			break;
		//times = 0;
		while(1){
			printf("please input username\n");
			memset(buf,0,sizeof(buf));
			scanf("%s",buf);
			printf("please input password\n");
			memset(tmp,0,sizeof(tmp));
			scanf("%s",tmp);
			
			if((strcmp(buf,myencrypt->username) == 0) && (strcmp(tmp,myencrypt->password) == 0))
			{
				break;
			}else{
				times++;
				if(times == 4){
						printf("4 times error!!!!\n");
						break;
				}else
					printf("username or password error,input again\n");
				continue;
			}
			
		}
		
		
		
		if(num == 1)//change key
		{
			while(1){
				printf("please input old key\n");
				//printf("myencrypt->key[0]:%c\n",myencrypt->key[0]);
				memset(buf,0,sizeof(buf));
				scanf("%s",buf);
				for(i=0;i<strlen(myencrypt->key);i++){
					if(buf[i] != myencrypt->key[i])
					{
						break;
					}
				}
				if(i==strlen(myencrypt->key)){
					printf("please input new key,not more than 100\n");
				}
				else
				{
					times++;
					if(times == 4){
						printf("4 times error!!!!\n");
						break;
					}else
						printf("key is error,input again\n");
					continue;
				}
				memset(buf,0,sizeof(buf));
				scanf("%s",buf);
				memset(myencrypt->key,0,sizeof(myencrypt->key));
				memcpy(myencrypt->key,buf,strlen(buf));

				printf("change key success\n");
				break;
			}
		}else if(num == 2)//delete key
		{
			while(1){
				printf("please input old key\n");
				memset(buf,0,sizeof(buf));
				scanf("%s",buf);
				for(i=0;i<strlen(myencrypt->key);i++){
					if(buf[i] != myencrypt->key[i])
					{
						break;
					}
				}
				if(i==strlen(myencrypt->key)){
					memset(myencrypt->key,0,sizeof(myencrypt->key));
					for(i=0;i<8;i++){
						myencrypt->key[i] = '0';
					}
					printf("delete key success\n");
				}
				else
				{
					printf("key is error,input again\n");
					continue;
				}

				break;
			}
		}else if(num == 4)
		{
			printf("please input new username\n");
			memset(buf,0,sizeof(buf));
			scanf("%s",buf);
			memset(myencrypt->username,0,sizeof(myencrypt->username));
			memcpy(myencrypt->username,buf,strlen(buf));
			printf("please input new password\n");
			memset(tmp,0,sizeof(tmp));
			scanf("%s",tmp);
			memset(myencrypt->password,0,sizeof(myencrypt->password));
			memcpy(myencrypt->password,tmp,strlen(tmp));
			printf("change login username and password success\n");
		}


		usleep(200);

	}

	printf("Exit\n");
	return 0;
}
