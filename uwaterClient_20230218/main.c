
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <termios.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include "include/globa.h"
    
#include <signal.h>   
#include <unistd.h>   
#include <netinet/in.h>   
#include <netinet/ip.h>   
#include <netinet/ip_icmp.h>   
#include <netdb.h>   
#include <setjmp.h>   
#include <errno.h>   

#include <sys/ipc.h>
#include <sys/msg.h>


#define PACKET_SIZE     4096   
#define MAX_NO_PACKETS  3   

int exit8001            = 0;
int exit800ok           = 0;
int exit800             = 0;
int exit8001ok          = 0;
int exit8080            = 0;
int exit8080ok          = 0;

int uscount             = 0;
int timeCnt             = 0;
int g_timeStopFlag      = 0;
int resetBDcnt          = 0;
int counter_1Hz         = 0;
int counter_2Hz         = 0;
int counter_5Hz         = 0;
int counter_10Hz        = 0;
int counter_60Hz        = 0;
int counter_20Hz        = 0;

int counter_3Hz         = 0;
int feedbackTimer       = 0;
int feedbackOpen        = 0;
int feedbackOK          = 0;
int feedbackErrorCount  = 0;

char g_errorStatuTxFlag = 0;
char g_readBuf[1024];
int  g_lenth            = 0;
int  timeoutBreak       = 0;
char setsysTimeFlag     = 1;



#define WGETENC 		"/root/wget -P /root/LFile/LEnc ftp://root@192.168.168.240/root/LFile/LEnc/* -c"
#define WGETPC104DUTY   "/root/wget -P /root/LFile/LDec ftp://root@192.168.168.240/media/mmcblk0p1/BBFile/duty.txt -c"

#define  debugMode

#ifdef debugMode

//此处存在疑问，mount应该挂载在何处0.3或0.55
//#define MOUNT 			"mount -t cifs //192.168.0.55/hf-data-share /media/mmcblk0p1 -o username=administrator,password=hf123456,sec=ntlm"
//#define MOUNT 			"mount -t cifs //192.168.0.55/hf-data-share /media/mmcblk0p1 -o username=12991,password=271221,sec=ntlm"

//#define MOUNT 			"mount -t cifs //192.168.0.3/home /media/mmcblk0p1 -o username=15993,password=2271221,sec=ntlm"
#define MOUNT 			"mount -t cifs //192.168.0.3/home /media/mmcblk0p1 -o username=12991,password=271221,sec=ntlm"
#else 

#define MOUNT 			"mount -t cifs //192.168.0.3/home /media/mmcblk0p1 -o username=target,password=password,sec=ntlm" //(天和控制微机)

#endif


#define SERVERIP 		"192.168.168.240"
//#define DTLOCALIP 		"192.168.0.41"
#define DTLOCALIP 		"192.168.168.241"

//#define DTSERVERIP 		"192.168.0.34"
#define DTSERVERIP 		"192.168.168.34"
//#define DTSERVERIP 		"192.168.168.240"

#define TCPSERVERIP     "192.168.0.41"

//#define UDPLOCALIP		"192.168.0.241"
#define UDPLOCALIP		"192.168.0.2"

//#define UDPSERVERIP		"192.168.0.41"
#define UDPSERVERIP		"192.168.0.3"
//#define UDPSERVERIP		"192.168.0.55"


//#define UDPSERVERPORT   8010
#define UDPSERVERPORT   8000


//#define UDPLOCALPORT    8010
#define UDPLOCALPORT    9000


#define DTLOCALPORT     8001
#define DTSERVERPORT    8001
#define TCPSERVERPORT   8010

#define DTLOCALPORT_E   8001
#define DTSERVERPORT_E  8001


#define KZWJIP   		"192.168.0.3"


#define pingDT_CMD  	"ping 192.168.168.34 -c 2 > /root/LFile/ping.log"
#define Pingpc104_CMD  	"ping 192.168.0.55 -c 2 > /root/LFile/ping.log"

#define pingfile 		"/root/LFile/ping.log"

#define ENCFILELIST 	"ls -lh /root/LFile/LEnc/ | grep ^-> /root/LFile/Filelist.log"
#define DECFILELIST 	"ls -lh /root/LFile/LDec/ | grep ^-> /root/LFile/Filelist.log"
#define FILELIST 		"/root/LFile/Filelist.log"

pthread_t   *ThdWGet_t, *ThdBDPos_t,*ThdDBMsg_t,*ThdDTSet_Uart4_t,*ThdEbyte_Lora,*ThdBB_t,*ThdPingPC104_t,
*ThdCheckLinkBB_t,*ThdFeedBackLinkBB_t,*ThdBDTXSQ_t,*ThdPosToBB_t,*socket_client_1_task_t,*socket_client_2_task_t,
*socket_client_3_task_t,*ThdConn8080_t,*ThdConn800_t,*ThdBDICJC_t,*ThdConn8001_t,*ThdParsePC_Udp_t,*ThdLora_Uart7_t,
ThdYTJ_Uart8_t,ThdU8001Process_t,*QueTimerProcess_t;
pthread_mutex_t mut;
pthread_mutex_t udp;
pthread_mutex_t que;


char flagSend = 0;

struct Uwater{
    char SIMcard;			//sim卡在位情况 ，   	1：卡在位，  0：卡异常
    char tx_RNSS;			//RNSS串口通信情况  	1：通信正常  0：通信异常
    char tx_RDSS;			//RDSS串口通信情况  	1：通信正常  0：通信异常
    char DT_uart;			//电台串口通信情况
    char DT_100Base;		//电台网口通信情况
    char external_Uart;		//外部串口通信情况
    char external_100base;	//外部网口通信情况
	//int  i800SendNum;
} Uwater_status;

struct protocol{
	char head;
	char length;
	char type;
	struct Uwater content;
	char crc;
}protocol_ZJ;

/**********************************************************/
#define MSG_SIZE 6144
struct message {
    long mtype;
    char mtext[MSG_SIZE];
};
int msgid;

struct message msge_f;
struct message msge_r; 

struct messageC {
    long mtype;
    char mtext[1024];
};
struct messageC msge_c;
/**********************************************************/

char sendbuff[100] = {0};
char protocol_str_tmp[100] = {0};

int  send_time_flag = 0;
int  set_time_flag  = 0;
int  rcv_cmd_flag   = 0;
char navigational_status   = 0;   //航行状态
char malfunction    = 0;          //故障状态
char electrical_status     = 0;   //电机状态
char LinkBBFlag1    = 0;           //与BB控制微机通信状态  0:正常 1:通信异常
char LinkBBFlag2    = 0;           //与控制微机通信状态  0:正常 1:通信异常
char linkKzwjFlag   = 0;           //与控制微机通信状态   0:正常 1:通信异常


char rds_buf[1024]   = {0};
char rds_rcv_buf[10] = {0};
char rns_buf[1024]   = {0};
char TXXX_buf[1024]  = {0};       //通信信息buf
char ZJXX_buf[1024]  = {0};
char ICJC_buf[3]     = {0};
uint16_t depth = 0;

char snd_240Sv[1024] = {0};
int SndDTFlag = 0;
int check_ICJC_flag  = 2;

int bufLen = 1024;
int dataLen = 1024-5;


int failure_times  = 0;
int success_times  = 0;
int rcv_file_flag  = 0;            //1:已接收完数据	2:可以接收数据		3:接收数据有误		0:启机状态
int snd_error_flag = 0;
int snd_error_module[100] = {0};
int snd_beidou_addr_info  = 2;     //是否发送位置信息标志位
int g_count   = 0;
int g_sencunt = 0;
int countstop = 0;
char recvbuf_f[1024] = {0};
int len_f = 0;

char GNRMC[6] = {'$','G','N','R','M','C'};
char GNGGA[6] = {'$','G','N','G','G','A'};
char TXSQ[5]  = {'$','T','X','S','Q'};
char XTZJ[5]  = {'$','X','T','Z','J'};
char SJSC[5]  = {'$','S','J','S','C'};
char BBSQ[5]  = {'$','B','B','S','Q'};

char ICXX[5]  = {'$','I','C','X','X'};
char FKXX[5]  = {'$','F','K','X','X'};
char TXXX[5]  = {'$','T','X','X','X'};
char ZJXX[5]  = {'$','Z','J','X','X'};
char BBXX[5]  = {'$','B','B','X','X'};
char SJXX[5]  = {'$','S','J','X','X'};

int  g_dt_udpsock   =   0;
int  g_tcpsock      =   0;
int  g_kzwj_udpsock =   0;
char g_bstop        =   0;
char OPEN           =   0;

struct sockaddr_in g_dt_serv_addr;
struct sockaddr_in g_kzwj_serv_addr;
struct sockaddr_in g_dt_serv_addr_e;

struct packet_flag
{
    unsigned char flag0;
    unsigned char flag2;
    unsigned char flag3;
    int len;
    time_t lasttime;
};

void Thddebug();
void Uwater_status_init();
void Uwater_status_check(void);
void SIMcard_check(void);
int  get_ping_result();
void DT_100base_check(void);
void external_100base_check(void);
void Status_struct2string(struct Uwater st);
void protocol_struct2string(struct protocol pro );
void PowerMode_Select(void);

int  UdpMessageRecv(int sock,char* recvbuf,int buflen,struct sockaddr_in  serv_addr);
int  UdpMessageSend(int sock,char* strmsg,int msglen,struct sockaddr_in serv_addr);
void UdpFileRecv(int sndsock,struct sockaddr_in clnt_addr,char* filename);
void UdpFileSend(int sndsock,struct sockaddr_in serv_addr,char* filename);

void print_buffer1(char* buffer, int length) 
{
	int i=0;
	FILE *fp;
	fp = fopen("/root/beidou.txt","a+");
	
	if(buffer==NULL)return;
	printf("print buf data length:%d\n",length);
	//printf("buffer[0]:%x\n",buffer[0]);
	for(i = 0;i<length;i++)
	{
		printf("%x ", buffer[i]);
		fprintf(fp,"%x ",buffer[i]);
		//if( (i+1)%16 == 0)
		//	printf("\n");
	}
	printf("\n\nend!\n");
	fclose(fp);
}

void print_buffer(char* buffer, int length) 
{
	int i=0;
	
	if(buffer==NULL)
	{
		return;
	}
	printf("print buf data length:%d\n",length);
	//printf("buffer[0]:%x\n",buffer[0]);
	for(i = 0;i<length;i++)
	{
		printf("%x ", buffer[i]);
		
		//if( (i+1)%16 == 0)
		//	printf("\n");
	}
	printf("\n\nend!\n");	
}

void print_buffer_asii(char* buffer, int length) 
{
	int i=0;
	if(buffer==NULL)
	{
		return;
	}
	printf("print asii data length:%d\n",length);
	//printf("buffer[0]:%x\n",buffer[0]);
	for(i = 0;i<length;i++)
	{
		printf("%c", buffer[i]);
		//if( (i+1)%16 == 0)
		//	printf("\n");
	}
	printf("\n\nend!\n");
}
int check_crc(char *buf,char *buffer)//check beidou crc
{
	int k = 0,reg = 0,len = 0,i = 0;
	for(k=0;k<1024;k++)
	{
		if((buf[k] == buffer[0]) && (buf[k+1] == buffer[1]) && (buf[k+2] == buffer[2]) && (buf[k+3] == buffer[3]) && (buf[k+4] == buffer[4]))
		{
			//printf("check %c%c%c%c%c crc start:\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
			for(i=1;i<1024;i++)
			{
				if(buf[i] == '$')
				{
					len = i;
					break;
				}				
			}
			//printf("len::%d\n",len);
			reg = 0;
			for(k=0;k<len-1;k++)
			{
				reg ^= buf[k];//jiaoyanhe
			}
			//printf("reg:%x,%x\n",reg,buf[len - 1]);
			if(reg != buf[len - 1])
			{
				printf("warning:%c%c%c%c%c crc is error!!!!!!!!!!\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
				printf("crc reg:%x,%x\n",reg,buf[len - 1]);
				//print_buffer(buf,sizeof(buf));
				return -1;
			}
			else
			{
				printf("%c%c%c%c%c crc OK!\n",buffer[0],buffer[1],buffer[2],buffer[3],buffer[4]);
				return 0;
			}
			break;
		}
	}
}

int check_sum_host_PC(char *buf,int len)//检测与综合计算机通信的CRC
{
	int i = 0;
	int crc = 0;
	for(i=1;i<len-1;i++)
	{
		crc += buf[i];
	}
	crc &= 0xff;
    printf("crcResutl:%0x --- %0x\r\n",crc,buf[len-1]);
	if(crc == buf[len-1])
	{
		return 0;
	}
	return crc;
}

void diantai_power_control(int flag)
{
	if(flag == 0)
	{
		system("gpio-test out 1 0");
		usleep(200);
		system("gpio-test out 19 0");
		usleep(200);
		system("gpio-test out 2 0");
	}
	else if(flag == 1)
	{
		system("gpio-test out 2 1");
		usleep(200);
		system("gpio-test out 19 1");
		usleep(200);
		system("gpio-test out 1 1");
	}
}

int calc_sum_host_pc(char*buf,int len)
{
    int i = 0;
    int crc = 0;
    for(i=0;i<len-1;i++)
    {
        crc += buf[i];
    }
    crc &= 0xff;
    return crc;
}

void snd_to_diantai(char *data_from)
{
	char buf[50];
	int i = 0;
	sleep(3);
	send_message_to_uart(uart.fd_4,"admin\n",7);
	sleep(3);
	send_message_to_uart(uart.fd_4,"123456\n",8);
	sleep(5);
	//printf("data_from:%s\n",data_from);
	SndDTFlag = 1;
	memset(buf,0,sizeof(buf));
	sprintf(buf,"%s\n",data_from);
	send_message_to_uart(uart.fd_4,buf,strlen(buf));
	
	sleep(3);
	SndDTFlag = 0;
	for(i=0;i<strlen(data_from);i++)
	{
		if(data_from[i] == '=')
		{
			send_message_to_uart(uart.fd_4,"at&w\n",6);
			sleep(4);
		}
	}
	
	send_message_to_uart(uart.fd_4,"ata\n",5);
}

/*void ThdBDPos_Uart2(void)  //实现北斗二代与GPS的定位信息解析
{	
	int Ret,failtimes = 0,times = 0;
	int i,j;
	char buf[1024] = {0};
	printf("start ThreadParseBDPos_thread\n");
	
	while(1)
	{
		memset(buf, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_2,buf,RNS_LEN); //length is 100
		pthread_mutex_unlock(&mut);
		if (Ret == -1)	
		{
			printf("read get_message_from_uart error.\n");
			write_log("BD communication is error\r\n");
			failtimes++;
			if(failtimes < 3)
			{
				//k = 0;
				//检测到北斗异常				
				continue;
			}
			else
			{				
				Uwater_status. tx_RNSS=0;
				printf("UART2 RNSS get message  error！！failtimes:%d \n",failtimes );
				continue;				
			}
		}
		
		if(Ret > 1)
		{		
			
			failtimes=0;
			Uwater_status. tx_RNSS=1;
			
			for(i=0;i<RNS_LEN;i++)
			{
				if((buf[i] == GNRMC[0]) && (buf[i+1] == GNRMC[1]) && (buf[i+2] == GNRMC[2]) && (buf[i+3] == GNRMC[3]) && (buf[i+4] == GNRMC[4]) && (buf[i+5] == GNRMC[5]))//$GNRMC
				{
					for(j=i;j<1024;j++)
					{
						if((buf[j] == 0x0d) && (buf[j+1] == 0x0a))
						{
							buf[j] = '\0';
							break;
						}
					}
					memset(rns_buf, 0, 1024);
					memcpy(rns_buf,&buf[i],j-i);					
					break;
				}	
			}				
		}	
		usleep(200);
	}
}*/

int check_crc_hex(char *buf,int len)
{
	int k = 0,reg = 0;
	for(k=0;k<len-1;k++)
	{
		reg ^= buf[k];
	}

	if(reg != buf[len - 1])
	{
		printf("check_crc error:%02x-%02x\n",reg,buf[len-1]);
		return -1;
	}
	else
	{
		printf("check_crc ok:%02x-%02x\n",reg,buf[len-1]);
		return 0;
	}
}


//**********************************ThdDBMsg_Uart3********************************//
//ThdDBMsg_Uart3实现北斗一代通信信息接收解析。
//*******************************************************************************//
void ThdDBMsg_Uart3(void) //实现北斗一代通信协议接收，ICXX，FKXX，TXXX等
{	
	int Ret,failtimes = 0;
	char buf[1024] = {0};
	char buffer[1024] = {0};
	int crc_ok = -1;
	printf("start Uart_thread3\n");
	int rcv_times = 0;

	while(1)
	{
		memset(buf, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_3,buf,RNS_LEN); //
		pthread_mutex_unlock(&mut);
		if (Ret == -1)
		{
			printf("read get_message_from_uart error！\n");
			write_log("read Uart3 is error\r\n");
			failtimes++;
			if(failtimes < 10)
			{
				//k = 0;
				continue;
			}
			else
			{
				Uwater_status. tx_RDSS=0;
				printf("UART3 RDSS get message  error！！failtimes:%d \n",failtimes );
				continue;
			}
		}
		else if(Ret > 1)
		{
			Uwater_status. tx_RDSS=1;  
			failtimes=0;
			printf("UART3 got the message \n",Ret);
			print_buffer_asii(buf,Ret);		
			if((buf[0] == ICXX[0]) && (buf[1] == ICXX[1]) && (buf[2] == ICXX[2]) && (buf[3] == ICXX[3]) && (buf[4] == ICXX[4]))//$ICXX
			{
				memset(rds_buf,0,1024);
				memcpy(rds_buf,buf,sizeof(buf));
				if(rds_buf[7]==0x00 && rds_buf[8]==0x00 && rds_buf[9]==0x00) //判断SIMcard状态
					Uwater_status. SIMcard=0; 
				else
				{
					Uwater_status. SIMcard=1; 
					printf("SIMcar is  :%02x,%02x,%02x\n",rds_buf[7],rds_buf[8],rds_buf[9]);
				}						
			}
				
			if((buf[0] == FKXX[0]) && (buf[1] == FKXX[1]) && (buf[2] == FKXX[2]) && (buf[3] == FKXX[3]) && (buf[4] == FKXX[4]))//$FKXX
			{
				//printf("$FKXX:%x\n",buf[10]);
				crc_ok = check_crc(buf,&buf[0]);
				if(crc_ok == 0)
				{
					//print_buffer(buf,Ret);
					if(buf[10] == 0)
					{
						success_times ++;
						printf("Success,and success times is:%d\n",success_times);											
					}
					else if(buf[10] == 1)
					{
						failure_times ++;
						printf("Failure and please retry,FKXX failure times:%d\n",failure_times);
					}
					else if(buf[10] == 2)
					{
						printf("Signal unlock ,please move BD and retry\n");
						//failure_times ++;
					}
					else if(buf[10] == 3)
						printf("Electricity not enough\n");
					else if(buf[10] == 4)
						printf("Sent frequency not rcv\n");
					else if(buf[10] == 5)
						printf("Encoder error\n");
					else if(buf[10] == 6)
						printf("CRC error\n");
				}				
			}
			
			if((buf[0] == TXXX[0]) && (buf[1] == TXXX[1]) && (buf[2] == TXXX[2]) && (buf[3] == TXXX[3]) && (buf[4] == TXXX[4]))//$TXXX
			{			
				printf("start print $TXXX:\n");
				//printf("len:::%d\n",sizeof(&buf[k]));
				crc_ok = check_crc(buf,&buf[0]);
				if(crc_ok == 0)
				{
					print_buffer(buf,Ret);
					//rcv_auv_flag = 1;
					memset(TXXX_buf,0,sizeof(TXXX_buf));
					memcpy(TXXX_buf,buf,Ret);
					//print_buffer1(buf,Ret);
				}
				rcv_times ++;
				//printf("\033[5;31m TXXX rcv_times:%d: \033[0m \n",rcv_times);
				//printf("finish print $TXXX!!!\n");
				//continue;
			}
			
			if((buf[0] == ZJXX[0]) && (buf[1] == ZJXX[1]) && (buf[2] == ZJXX[2]) && (buf[3] == ZJXX[3]) && (buf[4] == ZJXX[4]))//$ZJXX
			{
				crc_ok = check_crc(buf,&buf[0]);
				if(crc_ok == 0)
				{
					//printf("start print $ZJXX:\n");
					//print_buffer(buf,Ret);
						
					//printf("IC status:%x\n",buf[10]);
					//printf("hardboard status:%x\n",buf[11]);
					//printf("dianliang:%x\n",buf[12]);
					memset(ZJXX_buf,0,1024);
					memcpy(ZJXX_buf,&buf[0],(buf[5]<<8)+buf[6]);
					//printf("ruzhan status:%x\n",buf[13]);
					printf("gonglv:%x %x %x %x %x %x\n",buf[14],buf[15],buf[16],buf[17],buf[18],buf[19]);
					//printf("finish print $ZJXX!!!\n");
				}				
				//continue;
			}
			
			if((buf[0] == BBXX[0]) && (buf[1] == BBXX[1]) && (buf[2] == BBXX[2]) && (buf[3] == BBXX[3]) && (buf[4] == BBXX[4]))//$BBXX
			{
				crc_ok = check_crc(buf,&buf[0]);
				if(crc_ok == 0)
				{
					printf("start print $BBXX:\n");
					print_buffer(buf,Ret);					
				}
				//continue;
			}
			if((buf[0] == SJXX[0]) && (buf[1] == SJXX[1]) && (buf[2] == SJXX[2]) && (buf[3] == SJXX[3]) && (buf[4] == SJXX[4]))//$SJXX
			{
				crc_ok = check_crc(buf,&buf[0]);
				if(crc_ok == 0){
					printf("start print $SJXX:\n");
					print_buffer(buf,Ret);				
					printf("beidou time(year,month,day,hour,minute,second):%x%x %x %x %x %x %x\n",buf[10],buf[11],buf[12],buf[13],buf[14],buf[15],buf[16]);					
					printf("finish print $SJXX!!!\n");
					
					if (set_time_flag == 0)
					{
						char date[64] = {0};
						memset(date,0,64);
						sprintf(date,"date -s “%2x%2x-%2x-%2x %2x:%2x:%2x”",buf[10],buf[11],buf[12],buf[13],buf[14],buf[15],buf[16]);
						system(date);
						system("hwclock -w");
						set_time_flag = 1;
					}
				}
				//continue;
			}		
		}
		usleep(100);
	}
}

void ThdDTSet_Uart4(void)//实现电台模块串口通信
{
	int Ret,k=0;
	int i;
	char buf[1024] = {0};
	char decrypt_buf[1024] = {0};
	char cmd_buf[1024] = {0};
	int special_cmd = 0;
	int num = 0,reg = -1;
	while(1)
	{
		memset(buf, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_4,buf,RNS_LEN); //length is 100
		pthread_mutex_unlock(&mut);
		if (Ret == -1)	
		{
			printf("read fd_4 error.\n");
			write_log(" read Uart4 is error\r\n");
			k++;
			if(k < 3)
			{
				k = 0;
				continue;
			}
		}
		else if(Ret > 1)
		{			
			printf("\033[5;31m start print uart4 rcv info %d: \033[0m \n",Ret);
			print_buffer_asii(buf,Ret);
			printf("SndDTFlag:%x\n",SndDTFlag);
			
			if(SndDTFlag == 1)
			{				
				memset(cmd_buf,0,sizeof(cmd_buf));
				cmd_buf[0] = 0x8A;
				cmd_buf[1] = (Ret+4);
				cmd_buf[2] = 0x03;
				cmd_buf[3] = 0x01;
				memcpy(&cmd_buf[4],buf,Ret);
				cmd_buf[Ret+4] = 0;
				for(i=1;i<(Ret+4);i++)
					cmd_buf[Ret+4] += cmd_buf[i];
				cmd_buf[Ret+4] &= 0xff;
				memset(snd_240Sv,0,sizeof(snd_240Sv));
				
				snd_240Sv[0] = cmd_buf[0];
				otp_encrypt_buf(&cmd_buf[1],Ret+4,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);				
				rcv_cmd_flag = 1;
			}			
		}		
		
	sleep(2);
	}
}

void QueCreateNew(void)
{
    key_t key;
    // 生成一个key
    if ((key = ftok(".", 'a')) == -1) {
        perror("ftok");
        exit(1);
    }
    // 创建消息队列
    if ((msgid = msgget(key, 0666 | IPC_CREAT)) == -1) {
        perror("msgget");
        exit(1);
    }
}

void QueSndCmd(char * str,size_t len)
{
    int ret;
    pthread_mutex_lock(&que);
    memset(&msge_c,0,sizeof(msge_c));
    msge_c.mtype = 1;
    printf("Enter a messageCmd:\r\n");
    for(int i=0;i<len;i++)msge_c.mtext[i] = str[i];
    //ret = msgsnd(msgid, &msge_c, sizeof(msge_c.mtext), 0);
    while((ret = msgsnd(msgid, &msge_c, sizeof(msge_c.mtext), 0)) == -1 && errno == EINTR){
       continue;
    }

//    // 检查是否发生中断
//    if (ret == -1 && errno == EINTR) {
//        continue;
//    } 

    if(ret == -1)
    {
        perror("msgsnd");
        exit(1);
    }

    pthread_mutex_unlock(&que);
}

void QueSndFile(char * str,size_t len)
{
    int ret;
    pthread_mutex_lock(&que);
    memset(&msge_f,0,sizeof(msge_f));
    msge_f.mtype = 1;
    printf("Enter a messageFile:\r\n");
    for(int i=0;i<len;i++)msge_f.mtext[i] = str[i];
    //ret = msgsnd(msgid, &msge_f, sizeof(msge_f.mtext), 0);
    while((ret = msgsnd(msgid, &msge_f, sizeof(msge_f.mtext), 0)) == -1 && errno == EINTR){
       continue;
    }

//    // 检查是否发生中断
//    if (ret == -1 && errno == EINTR) {
//        // 处理中断，例如重新尝试发送消息
//        continue;
//    } 

    if(ret == -1)
    {
        perror("msgsnd");
        exit(1);
    }
    pthread_mutex_unlock(&que);
}

void QueTimerProcess(void)
{
    char sndbuf[1024];
    char buffer[1024];          // 缓冲区用于存储接收到的字节数据
    int  bufferLength = 0;      // 缓冲区当前的长度
    int  len = 0,recvLen = 0;   
    int  Fcount = 0,Cmdcount = 0;
    while(1)
    {
        msge_r.mtype = 1;
        len = msgrcv(msgid, &msge_r, sizeof(msge_r.mtext), msge_r.mtype, 0);

        if (len == -1) {
            if (errno == EINTR) {
                continue; // 重新启动msgrcv函数
            } else {
                perror("msgrcv");
                exit(1);
            }
        }

        
//        if(len <= 0) {
//            perror("msgrcv");
//            exit(1);
//        }
        else {
            if(msge_r.mtext[0] == 0x01)
                recvLen = strlen(msge_r.mtext+5)+5;
            else if((msge_r.mtext[0] == 0x7F)||(msge_r.mtext[0] == 0x7E)||(msge_r.mtext[0] == 0x7C)||(msge_r.mtext[0] == 0x8A))
                recvLen = (msge_r.mtext[1] - 0xb0)+1;
            else if((msge_r.mtext[0] == 'k')&&(msge_r.mtext[1] == 'o'))
                recvLen = 2;
            UdpMessageSend(g_dt_udpsock,msge_r.mtext,recvLen,g_dt_serv_addr);
            memset(&msge_r,0,sizeof(msge_r));
            //printf("lenthmsg1:%d -- %d --- %d\r\n",Fcount,Cmdcount,recvLen);
            //if(msge_r.mtext[0] == 0x01)Fcount++;
            //if(msge_r.mtext[0] == 0x7f)Cmdcount++;
        }
        usleep(1000*20);
    }
}


void ThdParsePC_Udp(void)//实现与综合计算机通信的串口协议，包括7F、7E、8A等。
{
	int  Ret;
	int  i = 0, crc_ok = 0;
	char buf[1024] = { 0 };
	char encrypt_buf[1024] = { 0 };
	int  num = 0, reg = -1;
	/**********************************************/
	int clnt_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (clnt_sock < 0 )
	{
		printf("socket create error---return\n");
		return;
	}
	/**********************************************/
	//local
	struct sockaddr_in local_addr;
	// memset(&local_addr,0,sizeof(local_addr));
	local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(UDPLOCALIP);
	local_addr.sin_port = htons(UDPLOCALPORT);
	/**********************************************/
	//server
	struct sockaddr_in serv_addr;
	//memset(&serv_addr,0,sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(UDPSERVERIP);
	serv_addr.sin_port = htons(UDPSERVERPORT);
    /**********************************************/
	memcpy(&g_kzwj_serv_addr, &serv_addr, sizeof(serv_addr));
    g_kzwj_serv_addr.sin_family = AF_INET;
	g_kzwj_serv_addr.sin_addr.s_addr = inet_addr(UDPSERVERIP);
	g_kzwj_serv_addr.sin_port = htons(UDPSERVERPORT);
    g_kzwj_udpsock = clnt_sock;
	/**********************************************/
	if (0 != bind(clnt_sock, (struct sockaddr *)&local_addr, sizeof(local_addr)))
	{
		printf("bind failed ip:%s  --port=%d  \n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
		printf("-------to:%s:%d---------\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
		return;
	}
	/**********************************************/
	socklen_t clnt_addr_size = 0;
	struct sockaddr_in clnt_addr;
    /**********************************************/
	while (1)
	{
		memset(buf, 0, 1024);
		Ret = recvfrom(clnt_sock, buf, sizeof(buf), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
		if (Ret < 1)
		{
			printf("udp 8010 read error\n");
			continue;
		}
		if (Ret > 1)
		{
			printf("udp8010 recv:\n");
			print_buffer(buf, Ret);
            if((buf[0] == 0x7F) || (buf[0] == 0x7E)|| (buf[0] == 0x8A))
			{
				num = check_sum_host_PC(&buf[0], buf[1] + 1);
				if (0 == num)
				{
                    if((buf[0] == 0x7E) && (buf[2] == 0x20))
                    {
                        memcpy(&depth,&buf[3],2);
                        navigational_status = buf[5];
                        malfunction         = buf[6];
                        electrical_status   = buf[7];
                        continue;
                    }		
					memset(snd_240Sv, 0, sizeof(snd_240Sv));
					snd_240Sv[0] = buf[0];
					otp_encrypt_buf(&buf[1], Ret - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                    UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
				}
			}
            else if((buf[0] == 0x7C)&&(buf[2] == 0x0A))//测试反馈信息—表5 command:0x7c 06 0A 0A 01 1b
			{					
				num = check_sum_host_PC(buf, Ret);
				if(0 == num)
				{
					feedbackOK = 1; 
				}
			}
		}
		usleep(1000);
	}
	close(clnt_sock);
}


//***********************************ThdBB_Uart5**************************************//
//ThdBB_Uart5实现接收Uart5的串口信息，并对信息进行解析，如果为7E，则提取水深信息并进行控制
//其他信息则通过网络进行转发。
//***********************************ThdBB_Uart5**************************************//
void ThdBB_Uart5(void)//实现与综合计算机通信的串口协议，包括7F、7E、8A等。
{	
	int Ret,failtimes = 0;
	int i=0,crc_ok = 0;
	char buf[1024] = {0};
	char encrypt_buf[1024] = {0};
	//uint16_t depth = 0;
	int num = 0,reg = -1;
	printf("ThdBB_Uart5\n");
	while(1)
	{
		memset(buf, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_5,buf,RNS_LEN); //length is 100
		pthread_mutex_unlock(&mut);
		if (Ret == -1)	
		{
			printf("read fd_5 error.\n");
			write_log(" read Uart5 is error\r\n");
			failtimes++;
			if(failtimes < 10)
			{
				//failtimes = 0;
				continue;
			}
			else
			{
				Uwater_status. external_Uart=0;	
				printf("uart5 to PC104 error failtimes:%d\n",failtimes);
				continue;
			}
		}
	//	LinkBBFlag1 = 1;
	//	LinkBBFlag2 = 1;
		if(Ret > 1)
		{
			failtimes=0;
			Uwater_status. external_Uart=1;	
			//print_buffer(buf,Ret);
			//printf("\033[5;31m start print uart5 rcv info %d: \033[0m \n",Ret);
			//print_buffer_asii(buf,Ret);
			//print_buffer(buf,Ret);
			//printf("\033[5;31m finish print uart5 rcv info! \033[0m \n");
			if((buf[0] == 0x7F) || (buf[0] == 0x7E)|| (buf[0] == 0x8A))
			{
				LinkBBFlag1 = 0;
				LinkBBFlag2 = 0;
				num = check_sum_host_PC(&buf[0],buf[1]+1);
				if(0 == num)
				{
					if((buf[0] == 0x7E) && (buf[2] == 0x20))
					{
						memcpy(&depth,&buf[3],2);
						navigational_status = buf[5];
						malfunction = buf[6];
						electrical_status = buf[7];
						continue;
					}			
					memset(snd_240Sv,0,sizeof(snd_240Sv));
					snd_240Sv[0] = buf[0];
					otp_encrypt_buf(&buf[1],Ret-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);						
					rcv_cmd_flag = 1;
				}
			}	
			else
				write_log(" data received from Uart5 is error\r\n");
		}
		//sleep(2);
		usleep(200);
	}
}

void ThdYTJ_Uart8(void)//
{
	int Ret=0,Reg=0;
    int i=0,j=0;
	char buf_recv[1024]={0},crc_ok;
	while(1)
	{
		memset(buf_recv, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_8,buf_recv,RNS_LEN); //length is 1024
		pthread_mutex_unlock(&mut); 
		
		if (Ret == -1)	
		{ 
			printf("read fd_8 error.\n");
            Uwater_status. tx_RNSS=0;
		}
		else if(Ret > 1)
		{			
            //printf("GBRMC is already received---:hex\r\n");
            //print_buffer(buf_recv, Ret);
            //printf("GBRMC is already received---:ascall\r\n");
            //print_buffer_asii(buf_recv, Ret);
            Uwater_status. tx_RNSS=1;
    		for(i=0;i<RNS_LEN;i++)
    		{
    			if((buf_recv[i] == GNRMC[0]) && (buf_recv[i+1] == GNRMC[1]) && (buf_recv[i+2] == GNRMC[2]) && (buf_recv[i+3] == GNRMC[3]) && (buf_recv[i+4] == GNRMC[4]) && (buf_recv[i+5] == GNRMC[5]))//$GNRMC
    			{
    				for(j=i;j<1024;j++)
    				{
    					if((buf_recv[j] == 0x0d) && (buf_recv[j+1] == 0x0a))
    					{
    						buf_recv[j] = '\0';
                            printf("GBRMC is already received but data is normal\r\n");
                            print_buffer_asii(buf_recv,Ret);
                            //SendLoraBD_Uart7(rns_buf,rds_buf);
    						break;
    					}
    				}
    				memset(rns_buf, 0, 1024);
                    memcpy(rns_buf,&buf_recv[i],j-i);
    				printf("Client cycle_1s Recv GBRMC Data&Lenth:%d\n",j-i);
    				print_buffer_asii(rns_buf,j-i);
    				break;
    			}	
    		}
		}
	//usleep(1000*100);
	}
}

void ThdEbyte_Lora_Uart8(void)
{
	int Ret,failtimes = 0;
	int i=0,reg = 0;
	char buf[1024] = {0};
	char buf_snd[] = "this is Lora test programe 20220826";
	printf("Thd_Lora_Uart8\n");
	while(1)
	{
	    reg = send_message_to_uart(uart.fd_8,buf_snd,sizeof(buf_snd));	
		reg=0;
		memset(buf, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_5,buf,RNS_LEN); //length is 100
		pthread_mutex_unlock(&mut);
		if (Ret == -1)	
		{
			printf("read fd_8 error.\n");
			write_log(" read Uart8 is error\r\n");
		}
		if(Ret > 1)
		{
			failtimes=0;
			Uwater_status. external_Uart=1;	
			if((buf[0] == 0x7F) && (buf[1] == 0x8A))
			{
			    printf("uart8Disply:"); 
				for(i=0;i<RNS_LEN;i++)printf("%02x ",buf[i]);
				printf("\r\n");
				memset(buf,0,1024);
				reg = send_message_to_uart(uart.fd_8,buf_snd,sizeof(buf_snd));	
			}else {
				printf("uart8Disply_F:");
				for(i=0;i<8;i++)printf("%02x ",buf[i]);
				printf("\r\n");
			}
		}
		usleep(200);
	}
	
}
int GetSection(char Str[],char FinStr[],char Index,char StrValue[])
{
    char *pStr=Str;
    char *pStr1=NULL;
    char Index1=0;
    int Length = 0;
    for (;;)
    {
        pStr=strstr(pStr,FinStr);
        if (pStr!=NULL)
        {
            if (Index1==Index)
            {
                pStr1=strstr(pStr+1,FinStr);
                if (pStr1!=NULL)
                {
                    Length=pStr1-pStr-1;
                    memcpy(StrValue,pStr+1,Length);
                    StrValue[Length]='\0';
                }
                else
                {
                    strcpy(StrValue,pStr+1);
                }
                return Length;
            }
            Index1++;
        }
        else
        {
            return 0;
        }
        pStr++;
    }
}
//***********************************SendBDTXSQ_Uart3********************************//
//实现对TXSQ的协议组包，其中0-4位为帧头，5，6位为协议长度，7,8,9位为用户地址，10为信息类别，11,12,13位用户地址
//14，15为短消息长度，16为0，17之后为短消息内容
//***********************************************************************************//
void SendBDTXSQ_Uart3(char *buf_rns,char *buf_rds)//$TXSQ
{
	int k;
	int len = 0,reg = 0,temp = 0;
	char buf_time_addr[100] = {0};
	char buf_snd[1024] = {0};
	char test[]={0x30,0x37,0x33,0x39,0x33,0x37,0x2e,0x30,0x30,0x30,0x2c,0x33,0x39,0x34,0x39,0x2e,0x34,0x34,0x31,0x34,0x2c,0x4e,0x2c,0x31,0x31,0x36,0x31,0x38,0x2e,0x33,0x34,0x30,0x30,0x2c,0x45};//gns_debug
	FILE *fp;
	memset(buf_snd,0,1024);
		
	/*for(k=0;k<strlen(buf_rns);k++)
	{
		if((buf_rns[k] == 'E') || (buf_rns[k] == 'W')){
			len = (k-6);
			break;
		}
	}
	printf("sizeof(test):%d\n",len);
	buf_time_addr[0] = 0xA4;
	memcpy(&buf_time_addr[1],&buf_rns[7],len);*/
	
	memset(buf_time_addr, 0, 100);
	buf_time_addr[0] = 0xA4;	
	char Col[12] = { 0 };	
    char Valid[12] = { 0 };	
	memset(Valid, 0, 12);
	GetSection((char*)buf_rns,",",1,Valid);
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",0,Col);

    if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_time_addr[4] = (Col[0] - '0')*10 + (Col[1] - '0');
		buf_time_addr[5] = (Col[2] - '0')*10 + (Col[3] - '0');
		buf_time_addr[6] = (Col[4] - '0')*10 + (Col[5] - '0');
	}
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",8,Col);
	if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_time_addr[1] = (Col[4] - '0')*10 + (Col[5] - '0');
		buf_time_addr[2] = (Col[2] - '0')*10 + (Col[3] - '0');	
		buf_time_addr[3] = (Col[0] - '0')*10 + (Col[1] - '0');
	}

	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",2,Col);
	double lat=(Col[0] - '0')*10 + (Col[1] - '0')+(atof(&Col[2])/60.0);	
	if (strlen(Col) > 0)
	{		
		long t = lat*100000;
		buf_time_addr[10] = (t & 0xFF000000) >> 24;
		buf_time_addr[9] = (t & 0x00FF0000) >> 16;
		buf_time_addr[8] = (t & 0x0000FF00) >> 8;
		buf_time_addr[7] = (t & 0x000000FF);	
		
		printf("---------lat:%d\n",t);
	}
	
	/*buf_time_addr[10] = 0xa1;
	buf_time_addr[9] = 0xa1;
	buf_time_addr[8] = 0xa1;
	buf_time_addr[7] = 0xa1;*/
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",4,Col);
	double lon=(Col[0] - '0')*100 + (Col[1] - '0')*10 + (Col[2] - '0') + (atof(&Col[3])/60.0);
	if (strlen(Col) > 0)
	{		
		long t = lon*100000;
		buf_time_addr[14] = (t & 0xFF000000) >> 24;
		buf_time_addr[13] = (t & 0x00FF0000) >> 16;
		buf_time_addr[12] = (t & 0x0000FF00) >> 8;
		buf_time_addr[11] = (t & 0x000000FF);
		printf("---------lon:%d\n",t);
	}			
	
	/*	float UnderWaterlat = (buf_time_addr[10]*256*256*256+buf_time_addr[9]*256*256+buf_time_addr[8]*256+buf_time_addr[7])/100000.0;
				float UnderWaterlon = (buf_time_addr[14]*256*256*256+buf_time_addr[13]*256*256+buf_time_addr[12]*256+buf_time_addr[11])/100000.0;
				printf("---UnderWaterlon------lon :%f  %f\n",UnderWaterlat, UnderWaterlon);
	buf_time_addr[14] = 0xa2;
	buf_time_addr[13] = 0xa2;
	buf_time_addr[12] = 0xa2;
	buf_time_addr[11] = 0xa2;*/
	len = 15;
	//len += 1;//+0xA4
	
	buf_snd[0] = TXSQ[0];
	buf_snd[1] = TXSQ[1];
	buf_snd[2] = TXSQ[2];
	buf_snd[3] = TXSQ[3];
	buf_snd[4] = TXSQ[4];	
	//normal version
	
	buf_snd[11] = rds_rcv_buf[0];//buf_rds[7];
	buf_snd[12] = rds_rcv_buf[1];//buf_rds[8];用户地址 
	buf_snd[13] = rds_rcv_buf[2];//buf_rds[9];//id addr of rcv
	
	buf_snd[10] = 0x46;//type:0x44   0x46//信息类别
	
	buf_snd[7] = buf_rds[7];//0x03用户地址 

	buf_snd[8] = buf_rds[8];//0xc2
	buf_snd[9] = buf_rds[9];//0x6f
	
	//printf("len111:%d\n",len);	
	temp = (len*8);
	//printf("temp:%d\n",temp);
	buf_snd[14] = (((temp) >> 8) & 0xff);//电文长度
	buf_snd[15] = ((temp) & 0xff);//length is bite
	//printf("1415==============%x       %x\n",buf_snd[14],buf_snd[15]);
	buf_snd[16] = 0;//apply
		
	//otp_encrypt_buf(buf_time_addr,strlen(buf_time_addr),&buf_snd[17],myencrypt->key,strlen(myencrypt->key),0);
	//otp_encrypt_buf(buf_time_addr,15,&buf_snd[17],myencrypt->key,strlen(myencrypt->key),0);
	memcpy(&buf_snd[17],buf_time_addr,15);
		
	len = (16+(len)) + 2;//2:1(jiaoyanhe)+1(buf_snd[0])
	buf_snd[5] = (len >> 8) & 0xff;
	buf_snd[6] = len & 0xff;
		
	buf_snd[len-1] = 0;
	for(k=0;k<len-1;k++)
	{
		buf_snd[len-1] ^= buf_snd[k];//jiaoyanhe
	}
		
	printf("start print TXSQ info:\n");
	print_buffer(buf_snd,len);
	printf("finish print TXSQ info!\n");	
	reg = send_message_to_uart(uart.fd_3,buf_snd,len);
	if(reg == -1)
	{
		printf("Send uart3 $TXSQ buf error!\n\r\n");
		write_log("send uart3 $TXSQ buf error!\n");
	}
	else
		printf("send uart3 $TXSQ ok\r\n");
	send_time_flag = 0;
}


//*********************check_ICJC***********************************//
//实现ICJC的协议，并通过send_message_to_uart函数实现发送
//*******************************************************************
char *check_ICJC(void)//远程查询SIM卡号
{
	int reg = -1;	
	char buf[] = {0x24,0x49,0x43,0x4A,0x43,0x00,0x0C,0x00,0x00,0x00,0x00,0x2B};//$ICJC
	rds_buf[7] = 0;
	rds_buf[8] = 0;
	rds_buf[9] = 0;
	reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));
	if(reg == -1)
		write_log("send  ICJC buf error!\r\n");
	memset(ICJC_buf,0,3);
	memcpy(ICJC_buf,&rds_buf[7],3);
	return &ICJC_buf[0];
}


//****************************ThdBDICJC_Uart3****************************//
//ThdBDICJC_Uart3实现北斗SIM卡检测功能，通过定期查询SIM卡，检测北斗模块是否正常，
//如果检测异常，则通过GPIO对北斗模块进行断电复位。
//*************************************************************************//
/*int ThdBDICJC_Uart3(void)
{
	int reg = -1;
	char buf[] = {0x24,0x49,0x43,0x4A,0x43,0x00,0x0C,0x00,0x00,0x00,0x00,0x2B};//$ICJC
	int times = 0;
	
	#if 1
	while(1)
	{
		if(check_ICJC_flag == 2)
		{
			while(1)
			{
				if((rds_buf[7] == 0) && (rds_buf[8] == 0) && (rds_buf[9] == 0)){
					
					if(times == 3)
					{//reset beidou
						printf("beidou times 3\n");
						system("gpio-test out 0 0");
						system("gpio-test out 3 0");
						sleep(5);
						system("gpio-test out 0 1");
						system("gpio-test out 3 1");
						times = 0;
					}
					times += 1;
				}
				else
				{
					check_ICJC_flag = 0;
					break;
				}
				reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));
				usleep(100);
				//printf("buf_ic_card[7]:%x,%x,%x\n",rds_buf[7],rds_buf[8],rds_buf[9]);
				if(reg == -1)
				{
					printf("send buf error!\n");
				}					
			}
		}
		else if(check_ICJC_flag == 1)
		{
			reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));
			if(reg == -1)
			{
				printf("send buf error!\n");
			}
			check_ICJC_flag = 0;
		}
		usleep(200);
	}	
	#endif
	return 0;
}*/

void SendICJC_Uart3(void)
{
	int reg = -1;
	char buf[] = {0x24,0x49,0x43,0x4A,0x43,0x00,0x0C,0x00,0x00,0x00,0x00,0x2B};//$ICJC
	if(check_ICJC_flag == 2)
	{
		if((rds_buf[7] == 0) && (rds_buf[8] == 0) && (rds_buf[9] == 0))
		{					
		   if(resetBDcnt == 3)
			{//reset beidou
				printf("beidou times 3\n");
				system("gpio-test out 0 0");
				system("gpio-test out 3 0");
				sleep(5);
				system("gpio-test out 0 1");
				system("gpio-test out 3 1");
				resetBDcnt = 0;
			}
			resetBDcnt += 1;
		}
		else
		{
			check_ICJC_flag = 0;			
		}
		reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));		
		if(reg == -1)
		{
			printf("send buf error!\n");
		}	
		
	}
	else if(check_ICJC_flag == 1)
	{
		reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));
		if(reg == -1)
		{
			printf("send buf error!\n");
		}
		check_ICJC_flag = 0;
	}		
	return;
}

//****************************assem_XTZJ_packet****************************//
//assem_XTZJ_packet实现系统自检的协议组包
//*************************************************************************//
void assem_XTZJ_packet()
{
	int k = 0,len,reg;
	char buf_snd[1024] = {0};
	len = 13;
	
	buf_snd[0] = XTZJ[0];
	buf_snd[1] = XTZJ[1];
	buf_snd[2] = XTZJ[2];
	buf_snd[3] = XTZJ[3];
	buf_snd[4] = XTZJ[4];
	
	buf_snd[5] = 0;
	buf_snd[6] = len;//len
	
	buf_snd[7] = rds_buf[7];
	buf_snd[8] = rds_buf[8];
	buf_snd[9] = rds_buf[9];//usr addr
	
	buf_snd[10] = 0;
	buf_snd[11] = 0;//zijian frequency
	
	buf_snd[len-1] = 0;
	for(k=0;k<len-1;k++)
	{
		buf_snd[len-1] ^= buf_snd[k];//jiaoyanhe
	}
	
	reg = send_message_to_uart(uart.fd_3,buf_snd,len);	
	if(reg == -1)
		printf("send uart3 $XTZJ buf error!\n");
	else
		printf("send uart3 $XTZJ ok\n");
	
}
//****************************assem_SJSC_packet****************************//
//assem_SJSC_packet实现时间输出的协议组包
//*************************************************************************//
void assem_SJSC_packet()
{
	int k = 0,len,reg;
	char buf_snd[1024] = {0};
	len = 13;
	
	buf_snd[0] = SJSC[0];
	buf_snd[1] = SJSC[1];
	buf_snd[2] = SJSC[2];
	buf_snd[3] = SJSC[3];
	buf_snd[4] = SJSC[4];
	
	buf_snd[5] = 0;
	buf_snd[6] = len;//len
	
	buf_snd[7] = rds_buf[7];
	buf_snd[8] = rds_buf[8];
	buf_snd[9] = rds_buf[9];//usr addr
	
	buf_snd[10] = 0;
	buf_snd[11] = 0;//output frequency
	
	buf_snd[len-1] = 0;
	for(k=0;k<len-1;k++)
	{
		buf_snd[len-1] ^= buf_snd[k];//jiaoyanhe
	}
	
	reg = send_message_to_uart(uart.fd_3,buf_snd,len);	
	if(reg == -1)
		printf("send uart3 $SJSC buf error!\n");
	else
		printf("send uart3 $SJSC ok\n");	
}

//****************************assem_BBSQ_packet****************************//
//assem_BBSQ_packet实现版本读取的协议组包
//*************************************************************************//
void assem_BBSQ_packet()
{
	int k = 0,len,reg;
	char buf_snd[1024] = {0};
	len = 11;
	
	buf_snd[0] = BBSQ[0];
	buf_snd[1] = BBSQ[1];
	buf_snd[2] = BBSQ[2];
	buf_snd[3] = BBSQ[3];
	buf_snd[4] = BBSQ[4];
	
	buf_snd[5] = 0;
	buf_snd[6] = len;//len
	
	buf_snd[7] = rds_buf[7];
	buf_snd[8] = rds_buf[8];
	buf_snd[9] = rds_buf[9];//usr addr
		
	buf_snd[len-1] = 0;
	for(k=0;k<len-1;k++)
	{
		buf_snd[len-1] ^= buf_snd[k];//jiaoyanhe
	}
	//printf("buf:%s\n",buf_snd);
	reg = send_message_to_uart(uart.fd_3,buf_snd,len);
	
	if(reg == -1)
		printf("send uart3 $BBDQ buf error!\n");
	else
		printf("send uart3 $BBDQ ok\n");	
	//send_time_flag = 0;
}


//****************************ThdBDTXSQ****************************//
//ThdBDTXSQ实现北斗一代短报文的通信申请，通过全局变量snd_beidou_addr_info
//对数据发送进行控制。短报文通过SendBDTXSQ_Uart3进行组包发送
//*****************************************************************//
/*void ThdBDTXSQ(void)
{
	failure_times = 0;
	success_times = 0;
	int send_times = 0;
	while(1)
	{
		usleep(200);
		if(send_time_flag == 1)
			continue;
		
		sleep(80);//120	
		send_time_flag = 1;
		//printf("snd_beidou_addr_info=%d\n",snd_beidou_addr_info);
		if(snd_beidou_addr_info == 0)
		{
			printf("send TXSQ\n");
			SendBDTXSQ_Uart3(rns_buf,rds_buf);
			send_times ++;
		}
		else
		{
			send_time_flag = 0;
			printf("not send TXSQ\n");
		}						
		printf("\033[5;31m TXSQ send_times:%d: \033[0m \n",send_times);
	}
}*/

//****************************SendGpsToBB_Uart5****************************//
//SendGpsToBB_Uart5实现与AUV控制微机的通信协议（GPS信息部分）
//*************************************************************************//
void SendGpsToBB_Udp8010(char *buf_rns)
{
	int k = 0,len = 0,reg = 0;
	char buf_snd[37] = {0};
	int wd_int,jd_int;
	double wd,jd;
	unsigned int temp;
	float speed_f;
	int speed;
	buf_snd[0] = 0x7E;//包头
	buf_snd[1] = 12;//长度

	buf_snd[2] = 'G';//指令类型 0x47

	char Col[12] = { 0 };	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",2,Col);
	double lat=(Col[0] - '0')*10 + (Col[1] - '0')+(atof(&Col[2])/60.0);	
	if (strlen(Col) > 0)
	{		
		long t = lat*100000;
		buf_snd[6] = (t & 0xFF000000) >> 24;
		buf_snd[5] = (t & 0x00FF0000) >> 16;
		buf_snd[4] = (t & 0x0000FF00) >> 8;
		buf_snd[3] = (t & 0x000000FF);		
	}
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",4,Col);
	double lon=(Col[0] - '0')*100 + (Col[1] - '0')*10 + (Col[2] - '0') + (atof(&Col[3])/60.0);
	if (strlen(Col) > 0)
	{
		long t = lon*100000;
		buf_snd[10] = (t & 0xFF000000) >> 24;
		buf_snd[9] = (t & 0x00FF0000) >> 16;
		buf_snd[8] = (t & 0x0000FF00) >> 8;
		buf_snd[7] = (t & 0x000000FF);
	}
	//printf("---------------------------wd:%f %f\n",lat, lon);
	buf_snd[11] = buf_rns[47];
	buf_snd[12] = 0;
	for(k=1;k<12;k++){
		buf_snd[12] += buf_snd[k];//jiaoyanhe
	}

	buf_snd[12] &= 0xff;
	//reg = send_message_to_uart(uart.fd_5,buf_snd,13);
    //UdpMessageSend(g_kzwj_udpsock, buf_snd, 13, g_kzwj_serv_addr);

    sendto(g_kzwj_udpsock,buf_snd,13,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
	//if(reg == -1)write_log("send uart5 buf error!\r\n");
	//else
	//printf("send uart5 ok\n");
}


int timeGet(void) 
{
    time_t current_time;
    struct tm *time_info;
    char time_string[30];

    // 获取当前系统时间
    current_time = time(NULL);

    // 将时间转换为本地时间
    time_info = localtime(&current_time);

    // 格式化时间字符串
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", time_info);  
    
    printf("currentime:%s\n", time_string);
    
    return 0;
}


//****************************SendTimToBB_Uart5****************************//
//SendTimToBB_Uart5实现与AUV控制微机的通信协议（时间信息部分）
//*************************************************************************//
void SendTimToBB_Udp8010(char *buf_rns)
{
	char buf_snd[37] = {0};
	int  i = 0,reg    = 0;
    char date_str[8];
    char time_str[6];
    char setsysTime[30];
    
	buf_snd[0]       = 0x7E;  //包头
	buf_snd[1] = 9;           //长度
	
	buf_snd[2] = 'T';
	
	char Valid[12] = { 0 };	
	memset(Valid, 0, 12);
	GetSection((char*)buf_rns,",",1,Valid);
	
    char Col[12] = { 0 };	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",0,Col);

    if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_snd[6] = (Col[0] - '0')*10 + (Col[1] - '0');
		buf_snd[7] = (Col[2] - '0')*10 + (Col[3] - '0');
		buf_snd[8] = (Col[4] - '0')*10 + (Col[5] - '0');
	}
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",8,Col);
	if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_snd[3] = (Col[4] - '0')*10 + (Col[5] - '0');
		buf_snd[4] = (Col[2] - '0')*10 + (Col[3] - '0');	
		buf_snd[5] = (Col[0] - '0')*10 + (Col[1] - '0');
	}

    if((buf_snd[3] >= 23)&&(setsysTimeFlag == 1)){
        memset(date_str,0,10);
        memset(time_str,0,10);
        memset(setsysTime,0,30);
        sprintf(date_str,"20%d-%d-%d",buf_snd[3],buf_snd[4],buf_snd[5]);
        sprintf(time_str,"%d:%d:%d"  ,buf_snd[6]+8,buf_snd[7],buf_snd[8]);
        sprintf(setsysTime,"date -s %c%s %s%c",'"',date_str,time_str,'"');
        printf("Time:%s\r\n",setsysTime);
        system(setsysTime);
        setsysTimeFlag = 0;
    }else timeGet();

	buf_snd[9] = 0;//CRC
	for(i=1;i<9;i++)
        buf_snd[9] += buf_snd[i];
    sendto(g_kzwj_udpsock,buf_snd,10,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
	send_time_flag = 0;
}


//***********************************************************************************//
void SendLoraBD_Uart7(char *buf_rns,char *buf_rds)
{
	int  k;
	int  len = 0,reg = 0,temp = 0;
	char orientation[2]={0};
	char buf_time_addr[100] = {0};
	char buf_snd[1024] = {0};
	memset(buf_snd,0,1024);
	/*****************************************/
	memset(buf_time_addr, 0, 100);
	//buf_time_addr[0] = 0xA4;	
	char Col[12] = { 0 };	
    char Valid[12] = { 0 };	
	memset(Valid, 0, 12);
	GetSection((char*)buf_rns,",",1,Valid);
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",0,Col);

    if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_time_addr[3] = (Col[0] - '0')*10 + (Col[1] - '0');
		buf_time_addr[4] = (Col[2] - '0')*10 + (Col[3] - '0');
		buf_time_addr[5] = (Col[4] - '0')*10 + (Col[5] - '0');
	}
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",8,Col);
	if (strlen(Col) > 0 && Valid[0] == 'A')
	{
		buf_time_addr[0] = (Col[4] - '0')*10 + (Col[5] - '0');
		buf_time_addr[1] = (Col[2] - '0')*10 + (Col[3] - '0');	
		buf_time_addr[2] = (Col[0] - '0')*10 + (Col[1] - '0');
	}

	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",2,Col);
	double lat=(Col[0] - '0')*10 + (Col[1] - '0')+(atof(&Col[2])/60.0);	
	if (strlen(Col) > 0)
	{		
		long t = lat*100000;
		buf_time_addr[9] = (t & 0xFF000000) >> 24;
		buf_time_addr[8] = (t & 0x00FF0000) >> 16;
		buf_time_addr[7] = (t & 0x0000FF00) >> 8;
		buf_time_addr[6] = (t & 0x000000FF);	
		
		printf("---lat:%d\n",t);
	}

	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",3,Col);
	if (strlen(Col) > 0)
	{		
		orientation[0] = Col[0];
		buf_time_addr[10] = Col[0];
		printf("---lat-orientation:%c\n",orientation[0]);
	}
	
	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",4,Col);
	double lon=(Col[0] - '0')*100 + (Col[1] - '0')*10 + (Col[2] - '0') + (atof(&Col[3])/60.0);
	if (strlen(Col) > 0)
	{		
		long t = lon*100000;
		buf_time_addr[14] = (t & 0xFF000000) >> 24;
		buf_time_addr[13] = (t & 0x00FF0000) >> 16;
		buf_time_addr[12] = (t & 0x0000FF00) >> 8;
		buf_time_addr[11] = (t & 0x000000FF);
		printf("---lon:%d\n",t);
	}			

	memset(Col, 0, 12);
	GetSection((char*)buf_rns,",",5,Col);
	if (strlen(Col) > 0)
	{		
		orientation[1] = Col[0];
		buf_time_addr[15] = Col[0];
		printf("---lon-orientation:%c\n",orientation[1]);
	}
	len = 16;
	
	buf_snd[0] = 0x7E;//包头
	buf_snd[1] = 0x47;//G
	buf_snd[2] = 0x54;//T	
	buf_snd[3] = 18;  //数据长度
	memcpy(&buf_snd[4],buf_time_addr,16);
	len += 5;
	
	buf_snd[len-1] = 0;
	for(k=0;k<len-1;k++)
	{
		buf_snd[len-1] ^= buf_snd[k];//异或运算
	}
	
	print_buffer(buf_snd,len);
    
    system("echo 0 > /sys/class/leds/led1/brightness");//电平拉高发送uart7
	reg = send_message_to_uart(uart.fd_7,buf_snd,len);
    usleep(1000*500);
    system("echo 1 > /sys/class/leds/led1/brightness");//电平拉低等待接收uart7
	if(reg == -1)
	{
		printf("Send uart7 $Lora buf error!\n\r\n");
		write_log("send uart7 $Lora buf error!\n");
	}
	else
		printf("Respond LoraBD Ask_cmd ok!!!\n");
	send_time_flag = 0;
}

void ThdLora_Uart7(void)
{
	int Ret=0,Reg=0;
	char buf_recv[1024]={0},crc_ok;
	while(1)
	{
		memset(buf_recv, 0, 1024);
		pthread_mutex_lock(&mut);
		Ret = get_message_from_uart(uart.fd_7,buf_recv,RNS_LEN); //length is 1024
		pthread_mutex_unlock(&mut); 
		if (Ret == -1)	
		{ 
			printf("read fd_7 error.\n");
		}
		else if(Ret > 1)
		{			
            printf("Uart7 recv ask cmd:\n");
            print_buffer(buf_recv, Ret);
		    if((buf_recv[0]==0xAA)&&(buf_recv[1]==0x55)&&(buf_recv[2]==0x03))
		   	{
		   	    crc_ok = check_crc_hex(buf_recv,Ret);
				if(crc_ok == 0)
				{
				    usleep(1000*500);
					SendLoraBD_Uart7(rns_buf,rds_buf);
				}
		   	}
		}
	usleep(1000*100);
	}
}

//*************************value_card_info*****************************//
//value_card_info实现SIM卡信息读取。
//SIM卡信息保存在card_info文件中，通过fopen与fread进行内容读取
void value_card_info(void)
{
	/*FILE *fp;
	char tmp[7];
	memset(tmp,0,sizeof(tmp));
	fp = fopen("/opt/card_info","rt+");
	if(fp == NULL)
	{
		write_log("/opt/card_info  not exist\r\n");
		
	}
	else
	{
		fread(tmp,1,6,fp);
		memcpy(&rds_buf[7],tmp,3);
		memcpy(rds_rcv_buf,&tmp[3],3);
		printf("rds_buf:%x %x %x,rds_rcv_buf:%x %x %x\n",rds_buf[7],rds_buf[8],rds_buf[9],rds_rcv_buf[0],rds_rcv_buf[1],rds_rcv_buf[2]);
		fclose(fp);
	}*/
	
	FILE *fp;
	fp = fopen("/opt/BDServerID.txt","rt+");
	if(fp == NULL)
	{
		printf("C:/BDServerID  not exist\n");		
	}
	else
	{
		fscanf(fp,"%02X%02X%02X", &rds_rcv_buf[0], &rds_rcv_buf[1], &rds_rcv_buf[2]);	
		rds_buf[7]=0x03;
		rds_buf[8]=0xc2;
		rds_buf[9]=0x6F;
		printf("rds_buf:%x %x %x,rds_rcv_buf:%x %x %x\n",rds_buf[7],rds_buf[8],rds_buf[9],rds_rcv_buf[0],rds_rcv_buf[1],rds_rcv_buf[2]);
		fclose(fp);
	}
	
}
//*************************save_card_info*****************************//
//save_card_info实现SIM卡信息保存。
//SIM卡信息保存在card_info文件中，通过fopen与fwrite进行内容读取
void save_card_info(void)
{
	/*FILE *fp;
	char tmp[7];
	memset(tmp,0,sizeof(tmp));
	fp = fopen("/opt/card_info","w+");
	if(fp == NULL)
	{
		write_log("/opt/card_info  not exist\r\n");		
	}
	else
	{
		memcpy(tmp,&rds_buf[7],3);
		memcpy(&tmp[3],rds_rcv_buf,3);
		printf("rds_buf:%x %x %x,rds_rcv_buf:%x %x %x\n",tmp[0],tmp[1],tmp[2],tmp[3],tmp[4],tmp[5]);
		fwrite(tmp,6,1,fp);
		fclose(fp);
	}*/
	
	FILE *fp;
	fp = fopen("/opt/BDServerID.txt","wt+");
	if(fp == NULL)
	{
		printf("C:/card_info  not exist\n");		
	}
	else
	{	
		fprintf(fp,"%02X%02X%02X",rds_rcv_buf[0],rds_rcv_buf[1],rds_rcv_buf[2]);
		fclose(fp);
	}
}

void sig_handler(int signum)
{ 
	printf("-----sig_handler!\r\n");
	write_log("-----sig_handler!\r\n");
	//
	
	//pthread_cancel((pthread_t *)&(ThdConn800_t));
	exit8001 = 1;
	exit8080 = 1;
	
	usleep(300);
/*
	((pthread_t *)&(ThdConn8080_t));
	if(pthread_cancel((pthread_t *)&(ThdConn8080_t)) == 0)
	{
		printf("-----sig_handler           8080  ok\r\n");
		exit8080ok = 1;
	}
	else
	{
		printf("-----sig_handler           8080  false!\r\n");
		exit8080ok = 1;
		
	}
	pthread_cancel((pthread_t *)&(ThdConn800_t));
	*/
    return;
}

int GetFileLine(char *pInFileName, char *pOutLine, int OutMaxSize, int No)
{
	char *line = NULL;
	int    Num = 0;
	size_t len = 0;
	size_t ret;
	
	memset(pOutLine, 0, OutMaxSize);
	FILE *fp = fopen(pInFileName, "r");
	if (fp != NULL)
	{				
		while ((ret = getline(&line, &len, fp)) != -1)
		{		
			if (Num==No)
			{
				int i=0;
				int i0A = line[ret];
				for (i=ret; i>0; i--)
				{
					if (line[i]==0x0A)
					{
						i0A = i;
					}
					if(line[i]==' ')
					{				
						memcpy(pOutLine,line+i+1,i0A-i-1);	
						break;												
					}	
				}	
				
				if (i==0)
				{
					memcpy(pOutLine,line,ret>OutMaxSize?OutMaxSize:(ret-1));	
				}
				if (line)
				{
					free(line);
				}	
				
				return 1;
			}			
			Num ++;
		}				
	}
	else
	{
		printf("open pInFileName false\n");
		write_log("open pInFileName false\r\n");
		return 0;
	}

	if (line)
	{
		free(line);
	}	
	return 0;		
}

void ClearTmpFile(void)
{
	printf("ClearTmpFile------------\n");
	system("rm -rf /root/LFile/LEnc");
	usleep(200);	
    if(access("/root/LFile/LEnc",0)==0)
	{
		system("rm -rf /root/LFile/LEnc");
		usleep(500);
		if(access("/root/LFile/LEnc",0)==0)
		{
			write_log("rm -rf /root/LFile/LEnc fail ++++++++++++\r\n");
			system("rm -rf /root/LFile/LEnc");
			usleep(500);
		}
	}
	printf("ClearTmpFile------------1\n");
	system("rm -rf /root/LFile/LDec");
	usleep(200);	
    if(access("/root/LFile/LDec",0)==0)
	{
		system("rm -rf /root/LFile/LDec");
		usleep(500);
		if(access("/root/LFile/LDec",0)==0)
		{
			write_log("rm -rf /root/LFile/LEnc/LDec fail ++++++++++++\r\n");
			system("rm -rf /root/LFile/LDec");
			usleep(500);
		}
	}
    printf("ClearTmpFile------------2\n");
	system("mkdir /root/LFile/LEnc");
	usleep(200);
	system("mkdir /root/LFile/LDec");
	usleep(200);		
}

void ThdWGet(void)
{
	unsigned char src_name[500] = { 0 };
	unsigned char dec_name[500] = { 0 };
	unsigned char enc_name[500] = { 0 };	
	char LineBuf[256]={0};
	char CmdBuff[500]={0};
	int  LineCnt=0;
    char ReadBuf[200] = { 0 };
    char decrypt_buf[200];
    char encrypt_buf[200];
    int  len = 0;		
	
	system(ENCFILELIST);		
    while (GetFileLine(FILELIST, LineBuf, 256, LineCnt))
	{
		LineCnt++;				
		memset(src_name, 0, 500);
		memset(dec_name, 0, 500);
		sprintf(src_name,"/root/LFile/LEnc/%s",LineBuf);
		sprintf(dec_name,"/root/LFile/LDec/%s",LineBuf);						
		otp_encrypt(src_name,dec_name,myencrypt->key,strlen(myencrypt->key),1);

		memset(CmdBuff,0,500);
		//sprintf(CmdBuff,"ftpput -u ftpserver -p ftpserver %s ata0a/%s %s",KZWJIP,LineBuf,dec_name);
		//sprintf(CmdBuff,"ftpput -u 15993 -p 2271221 %s /localuser/ata0a/%s %s",KZWJIP,LineBuf,dec_name);//tcp方式put至控制微机，不用修改
		#ifdef debugMode
        sprintf(CmdBuff,"ftpput -u 15993 -p 2271221 %s /localuser/home/target/AUV_LINUX/%s %s",KZWJIP,LineBuf,dec_name);//tcp方式put至控制微机
        #else
        sprintf(CmdBuff,"ftpput -u target -p password %s /home/target/AUV_LINUX/%s %s",KZWJIP,LineBuf,dec_name);//tcp方式put至控制微机，不用修改（天和控制微机）
        #endif
        //"ftpput -u 15993 -p 2271221 192.168.1.240 ata0a/testcase.txt /root/LFile/LDec/testcasex.txt"
		//ftpget -u ftpserver -p ftpserver 192.168.0.3 /root/LFile/LDec/11.txt /ata0b/11.txt
		//./wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@192.168.0.3/ata0b/* -c
		system(CmdBuff);
		printf("%s\n", CmdBuff); 
		usleep(200);
	}					
}
//***********************************ThdConn8080**************************************//
//ThdConn8080通过TCP协议实现与192.168.168.240的无线socket连接，并挂在//192.168.0.55/hf-data-share文件夹到/media/mmcblk0p1下
//当检测到“start1”消息时，通过ls命令获取当前文件目录，并回传到240的文件夹中。
//当检测到“start2”消息时，主动获取192.168.168.240/media/mmcblk0p1/mission目录下的*.txt1
//当检测到“start3”消息时，对获取的协议数据进行解析。
//通过decrypt_buf[0]和[2]进行判断指令操作。包括设备上下电、状态查询，SIM卡查询和设置，电台指令与秘钥设置。
//*************************************************************************************************//
/*void ThdConn8080(void)
{
	char *server_ip = SERVERIP;//"192.168.168.240";
	int  server_port = 8010;
	int clnt_sock = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
	if(clnt_sock == -1)
	{		
		printf("socket8080 open fail!\n");	
		write_log("socket8080 open fail\r\n");		
		return;
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	serv_addr.sin_port = htons(server_port);
	while(1)
	{
		int ret = connect(clnt_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
		if(ret == 0)//建立链接成功
		{
			//while()
			//{
			//	printf("connect ok++++++\n");
		//	}				
		}
		else if(ret < 0 && errno == EINPROGRESS)  // 等待连接完成，errno == EINPROGRESS表示正在建立链接
		{
			fd_set set;
			FD_ZERO(&set);
			FD_SET(clnt_sock,&set);  //相反的是FD_CLR(_sock_fd,&set)
			time_t = 10;          //(超时时间设置为10毫秒)
			struct timeval timeo;
			timeo.tv_sec = timeout / 1000; 
			timeo.tv_usec = (timeout % 1000) * 1000;
			int retval = select(clnt_sock + 1, NULL, &set, NULL, &timeo);           //事件监听
			if(retval < 0)   
			{
				//建立链接错误close(clnt_sock)
				printf("retval < 0++++\n");
				
			}
			else if(retval == 0) // 超时
			{
					//超时链接没有建立close(clnt_sock)
			}
		 //将检测到clnt_sock读事件或写时间，并不能说明connect成功
			if(FD_ISSET(clnt_sock,&set))
			{
			   int error = 0;
			   socklen_t len = sizeof(error);
			   if(getsockopt(clnt_sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
			   {
					  //建立简介失败close(clnt_sock)
			   }
			   if(error != 0) // 失败
			   {
					  //建立链接失败close(clnt_sock)
			   }
			   else
				{
					  //建立链接成功
				}
			}

		}
		else
		{
		  //出现错误 close(_sock_fd)
		}
	}
}*/

//void ThdConn8080(void)
//{
//	char decrypt_buf[200];
//	char *server_ip = SERVERIP;//"192.168.168.240";
//	int  server_port = 8010;
//	
//	int  len = 0;
//	int  num = 0;
//	int  i   = 0;
//
//	char snd_diantai_buf[50];
//	unsigned char src_name[500] = { 0 };
//	unsigned char dec_name[500] = { 0 };
//	unsigned char enc_name[500] = { 0 };	
//	char LineBuf[256]={0};
//	int  LineCnt=0;
//	char CmdBuf[500]={0};
//	char ReadBuf[200] = { 0 };
//    /**********************************************************************************/
//    int sock = socket(AF_INET, SOCK_DGRAM, 0);
//    if (sock == -1)
//    {
//        printf("socket8001 open fail!\n");
//        write_log("socket8001 open fail\r\n");
//        return;
//    }
//	/**********************************************/
//    //local
//    struct sockaddr_in local_addr;
//    //memset(&local_addr,0,sizeof(local_addr));
//    local_addr.sin_family = AF_INET;
//    local_addr.sin_addr.s_addr = inet_addr("192.168.0.12");
//    local_addr.sin_port = htons(8010);
//	/**********************************************/
//    //server
//    struct sockaddr_in serv_addr;
//    //memset(&serv_addr,0,sizeof(serv_addr));
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = inet_addr("192.168.0.100");
//    serv_addr.sin_port = htons(8001);
//	/**********************************************/
//    if(0 != bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)))
//    {
//        printf("bind failed ip:%s  --port=%d  \n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
//        printf("-------to:%s:%d---------\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
//        return;
//    }
//    /**********************************************************************************/
//	while(1)
//	{		
//		memset(ReadBuf,0,sizeof(ReadBuf));
//		memset(decrypt_buf,0,sizeof(decrypt_buf));
//		printf("ThdConn8080 is activing!+++++++++++++++++++\r\n");
//		signal(SIGPIPE,sig_handler);
//		//pthread_testcancel();
//		//len = read(clnt_sock, ReadBuf, sizeof(ReadBuf));
//        
//        struct sockaddr_in clnt_addr;
//        int clnt_addr_size = sizeof(struct sockaddr_in);
//        int len = recvfrom(sock, ReadBuf, sizeof(ReadBuf), 0, (struct sockaddr_in*)&clnt_addr, &clnt_addr_size);
//        print_buffer_asii(ReadBuf, len);
//        
//		//pthread_testcancel();
//		printf("ThdConn8080 is activing!-------------------\r\n");
//		if (exit8080)
//		{
//			break;
//		}
//		if(0 >= len)
//		{
//			sleep(10);
//			continue;
//		}
//		
//		memset(src_name, 0, 500);
//		memset(enc_name, 0, 500);
//		memset(dec_name, 0, 500);
//
//		if((len > 5) && (ReadBuf[0] == 's') && (ReadBuf[1] == 't') &&(ReadBuf[2] == 'a') &&(ReadBuf[3] == 'r') &&(ReadBuf[4] == 't'))//fuzai
//		{
//			if (ReadBuf[5] == '1')
//			{
//				ClearTmpFile();
//				system("ls -l /media/mmcblk0p1/ADCdata/ | grep ^-> /root/LFile/LDec/duty.txt");//将前面路径下的文件以目录的形式存储到duty.txt中
//				otp_encrypt("/root/LFile/LDec/duty.txt","/root/LFile/LEnc/duty_enc.txt",myencrypt->key,strlen(myencrypt->key),0);
//
//                sendto(sock, "ok", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//				//write(clnt_sock,"ok",2);
//				
//                usleep(200);
//			}
//			else if (ReadBuf[5] == '2')
//			{	
//				ClearTmpFile();
//				pthread_create((pthread_t *)&(ThdWGet_t),NULL,(void*)(ThdWGet),(void*)(NULL));
//                
//				sendto(sock, "ok", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));    
//                //write(clnt_sock,"ok",2);
//			}
//			else if (ReadBuf[5] == '3')
//			{
//			    sendto(sock, "ok", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//				//write(clnt_sock,"ok",2);
//			
//				memset(ReadBuf,0,sizeof(ReadBuf));		
//				//len = read(clnt_sock, ReadBuf,sizeof(ReadBuf));
//                len = recvfrom(sock, ReadBuf, sizeof(ReadBuf), 0, (struct sockaddr_in*)&clnt_addr, &clnt_addr_size);
//				if(len == -1)
//				{
//					printf("read = -1\n");
//				}
//				if(ReadBuf[0] == 0x7E)//buf[i+3]:
//				{
//					decrypt_buf[0] = ReadBuf[0];
//					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
//					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);
//					printf("num11===%d\n",num);
//					if(0 == num)
//					{
//						printf("decrypt_buf[2] = %x\n",decrypt_buf[2]);
//						printf("connect_para->elec_info111111:%x\n",connect_para->elec_info);
//						if(decrypt_buf[2] == 0x01)//0x01:上电  0x02:断电  0x43('c'):查询--len:4
//						{
//							printf("shang dian\n");
//							//D0:电机 D1:舵机 D2:控制微机+深度传感器+姿态传感器 D3:路由器 D4:负载1 D5:负载2 D6:保留 D7:保留
//							printf("decrypt_buf[3] = %x\n",decrypt_buf[3]);
//							if((decrypt_buf[3] & 1) != 0)
//							{
//								connect_para->elec_info |= 1;
//								system("gpio-test out 6 0");
//							}							
//							printf("$$$$:%x\n",decrypt_buf[3] & 2);
//							if((decrypt_buf[3] & 2) != 0)
//							{
//								connect_para->elec_info |= 2;
//								system("gpio-test out 7 0");
//							}							
//							printf("decrypt_buf[3] = %x\n",decrypt_buf[3]);
//							if((decrypt_buf[3] & 4) != 0)
//							{
//								connect_para->elec_info |= 4;
//								system("gpio-test out 8 0");
//							}
//							if((decrypt_buf[3] & 8) != 0)
//							{
//								connect_para->elec_info |= 8;
//								system("gpio-test out 9 0");
//							}
//							if((decrypt_buf[3] & 0x10) != 0)
//							{
//								connect_para->elec_info |= 0x10;
//								system("gpio-test out 10 0");
//							}
//							if((decrypt_buf[3] & 0x20) != 0)
//							{
//								connect_para->elec_info |= 0x20;
//								system("gpio-test out 11 0");
//							}						
//						}
//						else if(decrypt_buf[2] == 0x02)
//						{
//							if((decrypt_buf[3] & 1) != 0)
//							{
//								connect_para->elec_info &= 0xfe;
//								system("gpio-test out 6 1");
//							}							
//							if((decrypt_buf[3] & 2) != 0)
//							{
//								connect_para->elec_info &= 0xfd;
//								system("gpio-test out 7 1");
//							}
//							if((decrypt_buf[3] & 4) != 0)
//							{
//								connect_para->elec_info &= 0xfb;
//								system("gpio-test out 8 1");
//							}
//							if((decrypt_buf[3] & 8) != 0)
//							{
//								connect_para->elec_info &= 0xf7;
//								system("gpio-test out 9 1");
//							}
//							if((decrypt_buf[3] & 0x10) != 0)
//							{
//								connect_para->elec_info &= 0xef;
//								system("gpio-test out 10 1");
//							}
//							if((decrypt_buf[3] & 0x20) != 0)
//							{
//								connect_para->elec_info &= 0xdf;
//								system("gpio-test out 11 1");
//							}
//						}
//						else if(decrypt_buf[2] == 0x43)
//						{//查询'C'
//							
//							memset(snd_240Sv,0,sizeof(snd_240Sv));
//							snd_240Sv[0] = decrypt_buf[0];
//							decrypt_buf[3] = connect_para->elec_info;
//							decrypt_buf[4] = 0;
//							for(i=1;i<4;i++)
//							{
//								decrypt_buf[4] += decrypt_buf[i];
//							}
//							decrypt_buf[4] &= 0xff;
//
//							otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
//							rcv_cmd_flag = 1;
//							continue;
//						}
//						else if(decrypt_buf[2] == 0x5A)
//						{//抛载检查('Z')--len:3{
//							printf("paozai check\n");	
//							//system("gpio-test out 12 1");
//						}
//						else
//						{
//							printf("bao tou:0x7E  command index:%x\n",decrypt_buf[2]);
//							send_message_to_uart(uart.fd_5,decrypt_buf,decrypt_buf[1]+1);
//							continue;
//						}
//						memset(snd_240Sv,0,sizeof(snd_240Sv));
//						snd_240Sv[0] = decrypt_buf[0];
//						otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
//						rcv_cmd_flag = 1;					
//					}
//				}
//				else if(ReadBuf[0] == 0x7F)
//				{
//					decrypt_buf[0] = ReadBuf[0];
//					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
//					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);
//
//					printf("num:%x\n",num);
//					if(0 == num)
//					{
//						for(i=0;i<len;i++)printf("decrypt_buf[%d]:%x\n",i,decrypt_buf[i]);
//						//printf("decrypt_buf:%x %x %x %x\n",decrypt_buf[0],decrypt_buf[1],decrypt_buf[2],decrypt_buf[3]);
//						send_message_to_uart(uart.fd_5,decrypt_buf,decrypt_buf[1]+1);
//					}
//				}
//				else if(ReadBuf[0] == 0x8A)
//				{
//					decrypt_buf[0] = ReadBuf[0];
//					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
//					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);
//					if(0 == num)
//					{
//						printf("decrypt_buf[2]:%x\n",decrypt_buf[2]);
//						print_buffer(decrypt_buf,strlen(decrypt_buf));
//						if(decrypt_buf[2] == 0x1)//SIM卡 查询/设置
//						{
//							if(decrypt_buf[3] == 0x1){
//								check_ICJC_flag = 1;
//								sleep(1);
//								memset(snd_240Sv,0,sizeof(snd_240Sv));
//								snd_240Sv[0] = decrypt_buf[0];
//								printf("Check SIM:%x %x %x\n",rds_buf[7],rds_buf[8],rds_buf[9]);
//								decrypt_buf[5] = rds_buf[7];
//								decrypt_buf[6] = rds_buf[8];
//								decrypt_buf[7] = rds_buf[9];
//								decrypt_buf[len-1] = 0;
//								for(i=1;i<len-1;i++)
//								{
//									decrypt_buf[len-1] += decrypt_buf[i];								
//								}
//								
//								decrypt_buf[len-1] &= 0xff;
//								otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
//								rcv_cmd_flag = 1;
//							}
//							else if(decrypt_buf[3] == 0x2)
//							{
//								if(decrypt_buf[4] == 0x01)
//								{
//									memcpy(&rds_buf[7],&decrypt_buf[5],3);
//									save_card_info();
//									
//								}
//								else if(decrypt_buf[4] == 0x02)
//								{
//									memset(rds_rcv_buf,0,sizeof(rds_rcv_buf));
//									memcpy(rds_rcv_buf,&decrypt_buf[5],3);
//									save_card_info();
//								}
//								memset(snd_240Sv,0,sizeof(snd_240Sv));
//								
//								snd_240Sv[0] = decrypt_buf[0];
//								decrypt_buf[5] = 0;
//								decrypt_buf[6] = 0;
//								decrypt_buf[7] = 1;
//								decrypt_buf[len-1] = 0;
//								for(i=1;i<len-1;i++)
//								{
//									decrypt_buf[len-1] += decrypt_buf[i];
//								}
//								
//								decrypt_buf[len-1] &= 0xff;
//								otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
//								rcv_cmd_flag = 1;
//							}
//						}
//						else if(decrypt_buf[2] == 0x2)//电台指令
//						{
//							if(decrypt_buf[3] == 0x01)
//							{
//								memset(snd_diantai_buf,0,sizeof(snd_diantai_buf));
//								memcpy(snd_diantai_buf,&decrypt_buf[4],decrypt_buf[1]-4);
//								snd_to_diantai(snd_diantai_buf);
//							}
//						}
//						else if(decrypt_buf[2] == 0x3)//密钥设置
//						{
//							memset(snd_240Sv,0,sizeof(snd_240Sv));			
//							snd_240Sv[0] = decrypt_buf[0];
//							otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
//							rcv_cmd_flag = 1;
//							
//							memset(myencrypt->key,0,sizeof(myencrypt->key));
//							memcpy(myencrypt->key,&decrypt_buf[4],decrypt_buf[1]-4);
//						}
//					}
//				}
//			}
//			else if (ReadBuf[5] == '5')
//			{
//				ClearTmpFile();	
//				usleep(200);
//				if(system(WGETPC104DUTY)==0)
//				{
//					printf("GetFile get fail\n");
//				}
//				else
//				{
//					printf("GetFile get succesful\n");
//				}
//				LineCnt=0;
//				while (GetFileLine("/root/LFile/LDec/duty.txt", LineBuf, 256, LineCnt))
//				{
//					LineCnt++;
//					memset(src_name, 0, 500);
//					memset(enc_name, 0, 500);
//					sprintf(src_name,"/media/mmcblk0p1/ADCdata/%s",LineBuf);//pc104
//					sprintf(enc_name,"/root/LFile/LEnc/%s",LineBuf);
//					otp_encrypt(src_name,enc_name,myencrypt->key,strlen(myencrypt->key),0);	
//					//printf("22222222222222222222222222-=media/mmcblk0p1/log %s\n",enc_name);
//				}				
//				//write(clnt_sock,"ok",2);
//				sendto(sock, "ok", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//			}
//			else if (ReadBuf[5] == '6')
//			{			
//				ClearTmpFile();
//				memset(ReadBuf,0,sizeof(ReadBuf));;
//				sprintf(ReadBuf,"/root/wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@%s/ata0b/* -c",KZWJIP);
//				//./wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@192.168.0.3/ata0b/* -c
//				system(ReadBuf);	
//					
//				system(DECFILELIST);
//				LineCnt=0;			
//				while (GetFileLine(FILELIST, LineBuf, 256, LineCnt))
//				{
//					LineCnt++;
//					memset(src_name, 0, 500);
//					memset(enc_name, 0, 500);
//					sprintf(src_name,"/root/LFile/LDec/%s",LineBuf);//控制微机
//					sprintf(enc_name,"/root/LFile/LEnc/%s",LineBuf);
//					otp_encrypt(src_name,enc_name,myencrypt->key,strlen(myencrypt->key),0);				
//					printf("---------------------=media/mmcblk0p1/log %s\n",src_name);	
//				}
//				//write(clnt_sock,"ok",2);
//				sendto(sock, "ok", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//			}
//		}
//	}
//	
//	printf("ThdConn8080 close\n");
//	close(sock);
//	exit8080ok = 1;
//}

void UdpConnErrUpt(void)
{
    if (1)
    {
		if (exit8080)
		{	
			return;
		}
		
		if(snd_beidou_addr_info == 1)
		{
			return;
		}
		
		if(rcv_cmd_flag == 0)
		{
			return;
		}
		
		signal(SIGPIPE,sig_handler);
        printf("port8001_sendStatus ");
        UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
        
        //sleep(1);//两次发送间隔
        
		//(Uwater_status.i800SendNum)++;
		//Thddebug();
		//write(clnt_sock,sendbuff,strlen(sendbuff));  //定时发送本机自检状态
		
		//printf("port8001 ");
		//UdpMessageSend(g_dt_udpsock, sendbuff, strlen(sendbuff), g_dt_serv_addr);//定时发送本机自检状态
		//printf("800-------------sendbuff:%s\n",sendbuff);
		rcv_cmd_flag = 0;
    }
}


void UdpStatusSend(void)
{
    while(1)
    {
		if (exit8080)
		{	
			return;
		}
		
		if(snd_beidou_addr_info == 1)
		{
			return;
		}
		
		if(rcv_cmd_flag == 0)
		{
			return;
		}
		
		signal(SIGPIPE,sig_handler);
        printf("port8001_sendStatus ");
        UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
        
        //sleep(1);//两次发送间隔
        
		//(Uwater_status.i800SendNum)++;
		//Thddebug();
		//write(clnt_sock,sendbuff,strlen(sendbuff));  //定时发送本机自检状态
		
		//printf("port8001 ");
		//UdpMessageSend(g_dt_udpsock, sendbuff, strlen(sendbuff), g_dt_serv_addr);//定时发送本机自检状态
		//printf("800-------------sendbuff:%s\n",sendbuff);
		rcv_cmd_flag = 0;
    }
}


//void UdpConnErrUpt(void)
//{
//    char decrypt_buf[200];
//    char encrypt_buf[200];
//    int  len = 0;
//    int  num = 0;
//    int  i = 0;
//    char snd_diantai_buf[50];
//    unsigned char src_name[500] = { 0 };
//    unsigned char dec_name[500] = { 0 };
//    unsigned char enc_name[500] = { 0 };
//    char LineBuf[256] = { 0 };
//    int  LineCnt = 0;
//    char CmdBuf[500] = { 0 };
//    char ReadBuf[200] = { 0 };
//	/**********************************************/
//    int sock = socket(AF_INET, SOCK_DGRAM, 0);
//    if (sock == -1)
//    {
//        printf("socket8001 open fail!\n");
//        write_log("socket8001 open fail\r\n");
//        return;
//    }
//	/**********************************************/
//    //local
//    struct sockaddr_in local_addr;
//    //memset(&local_addr,0,sizeof(local_addr));
//    local_addr.sin_family = AF_INET;
//    local_addr.sin_addr.s_addr = inet_addr(DTLOCALIP);
//    local_addr.sin_port = htons(8080);
//	/**********************************************/
//    //server
//    struct sockaddr_in serv_addr;
//    //memset(&serv_addr,0,sizeof(serv_addr));
//    serv_addr.sin_family = AF_INET;
//    serv_addr.sin_addr.s_addr = inet_addr(DTSERVERIP);
//    serv_addr.sin_port = htons(8080);
//	/**********************************************///赋值全局变量提供给其它函数调用
//	g_dt_serv_addr.sin_family = AF_INET;
//	g_dt_serv_addr.sin_addr.s_addr = inet_addr(DTSERVERIP);
//	g_dt_serv_addr.sin_port = htons(8080);
//	/**********************************************/
//    if(0 != bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)))
//    {
//        printf("bind failed ip:%s  --port=%d  \n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
//        printf("-------to:%s:%d---------\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
//        return;
//    }
//	/**********************************************/
//    //超时时间
//    struct timeval timeout;
//    timeout.tv_sec = 1;
//    timeout.tv_usec = 0;
//    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1)
//    {
//        printf("setsockopt SO_RCVTIMEO fail!\n");
//    }
//	/**********************************************/
//    g_dt_udpsock = sock;
//    g_rawsock = sock;
//    int sock_raw_fd = sock;
//    /**********************************************///set socket bufsize
//    int bufsize = 0;
//    socklen_t optlen = sizeof(bufsize);
//    if(setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, optlen) < 0)
//    {
//        printf("sock :set socket bufsize error\n");
//    }
//    if(setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, optlen) < 0)
//    {
//        printf("sock :set socket bufsize error\n");
//    }
//    if(setsockopt(sock_raw_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, optlen) < 0)
//    {
//        printf("rawsock :set socket bufsize error\n");
//    }
//    if(setsockopt(sock_raw_fd, SOL_SOCKET, SO_SNDBUF, &bufsize, optlen) < 0)
//    {
//        printf("rawsock :set socket bufsize error\n");
//    }
//	/**********************************************/
//    struct packet_flag flag;
//    memset(&flag, 0, sizeof(flag));
//    char sndbuf[1024] = { 0 };
//    memset(sndbuf, 0, 1024);
//	/**********************************************/
//    while (1)
//    {
//        usleep(200);
//		if (exit8080)
//		{	
//			break;
//		}
//		
//		if(snd_beidou_addr_info == 1)
//		{
//			continue;
//		}
//		
//		if(rcv_cmd_flag == 0)
//		{
//			continue;
//		}
//		
//		signal(SIGPIPE,sig_handler);
//        printf("port8001 ");
//        UdpMessageSend(sock, snd_240Sv, strlen(snd_240Sv), serv_addr); 
//        sleep(1);
//		//(Uwater_status.i800SendNum)++;
//		//Thddebug();
//		//write(clnt_sock,sendbuff,strlen(sendbuff));  //定时发送本机自检状态
//		printf("port8001 ");
//		UdpMessageSend(sock, sendbuff, strlen(sendbuff), serv_addr);
//		//printf("800-------------sendbuff:%s\n",sendbuff);
//		rcv_cmd_flag = 0;
//		usleep(200);
//    }
//    close(sock);
//    exit8080ok = 1;
//}


//***********************************ThdConn8001**************************************//
//ThdConn8001通过TCP协议实现与192.168.168.240的无线socket连接，并挂在//192.168.0.55/hf-data-share文件夹到/media/mmcblk0p1下
//当检测到“start1”消息时，通过ls命令获取当前文件目录，并回传到240的文件夹中。
//当检测到“start2”消息时，主动获取192.168.168.240/media/mmcblk0p1/mission目录下的*.txt1
//当检测到“start3”消息时，对获取的协议数据进行解析。
//通过decrypt_buf[0]和[2]进行判断指令操作。包括设备上下电、状态查询，SIM卡查询和设置，电台指令与秘钥设置。
//*************************************************************************************************//
void UdpConn8001(void)
{
    char decrypt_buf[200];
    char encrypt_buf[200];
    int  len = 0;
    int  num = 0;
    int  i = 0;
    char snd_diantai_buf[50];
    unsigned char src_name[500] = { 0 };
    unsigned char dec_name[500] = { 0 };
    unsigned char enc_name[500] = { 0 };
    char LineBuf[256] = { 0 };
    int  LineCnt = 0;
    char CmdBuf[500] = { 0 };
    char ReadBuf[200] = { 0 };
    int  flagx = 0,flags = 0;
	/**********************************************/
    //local
    struct sockaddr_in local_addr;
    //memset(&local_addr,0,sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = inet_addr(DTLOCALIP);
    local_addr.sin_port = htons(DTLOCALPORT);
	/**********************************************/
    //server
    struct sockaddr_in serv_addr;
    //memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(DTSERVERIP);
    serv_addr.sin_port = htons(DTSERVERPORT);
	/**********************************************///赋值全局变量提供给其它函数调用
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1)
    {
        printf("socket8001 open fail!\n");
        write_log("socket8001 open fail\r\n");
        return;
    }
    /**********************************************/
	g_dt_serv_addr.sin_family = AF_INET;
	g_dt_serv_addr.sin_addr.s_addr = inet_addr(DTSERVERIP);
	g_dt_serv_addr.sin_port = htons(DTSERVERPORT);
    g_dt_udpsock = sock;
	/**********************************************/
    // 设置阻塞超时时间为5秒
    struct timeval tv;
    tv.tv_sec  = 3;
    tv.tv_usec = 0;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
        perror("setsockopt");
    }
	/**********************************************/
    if(0 != bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)))
    {
        printf("bind failed ip:%s  --port=%d  \n", inet_ntoa(local_addr.sin_addr), ntohs(local_addr.sin_port));
        printf("-------to:%s:%d---------\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
        return;
    }
	/**********************************************/
    char sndbuf[1024] = { 0 };
    memset(sndbuf, 0, 1024);
    struct sockaddr_in clnt_addr;
    int clnt_addr_size = sizeof(struct sockaddr_in);
	/**********************************************/
    while (1)
    {
        memset(decrypt_buf, 0, sizeof(decrypt_buf));
        int len = recvfrom(sock, ReadBuf, sizeof(ReadBuf), 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);//超时就返回
        /**********************************************/
        if(timeCnt >= counter_1Hz)
		{
		    printf("uwaterclient udp8001 thread is running\n");
			counter_1Hz = timeCnt+1;
		}
        if (0>len)
        {
            continue;
        }else{
            if((ReadBuf[0] == 0x7E)||(ReadBuf[0] == 0x7F)||(ReadBuf[0] == 0x8A))
            {
                OPEN = 1;
                g_count++;
                //flagx = sendto(sock, "ko", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
                QueSndCmd("ko", 2);
            }print_buffer(ReadBuf, len);
        }

        memset(src_name, 0, 500);
        memset(enc_name, 0, 500);
        memset(dec_name, 0, 500);
        memset(sndbuf, 0, 1024);
        g_errorStatuTxFlag = 0;
        if (((len > 5) && (ReadBuf[0] == 's') && (ReadBuf[1] == 't') && (ReadBuf[2] == 'a') && (ReadBuf[3] == 'r') && (ReadBuf[4] == 't'))||OPEN == 1)//fuzai
        {  
            g_errorStatuTxFlag = 1;
            if (ReadBuf[5] == '1')
            {
                ClearTmpFile();
                system("ls -l /media/mmcblk0p1/ADCdata/ | grep ^-> /root/LFile/LDec/duty.txt");
                otp_encrypt("/root/LFile/LDec/duty.txt", "/root/LFile/LEnc/duty_enc.txt", myencrypt->key, strlen(myencrypt->key), 0);
                printf("udp8001:recvd WGETENC ##########begin trans file#############\n");                
                printf("ftp get filelist--------------------------\n");
                system("ls -l /root/LFile/LEnc/ | grep ^-> /root/LFile/lenclist.log");//获取文件目录
                memset(LineBuf,0,256);
                LineCnt = 0;
                while(GetFileLine("/root/LFile/lenclist.log",LineBuf,256,LineCnt))
                {
	                LineCnt++;
	                sndbuf[0]=0x7f;
	                sndbuf[1]=1+strlen(LineBuf);
	                sndbuf[2]=0;
	                memcpy(sndbuf+3,LineBuf,strlen(LineBuf));
	                int msglen=3+strlen(LineBuf);
	                if(msglen>200)
	                {
	                    printf("filepath len>200 error\n");
	                    continue; 
	                }
	                /******************************************/
	                printf("begin send filename:%s\n",LineBuf);
	                memset(encrypt_buf,0,sizeof(encrypt_buf));
                    print_buffer(sndbuf, msglen);
	                encrypt_buf[0] = sndbuf[0];
	                otp_encrypt_buf(&sndbuf[1],msglen-1,&encrypt_buf[1],myencrypt->key,strlen(myencrypt->key),0);
                    char str[200];
                    memset(str,0,200);
                    sprintf(str,"/root/LFile/LEnc/%s",LineBuf);
                    printf("path:%s\n",str);
	                FILE* fp = fopen(str, "rb+");
	                if (!fp)
	                {
	                    printf("file not found:%s\n",str);
	                    continue;
	                }else{
	                    fclose(fp);
	                }
	                UdpMessageSend(sock,encrypt_buf,msglen,serv_addr);
                    sleep(1);
	                UdpFileSend(sock, serv_addr, str);
                }
                UdpMessageSend(sock,"END",3,serv_addr);
                printf("all file send commpelet\n");
            }
            else if (ReadBuf[5] == '2')
            {
                int timeoutCount = 0 ;
                ClearTmpFile();
                /***********************************************/
                while(1)
                {
                    if(timeoutCount >= 5){
                        printf("file_cmd timeout type:start2");
                        break;
                    }
                    timeoutCount++;
                    
                    memset(ReadBuf,0,sizeof(ReadBuf));
                    len = UdpMessageRecv(sock, ReadBuf, sizeof(ReadBuf), serv_addr);
                    printf("UdpMessagRecv:%d\r\n",len);
                    if(len > 32)len = 32;
                    print_buffer(ReadBuf, len);
                    if(strstr(ReadBuf,"END"))
                    {
                        printf("all file recvd commpelet\n");
                        break;
                    }
                    else if(ReadBuf[0] == 0x7f)
                    {
                        decrypt_buf[0] = ReadBuf[0];
                        otp_encrypt_buf(&ReadBuf[1], len - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
                        printf("wgetTencFilex_2:%d\n",g_timeStopFlag);
                        print_buffer(decrypt_buf, len);
                        if(decrypt_buf[2]==0)
                        {
                            char strfilepath[100];
                            char savepath[100];
                            memset(strfilepath,0,100);
                            memset(savepath,0,100);
                            strncpy(strfilepath,decrypt_buf+3,len-3);       
                            sprintf(savepath,"/root/LFile/LEnc/%s",strfilepath);
                            printf("begin recv filename:%s\n",savepath);
                            UdpFileRecv(sock, serv_addr, savepath);
                        }
                        timeoutCount = 0 ;
                        printf("udp8001:recved 7f and translate\n");
                        print_buffer(ReadBuf,len);
                        sendto(g_kzwj_udpsock, decrypt_buf, len, 0, (struct sockaddr*)&g_kzwj_udpsock, sizeof(g_kzwj_udpsock));
                    }
                }
				/***********************************************/
                pthread_create((pthread_t *)&(ThdWGet_t), NULL, (void*)(ThdWGet), (void*)(NULL));
            }
            else if ((ReadBuf[5] == '3')||OPEN == 1)
            {
                OPEN = 0;             
                if (len == -1)
                {
                    printf("read = -1\n");
                    continue;
                }
                if (ReadBuf[0] == 0x7E)//buf[i+3]:
                {
                    decrypt_buf[0] = ReadBuf[0];
                    otp_encrypt_buf(&ReadBuf[1], len - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
                    print_buffer(decrypt_buf, len);
                    num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
                    if (0 == num)
                    {
                        printf("decrypt_buf[2] = %x\n", decrypt_buf[2]);
                        printf("connect_para->elec_info111111:%x\n", connect_para->elec_info);
                        if (decrypt_buf[2] == 0x01)//0x01:上电  0x02:断电  0x43('c'):查询--len:4
                        {
                            printf("shang dian\n");
                            //D0:电机 D1:舵机 D2:控制微机+深度传感器+姿态传感器 D3:路由器 D4:负载1 D5:负载2 D6:保留 D7:保留
                            printf("decrypt_buf[3] = %x\n", decrypt_buf[3]);
                            if ((decrypt_buf[3] & 1) != 0)//尾端
                            {
                                printf("io6 set 1\n");
                                connect_para->elec_info |= 1;
                                system("gpio-test out 6 1");
                            }
                            printf("$$$$:%x\n", decrypt_buf[3] & 2);
                            if ((decrypt_buf[3] & 2) != 0)//声磁
                            {
//                                printf("io7 set 0\n");
//                                connect_para->elec_info |= 2;
//                                system("gpio-test out 7 0");

                                printf("io5 set 1\n");
                                connect_para->elec_info |= 2;
                                system("gpio-test out 7 1");

                            }
                            printf("decrypt_buf[3] = %x\n", decrypt_buf[3]);
                            if ((decrypt_buf[3] & 4) != 0)//路由 -- 微机
                            {
//                                printf("io8 set 0\n");
//                                connect_para->elec_info |= 4;
//                                system("gpio-test out 8 0");

                                printf("io3 set 1\n");
                                connect_para->elec_info |= 4;
                                system("gpio-test out 9 1");

                            }
                            if ((decrypt_buf[3] & 8) != 0)//DVL
                            {
//                                printf("io9 set 0\n");
//                                connect_para->elec_info |= 8;
//                                system("gpio-test out 9 0");

                                printf("io2 set 1\n");
                                connect_para->elec_info |= 8;
                                system("gpio-test out 10 1");

                            }
                            if ((decrypt_buf[3] & 0x10) != 0)//控制微机
                            {
//                                printf("io10 set 0\n");
//                                connect_para->elec_info |= 0x10;
//                                system("gpio-test out 10 0");

                                printf("io4 set 1\n");
                                connect_para->elec_info |= 0x10;
                                system("gpio-test out 8 1");

                            }
                            if ((decrypt_buf[3] & 0x20) != 0)//自沉
                            {
//                                printf("io11 set 0\n");
//                                connect_para->elec_info |= 0x20;
//                                system("gpio-test out 11 0");

                                printf("io1 set 1\n");
                                connect_para->elec_info |= 0x20;
                                system("gpio-test out 11 1");

                            }
                        }
                        else if (decrypt_buf[2] == 0x02)
                        {
                            if ((decrypt_buf[3] & 1) != 0)
                            {
                                printf("io6 set 0\n");
                                connect_para->elec_info &= 0xfe;
                                system("gpio-test out 6 0");
                            }
                            if ((decrypt_buf[3] & 2) != 0)
                            {
//                                printf("io7 set 1\n");
//                                connect_para->elec_info &= 0xfd;
//                                system("gpio-test out 7 1");

                                printf("io5 set 0\n");
                                connect_para->elec_info &= 0xfd;
                                system("gpio-test out 7 0");

                            }
                            if ((decrypt_buf[3] & 4) != 0)
                            {
//                                printf("io8 set 1\n");
//                                connect_para->elec_info &= 0xfb;
//                                system("gpio-test out 8 1");

                                printf("io3 set 0\n");
                                connect_para->elec_info &= 0xfb;
                                system("gpio-test out 9 0");

                            }
                            if ((decrypt_buf[3] & 8) != 0)
                            {
//                                printf("io9 set 1\n");
//                                connect_para->elec_info &= 0xf7;
//                                system("gpio-test out 9 1");

                                printf("io2 set 0\n");
                                connect_para->elec_info &= 0xf7;
                                system("gpio-test out 10 0");

                            }
                            if ((decrypt_buf[3] & 0x10) != 0)
                            {
//                                printf("io10 set 1\n");
//                                connect_para->elec_info &= 0xef;
//                                system("gpio-test out 10 1");

                                printf("io4 set 0\n");
                                connect_para->elec_info &= 0xef;
                                system("gpio-test out 8 0");

                            }
                            if ((decrypt_buf[3] & 0x20) != 0)
                            {
//                                printf("io11 set 1\n");
//                                connect_para->elec_info &= 0xdf;
//                                system("gpio-test out 11 1");

                                printf("io1 set 0\n");
                                connect_para->elec_info &= 0xdf;
                                system("gpio-test out 11 0");

                            }
                        }
                        else if (decrypt_buf[2] == 0x08)
                        {
                            snd_240Sv[0] = decrypt_buf[0];
                            decrypt_buf[1] = 0x05;
                            printf("detph:%0x\r\n",depth);
                            decrypt_buf[3] = depth&0x00ff;
                            decrypt_buf[4] = (depth&0xff00)>>8;
                            decrypt_buf[5] = 0;
                            for (i = 1; i < 5; i++)
                            {
                                decrypt_buf[5] += decrypt_buf[i];
                            }
                            decrypt_buf[5] &= 0xff;
                            otp_encrypt_buf(&decrypt_buf[1], 5, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                            UdpMessageSend(g_dt_udpsock, snd_240Sv, 6, g_dt_serv_addr);
                            continue;
                        }
                        else if (decrypt_buf[2] == 0x43)
                        {//查询'C'

                            memset(snd_240Sv, 0, sizeof(snd_240Sv));
                            snd_240Sv[0] = decrypt_buf[0];
                            decrypt_buf[3] = connect_para->elec_info;
                            decrypt_buf[4] = 0;
                            for (i = 1; i < 4; i++)
                            {
                                decrypt_buf[4] += decrypt_buf[i];
                            }
                            decrypt_buf[4] &= 0xff;
                            otp_encrypt_buf(&decrypt_buf[1], len - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                            UdpMessageSend(g_dt_udpsock, snd_240Sv, 5, g_dt_serv_addr); 
                            rcv_cmd_flag = 1;
                            continue;
                        }
                        else if (decrypt_buf[2] == 0x5A)
                        {
                        	//抛载检查('Z')--len:3{
                            printf("paozai check\n");
                            //system("gpio-test out 12 1");
                        }
                        else
                        {
                            printf("bao tou:0x7E  command index:%x\n", decrypt_buf[2]);
                            UdpMessageSend(sock, decrypt_buf, decrypt_buf[1] + 1, serv_addr);
                            continue;
                        }
                        memset(snd_240Sv, 0, sizeof(snd_240Sv));
                        snd_240Sv[0] = decrypt_buf[0];
                        otp_encrypt_buf(&decrypt_buf[1], len - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                        sendto(g_kzwj_udpsock,decrypt_buf,len,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
                        rcv_cmd_flag = 1;
                    }
                }
                else if (ReadBuf[0] == 0x7F)
                {
                    decrypt_buf[0] = ReadBuf[0];
                    otp_encrypt_buf(&ReadBuf[1], len - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
                    printf("cmd_7F:%d\r\n",len);
                    print_buffer(decrypt_buf, len);
                    num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
                    if (0 == num)
                    {
                        g_sencunt++;
                        sendto(g_kzwj_udpsock,decrypt_buf,decrypt_buf[1] + 1,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
                    }
                }
                else if (ReadBuf[0] == 0x8A)
                {
                    decrypt_buf[0] = ReadBuf[0];
                    otp_encrypt_buf(&ReadBuf[1], len - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
                    num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
                    if (0 == num)
                    {
                        printf("decrypt_buf[2]:%x\n", decrypt_buf[2]);
                        print_buffer(decrypt_buf, strlen(decrypt_buf));
                        if (decrypt_buf[2] == 0x1)//SIM卡 查询/设置
                        {
                            if (decrypt_buf[3] == 0x1) {
                                check_ICJC_flag = 1;
                                sleep(1);
                                memset(snd_240Sv, 0, sizeof(snd_240Sv));
                                snd_240Sv[0] = decrypt_buf[0];
                                printf("Check SIM:%x %x %x\n", rds_buf[7], rds_buf[8], rds_buf[9]);
                                decrypt_buf[5] = rds_buf[7];
                                decrypt_buf[6] = rds_buf[8];
                                decrypt_buf[7] = rds_buf[9];
                                decrypt_buf[len - 1] = 0;
                                for (i = 1; i < len - 1; i++)
                                {
                                    decrypt_buf[len - 1] += decrypt_buf[i];
                                }
                                decrypt_buf[len - 1] &= 0xff;
                                otp_encrypt_buf(&decrypt_buf[1], len - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                                //UdpMessageSend(g_kzwj_udpsock, decrypt_buf, len, g_kzwj_serv_addr); 
                                sendto(g_kzwj_udpsock,decrypt_buf,len,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
                                rcv_cmd_flag = 1;
                            }
                            else if (decrypt_buf[3] == 0x2)
                            {
                                if (decrypt_buf[4] == 0x01)
                                {
                                    memcpy(&rds_buf[7], &decrypt_buf[5], 3);
                                    save_card_info();
                                }
                                else if (decrypt_buf[4] == 0x02)
                                {
                                    memset(rds_rcv_buf, 0, sizeof(rds_rcv_buf));
                                    memcpy(rds_rcv_buf, &decrypt_buf[5], 3);
                                    save_card_info();
                                }
                                memset(snd_240Sv, 0, sizeof(snd_240Sv));
                                snd_240Sv[0] = decrypt_buf[0];
                                decrypt_buf[5] = 0;
                                decrypt_buf[6] = 0;
                                decrypt_buf[7] = 1;
                                decrypt_buf[len - 1] = 0;
                                for (i = 1; i < len - 1; i++)
                                {
                                    decrypt_buf[len - 1] += decrypt_buf[i];
                                }
                                decrypt_buf[len - 1] &= 0xff;
                                otp_encrypt_buf(&decrypt_buf[1], len - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                                sendto(g_kzwj_udpsock,decrypt_buf,len,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
                                rcv_cmd_flag = 1;
                            }
                        }
                        else if (decrypt_buf[2] == 0x2)//电台指令
                        {
                            if (decrypt_buf[3] == 0x01)
                            {
                                memset(snd_diantai_buf, 0, sizeof(snd_diantai_buf));
                                memcpy(snd_diantai_buf, &decrypt_buf[4], decrypt_buf[1] - 4);
                                snd_to_diantai(snd_diantai_buf);
                            }
                        }
                        else if (decrypt_buf[2] == 0x3)//密钥设置
                        {
                            memset(snd_240Sv, 0, sizeof(snd_240Sv));
                            snd_240Sv[0] = decrypt_buf[0];
                            otp_encrypt_buf(&decrypt_buf[1], len - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                            sendto(g_kzwj_udpsock,decrypt_buf,len,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
                            rcv_cmd_flag = 1;
                            memset(myencrypt->key, 0, sizeof(myencrypt->key));
                            memcpy(myencrypt->key, &decrypt_buf[4], decrypt_buf[1] - 4);
                        }
                    }
                }
            }
            else if (ReadBuf[5] == '5')
            {
                ClearTmpFile();
                usleep(200);
                /*********************************************************/
                UdpMessageSend(sock,"WGETENC",7,serv_addr);
                while(1)
                {
                    memset(ReadBuf,0,sizeof(ReadBuf));
                    len = UdpMessageRecv(sock, ReadBuf, sizeof(ReadBuf), serv_addr);
                    printf("wgetTencFile:\n");
                    print_buffer(ReadBuf, len);
                    
                    if(strstr(ReadBuf,"continue"))
                    {
                        printf("all file recvd commpelet\n");
                        break;
                    }
                    else if(ReadBuf[0] = 0x7f)
                    {
                        decrypt_buf[0] = ReadBuf[0];
                        otp_encrypt_buf(&ReadBuf[1], len - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
                        printf("wgetTencFilex:\n");
                        print_buffer(decrypt_buf, len);
                        if(decrypt_buf[2]==0)
                        {
                            char strfilepath[100];
                            char savepath[100];
                            memset(strfilepath,0,100);
                            memset(savepath,0,100);
                            strncpy(strfilepath,decrypt_buf+3,len-3);       
                            sprintf(savepath,"/root/LFile/LDec/%s",strfilepath);
                            printf("begin recv filename:%s\n",savepath);
                            UdpFileRecv(sock, serv_addr, savepath);
                        }
                        printf("udp8001:recved 7f and translate\n");
                        print_buffer(ReadBuf,len);
                        sendto(g_kzwj_udpsock, decrypt_buf, len, 0, (struct sockaddr*)&g_kzwj_udpsock, sizeof(g_kzwj_udpsock));
                    }
                }
                /*********************************************************/
                LineCnt = 0;
                while (GetFileLine("/root/LFile/LDec/duty.txt", LineBuf, 256, LineCnt))
                {
                    LineCnt++;
                    memset(src_name, 0, 500);
                    memset(enc_name, 0, 500);
                    sprintf(src_name, "/media/mmcblk0p1/ADCdata/%s", LineBuf);//pc104
                    sprintf(enc_name, "/root/LFile/LEnc/%s", LineBuf);
                    otp_encrypt(src_name, enc_name, myencrypt->key, strlen(myencrypt->key), 0);
                }
                sleep(2);//等待岸基udp接收缓冲区释放完全
                UdpMessageSend(sock, "ok", 2, serv_addr);
                
                while(1)
                {
                    memset(ReadBuf, 0, sizeof(ReadBuf));
                    len = UdpMessageRecv(sock, ReadBuf, sizeof(ReadBuf),serv_addr);
                    print_buffer(ReadBuf, len);
                    if(strstr(ReadBuf,"WGETENC"))break;
                    else if((ReadBuf[0]==0x7e)||(ReadBuf[0]==0x7f)||(ReadBuf[0]==0x7c)||(ReadBuf[0]==0x8a))
                    {
                        memset(g_readBuf,0,1024);
                        memcpy(g_readBuf,ReadBuf,len);
                        g_lenth = len;
                        printf("g_readBuf_w:");
                        print_buffer(g_readBuf, len);
                    }
                    if(g_timeStopFlag>=10){
                        g_timeStopFlag = 0;
                        printf("cmd recv fail\n");
                        break;
                    }
                }
                
                if(strstr(ReadBuf,"WGETENC"))
                {
                    printf("udp8001:recvd WGETENC ##########begin trans file#############\n");
                    printf("ftp get filelist--------------------------\n");
                    system("ls -l /root/LFile/LEnc/ | grep ^-> /root/LFile/lenc2list.log");//获取文件目录
                    memset(LineBuf,0,256);
                    LineCnt = 0;
                    while(GetFileLine("/root/LFile/lenc2list.log",LineBuf,256,LineCnt))
                    {
    	                LineCnt++;
    	                // send file
    	                sndbuf[0]=0x7f;
    	                sndbuf[1]=1+strlen(LineBuf);
    	                sndbuf[2]=0;
    	                memcpy(sndbuf+3,LineBuf,strlen(LineBuf));
    	                int msglen=3+strlen(LineBuf);
    	                if(msglen>200)
    	                {
    	                    printf("filepath len>200 error\n");
    	                    continue; 
    	                }
    	                /******************************************/
    	                printf("begin send filename:%s\n",LineBuf);
    	                memset(encrypt_buf,0,sizeof(encrypt_buf));
                        print_buffer(sndbuf, msglen);
    	                encrypt_buf[0] = sndbuf[0];
    	                otp_encrypt_buf(&sndbuf[1],msglen-1,&encrypt_buf[1],myencrypt->key,strlen(myencrypt->key),0);
                        char str[200];
                        memset(str,0,200);
                        sprintf(str,"/root/LFile/LEnc/%s",LineBuf);
                        printf("path:%s\n",str);
    	                FILE* fp = fopen(str, "rb+");
    	                if (!fp)
    	                {
    	                    printf("file not found:%s\n",str);
    	                    continue;
    	                }else{
    	                    fclose(fp);
    	                }
    	                UdpMessageSend(sock,encrypt_buf,msglen,serv_addr);
                        sleep(1);
    	                UdpFileSend(sock, serv_addr, str);
                    }
                    UdpMessageSend(sock,"continue",8,serv_addr);
                    printf("all file send commpelet\n");
                }
      
            }
            else if (ReadBuf[5] == '6')
            {
                ClearTmpFile();
                memset(ReadBuf, 0, sizeof(ReadBuf));
                #ifdef debugMode
                //sprintf(ReadBuf, "/root/wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@%s/ata0b/* -c", KZWJIP);
                //sprintf(ReadBuf, "/root/wget -P /root/LFile/LDec ftp://15993:2271221@%s/localuser/ata0b/* -c", KZWJIP);
                sprintf(ReadBuf, "/root/wget -P /root/LFile/LDec ftp://12991:271221@%s/localuser/home/target/AUV_LINUX/logs/* -c", KZWJIP);
                //sprintf(ReadBuf, "/root/wget -P /root/LFile/LDec ftp://15993:2271221@%s/localuser/home/target/AUV_LINUX/logs/* -c", KZWJIP);
                #else
                sprintf(ReadBuf, "/root/wget -P /root/LFile/LDec ftp://target:password@%s/logs/* -c", KZWJIP);//天和防务路径
                #endif
                system(ReadBuf);         
                system(DECFILELIST);
                LineCnt = 0;
                while (GetFileLine(FILELIST, LineBuf, 256, LineCnt))
                {
                    LineCnt++;
                    memset(src_name, 0, 500);
                    memset(enc_name, 0, 500);
                    sprintf(src_name, "/root/LFile/LDec/%s", LineBuf);//控制微机
                    sprintf(enc_name, "/root/LFile/LEnc/%s", LineBuf);
                    otp_encrypt(src_name, enc_name, myencrypt->key, strlen(myencrypt->key), 0);
                    printf("---------------------=media/mmcblk0p1/log %s\n", src_name);
                }

                printf("udp8001:recvd WGETENC ##########begin trans file#############\n");
                printf("ftp get filelist--------------------------\n");
                system("ls -l /root/LFile/LEnc/ | grep ^-> /root/LFile/lenc6list.log");//获取文件目录
                memset(LineBuf,0,256);
                LineCnt = 0;
                while(GetFileLine("/root/LFile/lenc6list.log",LineBuf,256,LineCnt))
                {
	                LineCnt++;
	                // send file
	                sndbuf[0]=0x7f;
	                sndbuf[1]=1+strlen(LineBuf);
	                sndbuf[2]=0;
	                memcpy(sndbuf+3,LineBuf,strlen(LineBuf));
	                int msglen=3+strlen(LineBuf);
	                if(msglen>200)
	                {
	                    printf("filepath len>200 error\n");
	                    continue; 
	                }
	                /******************************************/
	                printf("begin send filename:%s\n",LineBuf);
	                memset(encrypt_buf,0,sizeof(encrypt_buf));
                    print_buffer(sndbuf, msglen);
	                encrypt_buf[0] = sndbuf[0];
	                otp_encrypt_buf(&sndbuf[1],msglen-1,&encrypt_buf[1],myencrypt->key,strlen(myencrypt->key),0);
                    char str[200];
                    memset(str,0,200);
                    sprintf(str,"/root/LFile/LEnc/%s",LineBuf);
                    printf("path:%s\n",str);
	                FILE* fp = fopen(str, "rb+");
	                if (!fp)
	                {
	                    printf("file not found:%s\n",str);
	                    continue;
	                }else{
	                    fclose(fp);
	                }
	                UdpMessageSend(sock,encrypt_buf,msglen,serv_addr);
                    sleep(1);
	                UdpFileSend(sock, serv_addr, str);
                }
                UdpMessageSend(sock,"END",3,serv_addr);
                printf("all file send commpelet\n");

            }
        }
    }
    printf("ThdConn8001 close\n");
    close(sock);
    exit8001ok = 1;
}

void ThdU8001Process(void)
{
    char decrypt_buf[200];
    char encrypt_buf[200];
    int  num = 0;
    int  i = 0;
    char snd_diantai_buf[50];
    unsigned char src_name[500] = { 0 };
    unsigned char dec_name[500] = { 0 };
    unsigned char enc_name[500] = { 0 };
    char LineBuf[256] = { 0 };
    int  LineCnt = 0;
    char CmdBuf[500] = { 0 };
    /**********************************************/
    while(1)
    {
        if (g_readBuf[0] == 0x7E)//buf[i+3]:
        {
            decrypt_buf[0] = g_readBuf[0];
            otp_encrypt_buf(&g_readBuf[1], g_lenth - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
            print_buffer(decrypt_buf, g_lenth);
            num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
            if (0 == num)
            {
                printf("decrypt_buf[2] = %x\n", decrypt_buf[2]);
                printf("connect_para->elec_info111111:%x\n", connect_para->elec_info);
                if (decrypt_buf[2] == 0x01)//0x01:上电  0x02:断电  0x43('c'):查询--len:4
                {
                    printf("shang dian\n");
                    //D0:电机 D1:舵机 D2:控制微机+深度传感器+姿态传感器 D3:路由器 D4:负载1 D5:负载2 D6:保留 D7:保留
                    printf("decrypt_buf[3] = %x\n", decrypt_buf[3]);
                    if ((decrypt_buf[3] & 1) != 0)
                    {
                        printf("io6 set 0\n");
                        connect_para->elec_info |= 1;
                        system("gpio-test out 6 0");
                    }
                    printf("$$$$:%x\n", decrypt_buf[3] & 2);
                    if ((decrypt_buf[3] & 2) != 0)
                    {
                        printf("io7 set 0\n");
                        connect_para->elec_info |= 2;
                        system("gpio-test out 7 0");
                    }
                    printf("decrypt_buf[3] = %x\n", decrypt_buf[3]);
                    if ((decrypt_buf[3] & 4) != 0)
                    {
                        printf("io8 set 0\n");
                        connect_para->elec_info |= 4;
                        system("gpio-test out 8 0");
                    }
                    if ((decrypt_buf[3] & 8) != 0)
                    {
                        printf("io9 set 0\n");
                        connect_para->elec_info |= 8;
                        system("gpio-test out 9 0");
                    }
                    if ((decrypt_buf[3] & 0x10) != 0)
                    {
                        printf("io10 set 0\n");
                        connect_para->elec_info |= 0x10;
                        system("gpio-test out 10 0");
                    }
                    if ((decrypt_buf[3] & 0x20) != 0)
                    {
                        printf("io11 set 0\n");
                        connect_para->elec_info |= 0x20;
                        system("gpio-test out 11 0");
                    }
                }
                else if (decrypt_buf[2] == 0x02)
                {
                    if ((decrypt_buf[3] & 1) != 0)
                    {
                        printf("io6 set 1\n");
                        connect_para->elec_info &= 0xfe;
                        system("gpio-test out 6 1");
                    }
                    if ((decrypt_buf[3] & 2) != 0)
                    {
                        printf("io7 set 1\n");
                        connect_para->elec_info &= 0xfd;
                        system("gpio-test out 7 1");
                    }
                    if ((decrypt_buf[3] & 4) != 0)
                    {
                        printf("io8 set 1\n");
                        connect_para->elec_info &= 0xfb;
                        system("gpio-test out 8 1");
                    }
                    if ((decrypt_buf[3] & 8) != 0)
                    {
                        printf("io9 set 1\n");
                        connect_para->elec_info &= 0xf7;
                        system("gpio-test out 9 1");
                    }
                    if ((decrypt_buf[3] & 0x10) != 0)
                    {
                        printf("io10 set 1\n");
                        connect_para->elec_info &= 0xef;
                        system("gpio-test out 10 1");
                    }
                    if ((decrypt_buf[3] & 0x20) != 0)
                    {
                        printf("io11 set 1\n");
                        connect_para->elec_info &= 0xdf;
                        system("gpio-test out 11 1");
                    }
                }
                else if (decrypt_buf[2] == 0x43)
                {
                    //查询'C'
                    memset(snd_240Sv, 0, sizeof(snd_240Sv));
                    snd_240Sv[0] = decrypt_buf[0];
                    decrypt_buf[3] = connect_para->elec_info;
                    decrypt_buf[4] = 0;
                    for (i = 1; i < 4; i++)
                    {
                        decrypt_buf[4] += decrypt_buf[i];
                    }
                    decrypt_buf[4] &= 0xff;
                    otp_encrypt_buf(&decrypt_buf[1], g_lenth - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                    //UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
                    rcv_cmd_flag = 1;
                    continue;
                }
                else if (decrypt_buf[2] == 0x5A)
                {
                	//抛载检查('Z')--len:3{
                    printf("paozai check\n");
                    //system("gpio-test out 12 1");
                }
                else
                {
                    printf("bao tou:0x7E  command index:%x\n", decrypt_buf[2]);
                    //send_message_to_uart(uart.fd_5, decrypt_buf, decrypt_buf[1] + 1);
                    //UdpMessageSend(sock, decrypt_buf, decrypt_buf[1] + 1, serv_addr);
                    continue;
                }
                memset(snd_240Sv, 0, sizeof(snd_240Sv));
                snd_240Sv[0] = decrypt_buf[0];
                otp_encrypt_buf(&decrypt_buf[1], g_lenth - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                //UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
                rcv_cmd_flag = 1;
            }
            memset(g_readBuf,0,1024);
            g_lenth = 0;
        }
        else if (g_readBuf[0] == 0x7F)
        {
            decrypt_buf[0] = g_readBuf[0];
            otp_encrypt_buf(&g_readBuf[1], g_lenth - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
            print_buffer(decrypt_buf, g_lenth);
            num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
            if (0 == num)
            {
                g_sencunt++;
                sendto(g_kzwj_udpsock,decrypt_buf,decrypt_buf[1] + 1,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
            }
            memset(g_readBuf,0,1024);
            g_lenth = 0;
        }
        else if (g_readBuf[0] == 0x8A)
        {
            decrypt_buf[0] = g_readBuf[0];
            otp_encrypt_buf(&g_readBuf[1], g_lenth - 1, &decrypt_buf[1], myencrypt->key, strlen(myencrypt->key), 1);
            num = check_sum_host_PC(decrypt_buf, decrypt_buf[1] + 1);
            if (0 == num)
            {
                printf("decrypt_buf[2]:%x\n", decrypt_buf[2]);
                print_buffer(decrypt_buf, strlen(decrypt_buf));
                if (decrypt_buf[2] == 0x1)//SIM卡 查询/设置
                {
                    if (decrypt_buf[3] == 0x1) {
                        check_ICJC_flag = 1;
                        sleep(1);
                        memset(snd_240Sv, 0, sizeof(snd_240Sv));
                        snd_240Sv[0] = decrypt_buf[0];
                        printf("Check SIM:%x %x %x\n", rds_buf[7], rds_buf[8], rds_buf[9]);
                        decrypt_buf[5] = rds_buf[7];
                        decrypt_buf[6] = rds_buf[8];
                        decrypt_buf[7] = rds_buf[9];
                        decrypt_buf[g_lenth - 1] = 0;
                        for (i = 1; i < g_lenth - 1; i++)
                        {
                            decrypt_buf[g_lenth - 1] += decrypt_buf[i];
                        }
                        decrypt_buf[g_lenth - 1] &= 0xff;
                        otp_encrypt_buf(&decrypt_buf[1], g_lenth - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                        rcv_cmd_flag = 1;
                    }
                    else if (decrypt_buf[3] == 0x2)
                    {
                        if (decrypt_buf[4] == 0x01)
                        {
                            memcpy(&rds_buf[7], &decrypt_buf[5], 3);
                            save_card_info();
                        }
                        else if (decrypt_buf[4] == 0x02)
                        {
                            memset(rds_rcv_buf, 0, sizeof(rds_rcv_buf));
                            memcpy(rds_rcv_buf, &decrypt_buf[5], 3);
                            save_card_info();
                        }
                        memset(snd_240Sv, 0, sizeof(snd_240Sv));
                        snd_240Sv[0] = decrypt_buf[0];
                        decrypt_buf[5] = 0;
                        decrypt_buf[6] = 0;
                        decrypt_buf[7] = 1;
                        decrypt_buf[g_lenth - 1] = 0;
                        for (i = 1; i < g_lenth - 1; i++)
                        {
                            decrypt_buf[g_lenth - 1] += decrypt_buf[i];
                        }
                        decrypt_buf[g_lenth - 1] &= 0xff;
                        otp_encrypt_buf(&decrypt_buf[1], g_lenth - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                        rcv_cmd_flag = 1;
                    }
                }
                else if (decrypt_buf[2] == 0x2)//电台指令
                {
                    if (decrypt_buf[3] == 0x01)
                    {
                        memset(snd_diantai_buf, 0, sizeof(snd_diantai_buf));
                        memcpy(snd_diantai_buf, &decrypt_buf[4], decrypt_buf[1] - 4);
                        snd_to_diantai(snd_diantai_buf);
                    }
                }
                else if (decrypt_buf[2] == 0x3)//密钥设置
                {
                    memset(snd_240Sv, 0, sizeof(snd_240Sv));
                    snd_240Sv[0] = decrypt_buf[0];
                    otp_encrypt_buf(&decrypt_buf[1], g_lenth - 1, &snd_240Sv[1], myencrypt->key, strlen(myencrypt->key), 0);
                    UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr); 
                    rcv_cmd_flag = 1;
                    memset(myencrypt->key, 0, sizeof(myencrypt->key));
                    memcpy(myencrypt->key, &decrypt_buf[4], decrypt_buf[1] - 4);
                }
            }
            memset(g_readBuf,0,1024);
            g_lenth = 0;
        }
        usleep(100000);
    }
}

void ThdConn8080(void)
{
	char decrypt_buf[200];
	char *server_ip = SERVERIP;//"192.168.168.240";
	int  server_port = 8010;
	
	int  len= 0;
	int  num = 0;
	int  i = 0;

	char snd_diantai_buf[50];
	unsigned char src_name[500] = { 0 };
	unsigned char dec_name[500] = { 0 };
	unsigned char enc_name[500] = { 0 };	
	char LineBuf[256]={0};
	int  LineCnt=0;
	char CmdBuf[500]={0};
	char ReadBuf[200] = { 0 };
	
	//int clnt_sock = socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);
	int clnt_sock = socket(AF_INET,SOCK_STREAM,0);
	if(clnt_sock == -1)
	{		
		printf("socket8080 open fail!\n");	
		write_log("socket8080 open fail\r\n");		
		return;
	}
	
	//超时时间
	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	
	if(setsockopt(clnt_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout))==-1)
	{
		printf("setsockopt SO_RCVTIMEO fail!\n");	
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	serv_addr.sin_port = htons(server_port);
	
	int ret = connect(clnt_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	if(ret == -1)
	{
		printf("socket8080 connect failse %d\n", ret);
		write_log("socket8080 connect fail\r\n");
		while(1)
		{
			sleep(10);
			ret = connect(clnt_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
			if(ret != -1)
			{
				printf("socket8080 connect Sucess,ret=%d\n",ret);
				break;
			}
			else
			{
				printf("socket8080 connect failse %d\n", ret);
				perror("ThdConn8080 error");
				write_log("socket8080 connect fail\r\n");
			}
		}
	}
	else
	{
		printf("socket8080 connect ok %d\n",ret);
	}
	while(1)
	{		
		memset(ReadBuf,0,sizeof(ReadBuf));	
		memset(decrypt_buf,0,sizeof(decrypt_buf));
		printf("ThdConn8080 is activing!+++++++++++++++++++\r\n");	
		signal(SIGPIPE,sig_handler);
		//pthread_testcancel();
		len = read(clnt_sock, ReadBuf, sizeof(ReadBuf));
		//pthread_testcancel();
		printf("ThdConn8080 is activing!-------------------\r\n");	
		if (exit8080)
		{
			break;
		}
		if(0 >= len)
		{
			sleep(10);
			continue;
		}
		
		memset(src_name, 0, 500);
		memset(enc_name, 0, 500);
		memset(dec_name, 0, 500);

		if((len > 5) && (ReadBuf[0] == 's') && (ReadBuf[1] == 't') &&(ReadBuf[2] == 'a') &&(ReadBuf[3] == 'r') &&(ReadBuf[4] == 't'))//fuzai
		{
			if (ReadBuf[5] == '1')
			{
				ClearTmpFile();
				system("ls -l /media/mmcblk0p1/ADCdata/ | grep ^-> /root/LFile/LDec/duty.txt");
				otp_encrypt("/root/LFile/LDec/duty.txt","/root/LFile/LEnc/duty_enc.txt",myencrypt->key,strlen(myencrypt->key),0);
				write(clnt_sock,"ok",2);
				usleep(200);
			}
			else if (ReadBuf[5] == '2')
			{
				ClearTmpFile();
				pthread_create((pthread_t *)&(ThdWGet_t),NULL,(void*)(ThdWGet),(void*)(NULL));
				write(clnt_sock,"ok",2);
			
			}
			else if (ReadBuf[5] == '3')
			{
				write(clnt_sock,"ok",2);
			
				memset(ReadBuf,0,sizeof(ReadBuf));
				len = read(clnt_sock, ReadBuf,sizeof(ReadBuf));
				if(len == -1)
				{
					printf("read = -1\n");

				}
				if(ReadBuf[0] == 0x7E)//buf[i+3]:
				{
					decrypt_buf[0] = ReadBuf[0];
					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);
					printf("num11===%d\n",num);
					if(0 == num)
					{
						printf("decrypt_buf[2] = %x\n",decrypt_buf[2]);
						printf("connect_para->elec_info111111:%x\n",connect_para->elec_info);
						if(decrypt_buf[2] == 0x01)//0x01:上电  0x02:断电  0x43('c'):查询--len:4
						{
							printf("shang dian\n");
							//D0:电机 D1:舵机 D2:控制微机+深度传感器+姿态传感器 D3:路由器 D4:负载1 D5:负载2 D6:保留 D7:保留
							printf("decrypt_buf[3] = %x\n",decrypt_buf[3]);
							if((decrypt_buf[3] & 1) != 0)
							{
								connect_para->elec_info |= 1;
								system("gpio-test out 6 0");
							}							
							printf("$$$$:%x\n",decrypt_buf[3] & 2);
							if((decrypt_buf[3] & 2) != 0)
							{
								connect_para->elec_info |= 2;
								system("gpio-test out 7 0");
							}							
							printf("decrypt_buf[3] = %x\n",decrypt_buf[3]);
							if((decrypt_buf[3] & 4) != 0)
							{
								connect_para->elec_info |= 4;
								system("gpio-test out 8 0");
							}
							if((decrypt_buf[3] & 8) != 0)
							{
								connect_para->elec_info |= 8;
								system("gpio-test out 9 0");
							}
							if((decrypt_buf[3] & 0x10) != 0)
							{
								connect_para->elec_info |= 0x10;
								system("gpio-test out 10 0");
							}
							if((decrypt_buf[3] & 0x20) != 0)
							{
								connect_para->elec_info |= 0x20;
								system("gpio-test out 11 0");
							}						
						}
						else if(decrypt_buf[2] == 0x02)
						{
							if((decrypt_buf[3] & 1) != 0)
							{
								connect_para->elec_info &= 0xfe;
								system("gpio-test out 6 1");
							}							
							if((decrypt_buf[3] & 2) != 0)
							{
								connect_para->elec_info &= 0xfd;
								system("gpio-test out 7 1");
							}
							if((decrypt_buf[3] & 4) != 0)
							{
								connect_para->elec_info &= 0xfb;
								system("gpio-test out 8 1");
							}
							if((decrypt_buf[3] & 8) != 0)
							{
								connect_para->elec_info &= 0xf7;
								system("gpio-test out 9 1");
							}
							if((decrypt_buf[3] & 0x10) != 0)
							{
								connect_para->elec_info &= 0xef;
								system("gpio-test out 10 1");
							}
							if((decrypt_buf[3] & 0x20) != 0)
							{
								connect_para->elec_info &= 0xdf;
								system("gpio-test out 11 1");
							}
						}
						else if(decrypt_buf[2] == 0x43)
						{//查询'C'
							
							memset(snd_240Sv,0,sizeof(snd_240Sv));
							snd_240Sv[0] = decrypt_buf[0];
							decrypt_buf[3] = connect_para->elec_info;
							decrypt_buf[4] = 0;
							for(i=1;i<4;i++)
							{
								decrypt_buf[4] += decrypt_buf[i];
							}
							decrypt_buf[4] &= 0xff;

							otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
							rcv_cmd_flag = 1;
							continue;
						}
						else if(decrypt_buf[2] == 0x5A)
						{//抛载检查('Z')--len:3{
							printf("paozai check\n");	
							//system("gpio-test out 12 1");
						}
						else
						{
							printf("bao tou:0x7E  command index:%x\n",decrypt_buf[2]);
							send_message_to_uart(uart.fd_5,decrypt_buf,decrypt_buf[1]+1);
							continue;
						}
						memset(snd_240Sv,0,sizeof(snd_240Sv));
						snd_240Sv[0] = decrypt_buf[0];
						otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
						rcv_cmd_flag = 1;					
					}
				}
				else if(ReadBuf[0] == 0x7F)
				{
					decrypt_buf[0] = ReadBuf[0];
					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);

					printf("num:%x\n",num);
					if(0 == num)
					{
						for(i=0;i<len;i++)
							printf("decrypt_buf[%d]:%x\n",i,decrypt_buf[i]);
						//printf("decrypt_buf:%x %x %x %x\n",decrypt_buf[0],decrypt_buf[1],decrypt_buf[2],decrypt_buf[3]);
						send_message_to_uart(uart.fd_5,decrypt_buf,decrypt_buf[1]+1);
					}
				}
				else if(ReadBuf[0] == 0x8A)
				{
					decrypt_buf[0] = ReadBuf[0];
					otp_encrypt_buf(&ReadBuf[1],len-1,&decrypt_buf[1],myencrypt->key,strlen(myencrypt->key),1);
					num = check_sum_host_PC(decrypt_buf,decrypt_buf[1]+1);
					if(0 == num)
					{
						printf("decrypt_buf[2]:%x\n",decrypt_buf[2]);
						print_buffer(decrypt_buf,strlen(decrypt_buf));
						if(decrypt_buf[2] == 0x1)//SIM卡 查询/设置
						{
							if(decrypt_buf[3] == 0x1){
								check_ICJC_flag = 1;
								sleep(1);
								memset(snd_240Sv,0,sizeof(snd_240Sv));
								snd_240Sv[0] = decrypt_buf[0];
								printf("Check SIM:%x %x %x\n",rds_buf[7],rds_buf[8],rds_buf[9]);
								decrypt_buf[5] = rds_buf[7];
								decrypt_buf[6] = rds_buf[8];
								decrypt_buf[7] = rds_buf[9];
								decrypt_buf[len-1] = 0;
								for(i=1;i<len-1;i++)
								{
									decrypt_buf[len-1] += decrypt_buf[i];								
								}
								
								decrypt_buf[len-1] &= 0xff;
								otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
								rcv_cmd_flag = 1;
							}
							else if(decrypt_buf[3] == 0x2)
							{
								if(decrypt_buf[4] == 0x01)
								{
									memcpy(&rds_buf[7],&decrypt_buf[5],3);
									save_card_info();
									
								}
								else if(decrypt_buf[4] == 0x02)
								{
									memset(rds_rcv_buf,0,sizeof(rds_rcv_buf));
									memcpy(rds_rcv_buf,&decrypt_buf[5],3);
									save_card_info();
								}
								memset(snd_240Sv,0,sizeof(snd_240Sv));
								
								snd_240Sv[0] = decrypt_buf[0];
								decrypt_buf[5] = 0;
								decrypt_buf[6] = 0;
								decrypt_buf[7] = 1;
								decrypt_buf[len-1] = 0;
								for(i=1;i<len-1;i++)
								{
									decrypt_buf[len-1] += decrypt_buf[i];
								}
								
								decrypt_buf[len-1] &= 0xff;
								otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
								rcv_cmd_flag = 1;
							}
						}
						else if(decrypt_buf[2] == 0x2)//电台指令
						{
							if(decrypt_buf[3] == 0x01)
							{
								memset(snd_diantai_buf,0,sizeof(snd_diantai_buf));
								memcpy(snd_diantai_buf,&decrypt_buf[4],decrypt_buf[1]-4);
								snd_to_diantai(snd_diantai_buf);
							}
						}
						else if(decrypt_buf[2] == 0x3)//密钥设置
						{						
							memset(snd_240Sv,0,sizeof(snd_240Sv));							
							snd_240Sv[0] = decrypt_buf[0];
							otp_encrypt_buf(&decrypt_buf[1],len-1,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
							rcv_cmd_flag = 1;
							
							memset(myencrypt->key,0,sizeof(myencrypt->key));
							memcpy(myencrypt->key,&decrypt_buf[4],decrypt_buf[1]-4);
						}
					}
				}				
			}		
			else if (ReadBuf[5] == '5')
			{
				ClearTmpFile();	
				usleep(200);
				if(system(WGETPC104DUTY)==0)
				{
					printf("GetFile get fail\n");
				}
				else
				{
					printf("GetFile get succesful\n");
				}					
				LineCnt=0;							
				while (GetFileLine("/root/LFile/LDec/duty.txt", LineBuf, 256, LineCnt))
				{
					LineCnt++;		
					memset(src_name, 0, 500);
					memset(enc_name, 0, 500);
					sprintf(src_name,"/media/mmcblk0p1/ADCdata/%s",LineBuf);//pc104
					sprintf(enc_name,"/root/LFile/LEnc/%s",LineBuf);				
					otp_encrypt(src_name,enc_name,myencrypt->key,strlen(myencrypt->key),0);						
					//printf("22222222222222222222222222-=media/mmcblk0p1/log %s\n",enc_name);				
				}				
				write(clnt_sock,"ok",2);							
			}
			else if (ReadBuf[5] == '6')
			{			
				ClearTmpFile();
				memset(ReadBuf,0,sizeof(ReadBuf));;
				sprintf(ReadBuf,"/root/wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@%s/ata0b/* -c",KZWJIP);
				//./wget -P /root/LFile/LDec ftp://ftpserver:ftpserver@192.168.0.3/ata0b/* -c
				system(ReadBuf);	
					
				system(DECFILELIST);
				LineCnt=0;			
				while (GetFileLine(FILELIST, LineBuf, 256, LineCnt))
				{
					LineCnt++;		
					memset(src_name, 0, 500);
					memset(enc_name, 0, 500);
					sprintf(src_name,"/root/LFile/LDec/%s",LineBuf);//控制微机
					sprintf(enc_name,"/root/LFile/LEnc/%s",LineBuf);				
					otp_encrypt(src_name,enc_name,myencrypt->key,strlen(myencrypt->key),0);						
					printf("---------------------=media/mmcblk0p1/log %s\n",src_name);				
				}				
				write(clnt_sock,"ok",2);							
			}
		}	
	}
	
	printf("ThdConn8080 close\n");
	close(clnt_sock);
	exit8080ok = 1;
}

//***********************************ThdConn800**************************************//
//ThdConn800通过TCP协议实现与192.168.168.240的无线socket连接,并回传snd_240Sv消息
//
//*************************************************************************************************//
void ThdConn800(void)//send to anji
{	
	char *server_ip = SERVERIP;//"192.168.168.240";
	int server_port = 800;

	printf("Socket_clint5\n");

	/*******************server app************************/
	int clnt_sock = socket(AF_INET,SOCK_STREAM,0);
	if(clnt_sock == -1)
	{		
		printf("ThdConn800 open socket fail!\n");	
		write_log("ThdConn800 connect fail\r\n");		
		return;
	}
	struct sockaddr_in serv_addr;
	memset(&serv_addr,0,sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_ip);
	serv_addr.sin_port = htons(server_port);

	int ret = connect(clnt_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	if(ret == -1)
	{
		printf("ThdConn800 connect failse %d\n", ret);
		write_log("ThdConn800 connect fail\r\n");
		while(1)
		{
			sleep(10);
			ret = connect(clnt_sock,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
			if(ret != -1)
			{
				printf("ThdConn800 connect Success\n");
				break;
			}
			else
			{
				/*times += 1;
				printf("ThdConn800  connect failse %d\n", ret);
				if(times == 1)
				{
					fd = fopen("/lost+found/error.log","a+");
					fwrite("ThdConn800 error\n",23,1,fd);
					fclose(fd);
				}*/
			}
		}
	}
	else
	{
		printf("ThdConn800 connect Success \n");
	}
	
	while(1)
	{
		usleep(200);
		if (exit800)
		{
			
			break;
		}
		
		if(snd_beidou_addr_info == 1)
		{
			
			continue;
		}
		
		if(rcv_cmd_flag == 0)
		{
			continue;
		}
		
		signal(SIGPIPE,sig_handler);
		int retval=write(clnt_sock,snd_240Sv,strlen(snd_240Sv));
		if(retval==-1)
		{
			printf("write = error\r\n");
			if(errno==EINTR)
			{
				printf("errno = EINTR\r\n");
			}
			else if(errno==EPIPE)
			{
				printf("errno = EPIPE\r\n");
			}
			else
			{
				printf("errno = others\r\n");
			}
		}
		//(Uwater_status.i800SendNum)++;
		//Thddebug();
		write(clnt_sock,sendbuff,strlen(sendbuff));  //定时发送本机自检状态
		//printf("800-------------sendbuff:%s\n",sendbuff);
		rcv_cmd_flag = 0;
		usleep(200);
	}
	printf("ThdConn800 close\n");
	close(clnt_sock);
	exit800ok = 1;
}

void gpio_reset(void)
{
	system("gpio-test out 0 0");	//beidou 12V
	system("gpio-test out 1 0");	//gongfang 12V
	system("gpio-test out 2 0");	//diantai 3.3V
	system("gpio-test out 3 0");	//beidou 5V
	sleep(5);
	system("gpio-test out 0 1");
	system("gpio-test out 3 1");
	sleep(5);
	system("gpio-test out 2 1");
	sleep(1);
	system("gpio-test out 1 1");
	printf("gpio reset\n");
}

void test1(void)
{
	int reg = 0;	
	snd_to_diantai("at+mwstatus");
}

void test2(void)
{
	char buf[50];
	char key[20] = "at+mwstatus";
	int i = 0;
	buf[0] = 0x8A;
	buf[1] = 4+strlen(key);
	buf[2] = 0x02;
	buf[3] = 0x01;
	memcpy(&buf[4],key,strlen(key));
	
	buf[4+strlen(key)] = 0;
	for(i=1;i<(4+strlen(key));i++)
	{
		buf[4+strlen(key)] += buf[i];
	}
	buf[4+strlen(key)] &= 0xff;
	
	printf("buf:%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7],buf[8],buf[9],buf[10],buf[11],buf[12],buf[13],buf[14],buf[15]);
	printf("\nfinish66666\n\n\n");		
}

int MountUser(void)
{	
	//if(system("mount -t cifs //192.168.0.55/hf-data-share /media/mmcblk0p1 -o username=administrator,password=hf123456,sec=ntlm")==0)
	system(MOUNT);
	if(access("/media/mmcblk0p1/ADCdata",0)==0)
	{
		printf("mount  192.168.0.55 OK\n");
		return 1;
	}
	else
	{
		printf("mount  192.168.0.55 False\n");
		//write_log("mount  192.168.0.55 False\r\n");
		return 1;
	}
	return 0;
}

void Thddebug()
{
	protocol_ZJ.head=0x8B;
	protocol_ZJ.length=0x88;
	protocol_ZJ.type=0x8B;
	protocol_ZJ.content=Uwater_status;
	protocol_ZJ.crc=0xAA;
	protocol_struct2string(protocol_ZJ);
	printf(" Uwater_status. SIMcard=%d\n",Uwater_status. SIMcard);
	printf(" Uwater_status. tx_RNSS=%d\n",Uwater_status. tx_RNSS);
	printf(" Uwater_status. tx_RDSS=%d\n",Uwater_status. tx_RDSS);
	printf(" Uwater_status. DT_uart=%d\n",Uwater_status. DT_uart);
	printf(" Uwater_status. DT_100Base=%d\n",Uwater_status. DT_100Base);
	printf(" Uwater_status. external_Uart=%d\n",Uwater_status. external_Uart);
	printf(" Uwater_status. external_100base=%d\n",Uwater_status. external_100base);			
}

//测试无线通信协议---表4
void testWxTX(void)
{
	char test[6] = {0x7c,0x06,0x0a,0x0a,0x01,0x00};
	char crc = 0,i=0;
	for(i=1;i<5;i++)crc+=test[i];
	test[5] = crc&0xff;
	print_buffer(test,6);
    sendto(g_kzwj_udpsock,test,6,0,(struct sockaddr*)&g_kzwj_serv_addr,sizeof(g_kzwj_serv_addr));
}

//自检状态反馈---表6
void zjStatusFeedback(void)
{
	char test[6] = {0x72,0x06,0x0a,0x0a,0x01,0x97};
	char crc = 0,i=0;
	for(i=0;i<5;i++)crc+=test[i];
	test[5] = crc&0xff;
	printf("zjStatus_crc:%02x\n",test[5]);
	print_buffer(test,6);
	send_message_to_uart(uart.fd_4, test, 6);
}

//******************************************ThdFeedBackLinkBB*********************************//
//ThdFeedBackLinkBB实现协议组包（故障反馈信息），并控制rcv_cmd_flag实现信息发送使能
//*******************************************************************************************//
void ThdFeedBackLinkBB(void)
{
	char buf[7];
	memset(buf,0,sizeof(buf));
	buf[0] = 0x7E;
	buf[1] = 6;
	buf[2] = 0x09;
	if(linkKzwjFlag == 1)
		buf[3] |= 0x10;                                 //与控制微机通信故障
    else 
        buf[3] = 0;
	buf[4] = malfunction;
	buf[5] = electrical_status;
	buf[6] = 0;
	for(int i=1;i<6;i++)
	{
		buf[6] += buf[i];
	}
	buf[6] &= 0xff;
	if((buf[3] != 0) || (buf[4] != 0) || (buf[5] != 0))//存在故障  2s周期发送
	{
		memset(snd_240Sv,0,sizeof(snd_240Sv));
		snd_240Sv[0] = buf[0];
		otp_encrypt_buf(&buf[1],6,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
        //UdpMessageSend(g_dt_udpsock, snd_240Sv, strlen(snd_240Sv), g_dt_serv_addr);
        QueSndCmd(snd_240Sv,strlen(snd_240Sv));
	}
}

int PingPC104(void)
{
	char failtimes=0;
	int  i = -1;
	char resultpingPC104;
	char buf2[4];
	
	int  file_fd;
	char buf[550];
	char *ptrf;
	char *ptrb;
	char fptr[50];
	const char *ptr = NULL;
	int n;
			
	system(Pingpc104_CMD);
	file_fd=open(pingfile,O_RDWR,644);
	if (file_fd < 0)
	{
		perror("open file fail!\n");
		write_log("open file PingPC104 fail\r\n");
	}
	
	read(file_fd,buf,sizeof(buf));	
	ftruncate(file_fd,0);		
	lseek(file_fd,0,SEEK_SET);
	close(file_fd);
	
	ptrf = strstr(buf,"100%");
	if(0 == ptrf)
	{
        //buf2[0] = 0x8A;
        //buf2[1] = 0x03;
        //buf2[2] = 0x05;
        //buf2[3] = 0x08;
        //memset(snd_240Sv, 0, sizeof(snd_240Sv));
        //snd_240Sv[0] = buf2[0];
        //otp_encrypt_buf(&buf2[1],3,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
        //rcv_cmd_flag = 1;
		return 1;	
	}	
	return 0;
}

int PingDT(void)
{
	char failtimes=0;
	int i = -1;
	char resultpingPC104;
	char buf2[4];
	
	int file_fd;
	char buf[550];
	char *ptrf;
	char *ptrb;
	char fptr[50];
	const char *ptr = NULL;
	int n;
					
	system(pingDT_CMD);
	file_fd=open(pingfile,O_RDWR,644);
	if (file_fd < 0)
	{
		perror("pingDT_CMD:open file fail!\n");
		write_log("open file pingDT_CMD fail\r\n");
	}
	
	read(file_fd,buf,sizeof(buf));	
	ftruncate(file_fd,0);		
	lseek(file_fd,0,SEEK_SET);
	close(file_fd);
	
	ptrf = strstr(buf,"100%");
	if(0 == ptrf)
	{
		buf2[0] = 0x8A;
		buf2[1] = 0x03;
		buf2[2] = 0x05;
		buf2[3] = 0x08;
		snd_240Sv[0] = buf2[0];
		otp_encrypt_buf(&buf2[1],3,&snd_240Sv[1],myencrypt->key,strlen(myencrypt->key),0);
		rcv_cmd_flag = 1;
		return 1;	
	}	
	return 0;
}

void tongXinTestScop(void)
{
    if(feedbackTimer >= 40)feedbackOpen = 1;
    if(feedbackOpen == 1)
    {
        testWxTX();
        if(feedbackErrorCount == 2){
            linkKzwjFlag =1;
            printf("kzwj communication is error\n");  
        } 
        if(feedbackOK == 1){
            feedbackErrorCount=0;
            feedbackOK=0;
            linkKzwjFlag = 0;
        }else feedbackErrorCount++;
    }
    feedbackTimer++;
}

void GetGNRMC_Uart2(void)
{
    int Ret;
	int i,j;
	char buf[1024] = {0};
	memset(buf, 0, 1024);
	pthread_mutex_lock(&mut);
	Ret = get_message_from_uart(uart.fd_2,buf,RNS_LEN); //length is 100
	pthread_mutex_unlock(&mut);
	if (Ret == -1)	
	{
		Uwater_status. tx_RNSS=0;
	}
	if(Ret > 1)
	{
		Uwater_status. tx_RNSS=1;		
		for(i=0;i<RNS_LEN;i++)
		{
			if((buf[i] == GNRMC[0]) && (buf[i+1] == GNRMC[1]) && (buf[i+2] == GNRMC[2]) && (buf[i+3] == GNRMC[3]) && (buf[i+4] == GNRMC[4]) && (buf[i+5] == GNRMC[5]))//$GNRMC
			{
				for(j=i;j<1024;j++)
				{
					if((buf[j] == 0x0d) && (buf[j+1] == 0x0a))
					{
						buf[j] = '\0';
						break;
					}
				}
				memset(rns_buf, 0, 1024);
				memcpy(rns_buf,&buf[i],j-i);					
				break;
			}
		}				
	}//end if
    return ;
}

void timer(int sig)
{
    char buf_snd[]={0x00,0x01,0x02};
    if(SIGALRM == sig)
    {
        timeCnt++;
        g_timeStopFlag++;
        alarm(1);    //we contimue set the timer
    } 
    return ;
}

void ustimer(int signo)
{
    if(SIGALRM == signo)
    {
        uscount++;
        if(uscount >= 1000){
            uscount = 0;
            timeCnt++;
            g_timeStopFlag++;
        }
        signal(SIGALRM,ustimer);
    } 
    return ;
}

int main(void)
{
	int  GpsOrTime  = 0;
	int  MountOk    = 0;
    char str[1024]  = {0};
    char systemTime[30];
	open_global_shm();          //貌似内存分配
	log_file();                 //貌似创建log文件存储路径
	dev_init();
    QueCreateNew();
    printf("client main:---------V1.0.1\n");
    sprintf(systemTime,"date -s %c%s%c",'"',"2023-07-01 07:01:01",'"');
    system(systemTime);         //设置系统时间
    system("ip addr");          //查看本机ip
	system("mkdir /root/LFile");
	pthread_mutex_init(&mut,NULL);
    pthread_mutex_init(&udp,NULL);
    pthread_mutex_init(&que,NULL);
    system("echo 1 > /sys/class/leds/led1/brightness");//初始化电平拉低等待接收uart7
    /*********************************************************************************************/
    pthread_create((pthread_t *)&(QueTimerProcess_t),NULL,(void*)(QueTimerProcess), (void*) (NULL));      //定时处理队列
    pthread_create((pthread_t *)&(ThdConn8001_t),    NULL,(void*)(UdpConn8001),     (void*) (NULL));      //电台转发
    pthread_create((pthread_t *)&(ThdParsePC_Udp_t), NULL,(void*)(ThdParsePC_Udp),  (void*) (NULL));	  //与控制微机交互指令(UDP方式)
    pthread_create((pthread_t *)&(ThdU8001Process_t),NULL,(void*)(ThdU8001Process), (void*) (NULL));	  //文件传输过程中处理控制指令
    pthread_create((pthread_t *)&(ThdYTJ_Uart8_t),   NULL,(void*)(ThdYTJ_Uart8),    (void*) (NULL));      //BDYTJ通信测试线程
    pthread_create((pthread_t *)&(ThdLora_Uart7_t),  NULL,(void*)(ThdLora_Uart7),   (void*) (NULL));      //Lora转发北斗短报文GPS&Time信息
    /*********************************************************************************************/
	signal(SIGALRM, timer);     //relate the signal and function 
    alarm(1);                   //trigger the timer
	while(1)
	{
        if(timeCnt>10)
        {
            if (!MountOk)
            {
                MountOk = MountUser();
                //system("arping -c 4 192.168.168.34");
                QueSndCmd("ko every body", 2); //电台通信链接建立响应，只发一次，解决只重启cpu时与对端通信无法建立的问题
            }
        }
		if(timeCnt >= counter_2Hz)
		{
			counter_2Hz = timeCnt+2;
		    printf("this client program is running  :%d --- %d\r\n",g_count,g_sencunt);
            memset(str,0,1024);
            sprintf(str,"%s%d --- %d%s","this is client program is running  :",g_count,g_sencunt,"\r\n");
            write_log(str);
			if(g_errorStatuTxFlag != 1)ThdFeedBackLinkBB();
		}

		if(timeCnt >= counter_3Hz)//通信测试 by zqw 20221012
		{
			counter_3Hz = timeCnt+3;
            tongXinTestScop();
		}
			
		if(timeCnt >= counter_20Hz)
		{
			counter_20Hz = timeCnt+20;
            if(countstop != g_count)countstop = g_count;
            else {
                g_sencunt    = 0;
                g_count      = 0;
            }
		}	

		if(timeCnt >= counter_5Hz)
		{
			counter_5Hz = timeCnt+5;			
			if (GpsOrTime)
			{
				SendGpsToBB_Udp8010(rns_buf);
				
			}else{
				SendTimToBB_Udp8010(rns_buf);
				
			}
			GpsOrTime = !GpsOrTime;
		}	

		if(system_para->watchdog == 1)
		{
			sleep(2);
			system_para->monitor ++;
		}
		else
			usleep(200);

	}
	close_file();
	printf("quit main!!!!!\n");
	return 0;
	
}

void Uwater_status_init()
{
	Uwater_status. SIMcard=2;
	Uwater_status. tx_RNSS=2;			
	Uwater_status. tx_RDSS=2;			
	Uwater_status. DT_uart=2;			
	Uwater_status. DT_100Base=2;		
	Uwater_status. external_Uart=2;		
	Uwater_status. external_100base=2;	
	return;
}

void Uwater_status_check(void)
{
	SIMcard_check();	
	external_100base_check();
	//DT_100base_check();
	//PowerMode_Select();
	return;
	
}
void SIMcard_check(void)
{
	int reg = -1;	
	char buf[] = {0x24,0x49,0x43,0x4A,0x43,0x00,0x0C,0x00,0x00,0x00,0x00,0x2B};//$ICJC
	//rds_buf[7] = 0;
	//rds_buf[8] = 0;
	//rds_buf[9] = 0;
	reg = send_message_to_uart(uart.fd_3,buf,sizeof(buf));
	if(reg == -1)
	{
		write_log("send message SIMcard_check error\r\n");
		printf("send message SIMcard_check error!\n");
	}

	return;
}

void external_100base_check(void)
{
	//对外部网络连接进行ping测试
	int status;
	status=PingPC104();
	if(status==1)
		Uwater_status. external_100base=1;
	else
		Uwater_status. external_100base=0;
	return;  
}
void DT_100base_check(void)
{
	//对电台网络连接进行ping测试
	int status;
	status=PingDT();
	if(status==1)
		Uwater_status. DT_100Base=1;
	else
		Uwater_status. DT_100Base=0;
	return;
}

void protocol_struct2string(struct protocol pro )
{
	int len;
	memset(sendbuff, 0, 100);
	sendbuff[0]=pro.head;
	sendbuff[1]=pro.length;
	sendbuff[2]=pro.type;
	Status_struct2string((pro.content));
	strcat(sendbuff,protocol_str_tmp);
	strcat(sendbuff,&(pro.crc));
	len=sizeof(sendbuff);
	//print_buffer(sendbuff,100);
	//printf("protocol_struct2string-----sendbuff:%s\n",sendbuff);
	//printf("sendbuff length %d\n",len);
	return;
}

void Status_struct2string(struct Uwater st)
{
	memset(protocol_str_tmp, 0, 100);
	protocol_str_tmp[0]=st.SIMcard;			    //sim卡在位情况 ，   	1：卡在位，  0：卡异常
	protocol_str_tmp[1]=st.tx_RNSS;			    //RNSS串口通信情况  	1：通信正常  0：通信异常
	protocol_str_tmp[2]=st.tx_RDSS;			    //RDSS串口通信情况  	1：通信正常  0：通信异常
	protocol_str_tmp[3]=st.DT_uart;			    //电台串口通信情况
	protocol_str_tmp[4]=st.DT_100Base;		    //电台网口通信情况
	protocol_str_tmp[5]=st.external_Uart;		//外部串口通信情况
	protocol_str_tmp[6]=st.external_100base;	//外部网口通信情况
	//print_buffer(protocol_str_tmp,100);
	return;
}

void PowerMode_Select(void)
{	
	printf("depth is :%d\r\n",depth);	
	if(depth >= 25)
	{//水深1米情况下，关闭电源进入低功耗模式。
		if((snd_beidou_addr_info == 0) || (snd_beidou_addr_info == 2))
		{
			printf("depth>25\n");
			snd_beidou_addr_info = 1;  //取消RD短报文发送使能信号
			diantai_power_control(0);  //关闭电台电源
		}
	}
	else
	{ //对电台电源进行上电，并打开短报文发送使能信号			
		if(snd_beidou_addr_info == 1)
		{
			printf("diantai_power on\n");
			diantai_power_control(1);
			sleep(50);
			snd_beidou_addr_info = 0;								
		}
		else if(snd_beidou_addr_info == 2)
		{
			printf("diantai_power on\n");
			diantai_power_control(1);
			snd_beidou_addr_info = 0;
		}
	}
	return ;

}

/************************************************/
int UdpMessageSend(int sock,char* strmsg,int msglen,struct sockaddr_in serv_addr)
{
    char recvbuf[1024];
    struct sockaddr_in clnt_addr;
    int  clnt_addr_size = sizeof(clnt_addr);
    int  ReSendtimes = 0;
    int  sendlen = 0;
    int  ret=0;
    char recv_ack=0;
    /****************/
    while (1)
    {	
        pthread_mutex_lock(&udp);
        int len = sendto(sock, strmsg, msglen, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        pthread_mutex_unlock(&udp);
        return ret;

        if(ReSendtimes>1000)
        {
            ret=-1;
            printf("resend times =10--break\n");
            break;
        }
        printf("udp send message:\n");
        print_buffer(strmsg, msglen);
        ReSendtimes++;
        memset(recvbuf, 0, 1024);
        int   len1= recvfrom(sock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);//10 ms后返回
        if(strstr(recvbuf,"ko"))
        {
            printf("recv ack --send success\n");
            return ret;
        }else{
            printf("udpmessagesend:wait ack:wrong ack\n");
            usleep(1000*10);
            continue;
        }

        //break;
    }
   return ret;
}
/************************************************/
int UdpMessageRecv(int sock,char* recvbuf,int buflen,struct sockaddr_in  serv_addr)
{
	int  ret=0;
    struct sockaddr_in clnt_addr;
    int clnt_addr_size = sizeof(struct sockaddr_in);
//    g_timeStopFlag = 0;
//    while (g_timeStopFlag<10)
//    {
		int len = recvfrom(sock, recvbuf, buflen, 0, (struct sockaddr*)&clnt_addr, &clnt_addr_size);//超时就返回
		if (len > 0)
		{
			if(len<=1)
			{
				return 0;
			}else{
                //回复确认包
			    //sendto(sock, "ko", 2, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
                ret = len;
                //break;
            }
     	}
//    }
    return ret;
}

//int bufLen = 1024;
//int dataLen = 1019;

//int bufLen = 25;
//int dataLen = 20;


//void UdpFileRecv(int sndsock, struct sockaddr_in clnt_addr, char* filename)
//{
//	if (filename == "")
//	{
//		filename = "./testfile_recv";
//	}
//	else if (strlen(filename) > 248)
//	{
//		printf("filename len>248 return\n");
//		sleep(3);
//		return;
//	}
//	char sndbuf[bufLen];
//	char recvbuf[bufLen];
//	int  count = 0;
//    int resendCount = 0;
//    char decrypt_buf[32];
//
//	char strcmd[256];
//	memset(strcmd, 0, 256);
//	sprintf(strcmd, "rm -f %s", filename);
//	system(strcmd);
//	FILE* fp = fopen(filename, "ab+");
//	while(1)
//	{
//		memset(recvbuf, 0, bufLen);
//        int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
//        printf("recv:");
//        print_buffer(recvbuf, 10);
//        count++;
//        if (count == 1000)
//        {
//            //fwrite(recvbuf + 5, len - 5, 1, fp);
//            //printf("recv file:%d \n", count);
//            count = 0;
//            break;
//        }
//	}
//}


void UdpFileRecv(int sndsock, struct sockaddr_in clnt_addr, char* filename)
{
	if (filename == "")
	{
		filename = "./testfile_recv";
	}
	else if (strlen(filename) > 248)
	{
		printf("filename len>248 return\n");
		sleep(3);
		return;
	}
	char sndbuf[bufLen];
	char recvbuf[bufLen];
	int  count = 0;
    int resendCount = 0;
    int resendAckCount   = 0;
    char decrypt_buf[32];

	char strcmd[256];
	memset(strcmd, 0, 256);
	sprintf(strcmd, "rm -f %s", filename);
	system(strcmd);
	FILE* fp = fopen(filename, "ab+");
	while(1)
	{
		memset(recvbuf, 0, bufLen);
        int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
		if (len > 0)
		{
			if (len < 5)
			{
				printf("file recv:len<5 continue\n");
				continue;
			}
			if (recvbuf[0] != 1)
			{
				printf("no file packet:%d ---- need count:%d\n", recvbuf[0], count);
				continue;
			}
			int count_recv = 0;
			char* p = recvbuf;
			memcpy(&count_recv, p + 1, sizeof(count_recv));
			if (count == 0)
			{
				count = count_recv;
				fwrite(recvbuf + 5, len - 5, 1, fp);
				printf("recv file:%d \n", count);
			}
			else if (count != count_recv + 1)
			{
			    if(count == count_recv){
                    printf("need resend ack\n");
                    if(resendAckCount > 10)
                        return;
                    resendAckCount++;
                }
                else{
    				printf("udp file recv error packet:%d---\n", count_recv);
    				continue;
                }
			}
			else 
			{
				count = count_recv;
				fwrite(recvbuf + 5, len - 5, 1, fp);
				printf("recv file packet:%d\n", count);
			}
		}
		else
		{
    		printf("udp file recv timeout ------continue\n");
            resendCount ++;
    		if(resendCount <1000)
            {
                resendCount = 0;
                usleep(1000);
                continue;
            }
		}
        memset(sndbuf, 0, bufLen);
		sndbuf[0] = 1;
		char* p1 = sndbuf;
		memcpy(p1 + 1, &count, sizeof(count));
        sendto(sndsock, sndbuf, 5, 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
		printf("send file ack:%d --- %d\n", count);
		if (count == 1)
		{
			if (fp != 0)
			{
				fclose(fp);
				fp = 0;
				printf("file recv complete\n");
			}
			printf("file recv  return\n");
			break;
		}
	}
}

//void UdpFileRecv(int sndsock, struct sockaddr_in clnt_addr, char* filename)
//{
//	if (filename == "")
//	{
//		filename = "./testfile_recv";
//	}
//	else if (strlen(filename) > 248)
//	{
//		printf("filename len>248 return\n");
//		sleep(3);
//		return;
//	}
//	char sndbuf[bufLen];
//	char recvbuf[bufLen];
//	int  count = 0;
//    int  resendCount = 0;
//    char decrypt_buf[32];
//
//	char strcmd[256];
//	memset(strcmd, 0, 256);
//	sprintf(strcmd, "rm -f %s", filename);
//	system(strcmd);
//	FILE* fp = fopen(filename, "ab+");
//	while(1)
//	{
////		memset(recvbuf, 0, bufLen);
////        int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
//		if (len_f > 0)
//		{
//			if (len_f < 5)
//			{
//				printf("file recv:len<5 continue\n");
//				continue;
//			}
//			if (recvbuf_f[0] != 1)
//			{
//				printf("no file packet:%d ---- need count:%d\n", recvbuf_f[0], count);
//                if((recvbuf_f[0]==0x7e)||(recvbuf_f[0]==0x7f)||(recvbuf_f[0]==0x7c)||(recvbuf_f[0]==0x8a))
//                {
//                    g_lenth = 0;
//                    memset(g_readBuf,0,1024);
//                    printf("g_readBuf_x: len:%d\n",len_f);
//                    print_buffer(recvbuf_f,32);
//                    
//                    g_lenth = ((recvbuf_f[1]-0xb0)&0xff)+1;
//                    
//                    printf("g_readBuf_r: len:%d\n",g_lenth);
//                    print_buffer(recvbuf_f,g_lenth);
//                    
//                    memcpy(g_readBuf,recvbuf_f,g_lenth);
//                    printf("g_readBuf: len:%d\n",g_lenth);
//                    print_buffer(g_readBuf,g_lenth);
//                }
//				continue;
//			}
//			int count_recv = 0;
//			char* p = recvbuf_f;
//			memcpy(&count_recv, p + 1, sizeof(count_recv));
//			if (count == 0)
//			{
//				count = count_recv;
//				fwrite(recvbuf_f + 5, len_f - 5, 1, fp);
//				printf("recv file:%d \n", count);
//			}
//			else if (count != count_recv + 1)
//			{
//				printf("udp file recv error packet:%d---\n", count_recv);
//				continue;
//			}
//			else
//			{
//				count = count_recv;
//				fwrite(recvbuf_f + 5, len_f - 5, 1, fp);
//				printf("recv file packet:%d\n", count);
//			}
//		}
//		else
//		{
//    		printf("udp file recv timeout ------continue\n");
//            resendCount ++;
//    		if(resendCount <10){
//                resendCount = 0;
//                continue;
//             }
//		}
//        memset(sndbuf, 0, bufLen);
//		sndbuf[0] = 1;
//		char* p1 = sndbuf;
//		memcpy(p1 + 1, &count, sizeof(count));   
//        sendto(sndsock, sndbuf, 5, 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
//		printf("send file ack:%d --- %d\n", count,fileRecvTime);
//        fileRecvTime = 0;
//		if (count == 1)
//		{
//			if (fp != 0)
//			{
//				fclose(fp);
//				fp = 0;
//				printf("file recv complete\n");
//			}
//			printf("file recv  return\n");
//			break;
//		}
//	}
//}

/*******************************************/
void UdpFileSend(int sndsock, struct sockaddr_in serv_addr, char* filename)
{
	if (filename == "")//可删除
	{
		filename = "./testfile";
	}
	else if (strlen(filename) > 248)
	{
		printf("filename len>248 return\n");
		sleep(3);
		return;
	}
	/************************************/
	FILE* fp = fopen(filename, "rb+");
	if (!fp)
	{
		printf("file not found\n");
		sleep(1);
		return;
	}
	/************************************/
	fseek(fp, 0, SEEK_END);//扫描文件
	int length = ftell(fp);//获取文件长度
	rewind(fp);//复位扫描
	/************************************/
	int count = length / dataLen;
	if (length % dataLen > 0)count += 1;
	char sndbuf[bufLen];
	char recvbuf[bufLen];
	int  ReSendtimes = 0;
	int  sendlen = 0;
	/************************************/
	while (count > 0)
	{
		if (sendlen > length)
		{
			printf("sendlen > length :error\n");
		}
		/************************************/
		memset(sndbuf, 0, bufLen);
		int len1 = length - sendlen > dataLen ? dataLen : length - sendlen;
		fseek(fp, sendlen, SEEK_SET);  //移动文件指针，每发送一帧移动一次
		fread(sndbuf + 5, len1, 1, fp);//读取下帧文件
		/************************************/
		sndbuf[0] = 1;
		char* p = sndbuf;
		memcpy(p + 1, &count, sizeof(count));//(倒序)放入序号
        int len = sendto(sndsock, sndbuf, len1 + 5, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
        /************************************/
        pthread_mutex_lock(&udp);
        if(flagSend == 0){
            flagSend = 1;
    		int len = sendto(sndsock, sndbuf, len1 + 5, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
            flagSend = 0;
        }       
        else {
            pthread_mutex_unlock(&udp);
            continue ;
        }
        pthread_mutex_unlock(&udp);
		/************************************/
		printf("send file packet:%d\n", count);
		if (ReSendtimes < 100)ReSendtimes++;
        if(ReSendtimes > 30){
            printf("file pack send fail\n");
            return;
        }
		if (count == 1 && ReSendtimes == 10) //last packet
		{
			if (fp != 0)
			{
				fclose(fp);
				fp = 0;
			}
			printf("file send complete and return\n");
			return;
		}
		/************************************/
		while (1)
		{
			memset(recvbuf, 0, bufLen);
            len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, 0, 0);
			if (len < 0)
			{
				break;
			}
			else if (len < 5)
			{
				printf("file ack recv: len<5 ---continue\n");
				continue;
			}
			/************************************/
			int count_ack = 0;
			char* p1 = recvbuf;
			memcpy(&count_ack, p1 + 1, sizeof(count_ack));
			if (len == 5 && count_ack == (count) && (unsigned char)recvbuf[0] == 1)
			{
				count--;
				ReSendtimes = 0;
				sendlen += len1;
				break;
			}
			else//wrong ack
			{
				//recv wrong ack
				if ((unsigned char)recvbuf[0] != 1)
				{
					printf("recv wrong file header:%d\n", recvbuf[0]);
				}
				else if (len == 5)
				{
					printf("recv wrong file packet:%d\n", count_ack);
				}
				else
				{
					printf("recv wrong file packet:len!=5\n");
				}
			}
		}
	}
	printf("file send complete and return\n");
}

//void UdpFileSend(int sndsock, struct sockaddr_in serv_addr, char* filename)
//{
//	if (filename == "")//可删除
//	{
//		filename = "./testfile";
//	}
//	else if (strlen(filename) > 248)
//	{
//		printf("filename len>248 return\n");
//		sleep(3);
//		return;
//	}
//	/************************************/
//	FILE* fp = fopen(filename, "rb+");
//	if (!fp)
//	{
//		printf("file not found\n");
//		sleep(1);
//		return;
//	}
//	/************************************/
//	fseek(fp, 0, SEEK_END);              //扫描文件
//	int length = ftell(fp);              //获取文件长度
//	rewind(fp);                          //复位扫描
//	if(length <= 0)printf("%s file is empty\r\n",filename);
//	/************************************/
//	int count = length / dataLen;
//	if (length % dataLen > 0)count += 1;
//	char sndbuf[bufLen];
//	char recvbuf[bufLen];
//	int  ReSendtimes = 0;
//	int  sendlen     = 0;
//    int  resendCount[10];
//    int  s_count     = count;
//    int  loopcount   = 0;
//	/************************************/
//	while (count > 0)
//	{
//		if (sendlen > length)
//		{
//			printf("sendlen > length :error\n");
//		}
//		/************************************/
//		memset(sndbuf, 0, bufLen);
//		int len1 = length - sendlen > dataLen ? dataLen : length - sendlen;
//		fseek(fp, sendlen, SEEK_SET);           //移动文件指针，每发送一帧移动一次        
//		fread(sndbuf + 5, sizeof(char), len1, fp); //读取下帧文件
//		/************************************/
//		sndbuf[0] = 1;
//		char* p = sndbuf;
//		memcpy(p + 1, &count, sizeof(count));   //(倒序)放入序号
//		if(count == 600)printf("---sendlen:%d\r\n",sendlen);
//		/************************************/
//        pthread_mutex_lock(&udp);
////        if(flagSend == 0){
////            flagSend = 1;
//    		int len = sendto(sndsock, sndbuf, len1 + 5, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//            //usleep(20000);
////            flagSend = 0;
////        }else {
////            pthread_mutex_unlock(&udp);
////            continue ;
////        }
//        pthread_mutex_unlock(&udp);
//        usleep(20000);
//		/************************************/
//		printf("send file packet:%d\n", count);
//        if(count != 1){
//            count--;
//            sendlen += len1;
//            continue;
//        }
//		/************************************/
//		while (1)
//		{
//			memset(recvbuf, 0, bufLen);
//            if(loopcount >= 10){
//                printf("recv wait timeout will be return\r\n");
//                timeoutBreak = 1;
//                return;
//            }
//			int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, 0, 0);
//			/************************************/
//            if(len > 0)loopcount = 0;
//			if (strstr(recvbuf,"sucess"))
//			{
//			    printf("file send sucess\r\n");
//			}else if(recvbuf[0] == 0xfd&&recvbuf[1] > 0){
//                printf("need resend count:");
//                print_buffer(recvbuf, recvbuf[1]*2+2);
//
//                for(int i=0;i<recvbuf[1];i++)
//                    resendCount[recvbuf[1]-1-i] = (recvbuf[2+i*2]<<8|recvbuf[2+i*2+1]);
//                
//                for(int i=0;i<recvbuf[1];i++){
//                    sendlen = (s_count-resendCount[i])*1019;
//                    fseek(fp, sendlen, SEEK_SET);                       //移动文件指针，每发送一帧移动一次
//                    fread(sndbuf + 5, sizeof(char), 1019, fp);          //读取下帧文件
//                    sndbuf[0] = 1;
//                    char* p = sndbuf;
//                    memcpy(p + 1, &resendCount[i], 4);                  //(倒序)放入序号
//                    int len = sendto(sndsock, sndbuf, 1019 + 5, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//                    //print_buffer(sndbuf, 1019 + 5);
//                    usleep(20000);
//                }
//            }else if(strstr(recvbuf,"fail")){
//                printf("file recv fail program will return\r\n");
//            }
//            if(count == 1){
//                if(fp != 0){
//                    fclose(fp);
//                    fp = 0;
//                }
//                printf("file send complete and return\n");
//                return;
//            }
//            loopcount++;
//		}
//    }
//}


//非一问一答
//void UdpFileRecv(int sndsock, struct sockaddr_in clnt_addr, char* filename)
//{
//	if (filename == "")
//	{
//		filename = "./testfile_recv";
//	}
//	else if (strlen(filename) > 248)
//	{
//		printf("filename len>248 return\n");
//		sleep(3);
//		return;
//	}
//	char sndbuf[bufLen];
//	char recvbuf[bufLen];
//	int  count = 0;
//    int  resendCount = 0,resendTimes = 0;
//    char decrypt_buf[32];
//    int  errorCount[500];//丢包超过500个则判fail
//    int  icount = 0,s_count = 0;
//	char strcmd[256];
//    int  rewriteTimes = 0;
//    char loopcount = 0;
//    char re_reTimeOut = 0;
//	memset(strcmd, 0, 256);
//	sprintf(strcmd, "rm -f %s", filename);
//	system(strcmd);
//	FILE* fp = fopen(filename, "ab+");
//	while(1)
//	{
//		memset(recvbuf, 0, bufLen);
//        if(loopcount >= 10){
//            printf("recv wait timeout will be return\r\n");
//            timeoutBreak = 1;
//            return;
//        }
//        int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
//		if (len > 0)
//		{
//		    loopcount = 0;
//			if (len < 5)
//			{
//				printf("file recv:len<5 continue\n");
//				continue;
//			}
//			if (recvbuf[0] != 1)
//			{
//				printf("no file packet:%d ---- need count:%d\n", recvbuf[0], count);
//                if((recvbuf[0]==0x7e)||(recvbuf[0]==0x7f)||(recvbuf[0]==0x7c)||(recvbuf[0]==0x8a))
//                {
//                    g_lenth = 0;
//                    memset(g_readBuf,0,1024);               
//                    g_lenth = ((recvbuf[1]-0xb0)&0xff)+1;
//                    memcpy(g_readBuf,recvbuf,g_lenth);
//                    printf("g_readBuf: len:%d\n",g_lenth);
//                    print_buffer(g_readBuf,g_lenth);
//                    g_count++;
//                    QueSndCmd("ko", 2);
//                }
//				continue;
//			}
//            /******************************************************/
//			int count_recv = 0;
//			char* p = recvbuf;
//			memcpy(&count_recv, p + 1, sizeof(count_recv));            
////            if(count_recv == 600||count_recv == 599||count_recv == 596||count_recv == 592||count_recv == 591){
////                continue;
////            }
//			if (count == 0)
//			{
//				count = count_recv;
//                s_count = count;
//				fwrite(recvbuf + 5, sizeof(char), len-5, fp);
//				printf("recv file:%d \n", count);
//			}
//            else if (count != count_recv + 1)
//            {
//				printf("udp file recv error packet:%d---\n", count_recv);          
//                rewriteTimes = count-(count_recv+1);
//                for(int i=0;i<rewriteTimes;i++)
//                {
//                    fwrite(recvbuf + 5, sizeof(char), len-5, fp);
//                    errorCount[icount] = (count_recv+1)+i;
//                    icount++;
//                }
//                count = count_recv;
//                fwrite(recvbuf + 5, sizeof(char), len-5, fp);
//			}
//            else
//            {
//				count = count_recv;
//				fwrite(recvbuf + 5, sizeof(char), len-5, fp);
//				printf("recv file packet:%d\n", count);
//			}
//		}    
//        /******************************************************/
//		if (count == 1)
//		{
//		    if (fp != 0)
//			{
//				fclose(fp);
//				fp = 0;
//                printf("file recv complete1\n");
//			}
//            if(icount > 0)
//            {
//                if(icount >= 255){
//                    printf("file recv fail program will return\r\n");
//                    sendto(sndsock, "fail", 4, 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
//                    return;
//                }
//                memset(sndbuf, 0, bufLen);
//                sndbuf[0] = 0xfd;
//                sndbuf[1] = icount;
//                for(int i=0;i<icount;i++){
//                    sndbuf[2+i*2]   = errorCount[i]>>8;
//                    sndbuf[2+i*2+1] = errorCount[i]&0x00ff;
//                }
//                sendto(sndsock, sndbuf, icount*2+2, 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
//                printf("recvErrorCount:");
//                print_buffer(sndbuf, icount*2+2);
//                
//                FILE* fp1 = fopen(filename, "r+");
//                if(fp1 != NULL)
//                {
//                    for(int i=0;i<icount;i++)
//                    {
//                        memset(recvbuf, 0, bufLen);
//                        int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, NULL, NULL);
//                        if(len > 0){
//                            int count_recv = 0;
//                            char* p = recvbuf;
//                            re_reTimeOut = 0;
//                            memcpy(&count_recv, p + 1, sizeof(count_recv));
//                            fseek(fp1, (s_count-count_recv)*dataLen, SEEK_SET);
//                            fwrite(recvbuf + 5, sizeof(char), len-5, fp1);
////                            print_buffer(recvbuf, 8);
////            				printf("re_recv file:%d -- %d \n", count_recv,s_count);
//                        }else{
//                            if(re_reTimeOut >= 3){
//                                printf("file_re_revive timeout\r\n");
//                                break;
//                            }
//                            re_reTimeOut ++;
//                        }
//                    }
//                }
//                else 
//                    printf("file open fail\r\n");
//                if (fp1 != 0)
//                {
//                    fclose(fp1);
//                    fp1 = 0;
//                    printf("file recv complete\n");
//                    return;
//                }
//            }
//			else {
//                sendto(sndsock, "sucess", 6, 0, (struct sockaddr*)&clnt_addr, sizeof(clnt_addr));
//                printf("file recv  return\n");
//                break;
//            }
//		}
//        loopcount++;
//	}
//}
//
//
//void UdpFileSend(int sndsock, struct sockaddr_in serv_addr, char* filename)
//{
//    clock_t start = clock();
//
//	if (filename == "")//可删除
//	{
//		filename = "./testfile";
//	}
//	else if (strlen(filename) > 248)
//	{
//		printf("filename len>248 return\n");
//		sleep(3);
//		return;
//	}
//	/************************************/
//	FILE* fp = fopen(filename, "rb+");
//	if (!fp)
//	{
//		printf("file not found\n");
//		sleep(1);
//		return;
//	}
//	/************************************/
//	fseek(fp, 0, SEEK_END);              //扫描文件
//	int length = ftell(fp);              //获取文件长度
//	rewind(fp);                          //复位扫描
//	if(length <= 0)printf("%s file is empty\r\n",filename);
//	/************************************/
//	int count = length / dataLen;
//	if (length % dataLen > 0)count += 1;
//	char sndbuf[bufLen];
//	char recvbuf[bufLen];
//	int  ReSendtimes = 0;
//	int  sendlen     = 0;
//    int  resendCount[10];
//    int  s_count     = count;
//    int  loopcount   = 0;
//	/************************************/
//	while (count > 0)
//	{
//		if (sendlen > length)
//		{
//			printf("sendlen > length :error\n");
//		}
//		/************************************/
//		memset(sndbuf, 0, bufLen);
//		int len1 = length - sendlen > dataLen ? dataLen : length - sendlen;
//		fseek(fp, sendlen, SEEK_SET);           //移动文件指针，每发送一帧移动一次        
//		fread(sndbuf + 5, sizeof(char), len1, fp); //读取下帧文件
//		/************************************/
//		sndbuf[0] = 1;
//		char* p = sndbuf;
//		memcpy(p + 1, &count, sizeof(count));   //(倒序)放入序号
//		if(count == 600)printf("---sendlen:%d\r\n",sendlen);
//		/************************************/
//        QueSndFile(sndbuf,len1+5);
//        usleep(20000);
//		/************************************/
//		printf("send file packet:%d\n", count);
//        if(count != 1){
//            count--;
//            sendlen += len1;
//            continue;
//        }
//		/************************************/
//		while (1)
//		{
//			memset(recvbuf, 0, bufLen);
//            if(loopcount >= 5){
//                printf("recv wait timeout will be return\r\n");
//                timeoutBreak = 1;
//                if(count == 1)
//                {
//                    if(fp != 0){
//                        fclose(fp);
//                        fp = 0;
//                    }
//                    printf("file send complete and return\n");
//                    return;
//                }
//                return;
//            }
//			int len = recvfrom(sndsock, recvbuf, sizeof(recvbuf), 0, 0, 0);
//			/************************************/
//            if(len > 0)loopcount = 0;
//			if (strstr(recvbuf,"sucess"))
//			{
//			    printf("file send sucess\r\n");
//			}else if(recvbuf[0] == 0xfd&&recvbuf[1] > 0){
//                printf("need resend count:");
//                print_buffer(recvbuf, recvbuf[1]*2+2);
//
//                for(int i=0;i<recvbuf[1];i++)
//                    resendCount[recvbuf[1]-1-i] = (recvbuf[2+i*2]<<8|recvbuf[2+i*2+1]);
//                
//                for(int i=0;i<recvbuf[1];i++){
//                    sendlen = (s_count-resendCount[i])*dataLen;
//                    fseek(fp, sendlen, SEEK_SET);                       //移动文件指针，每发送一帧移动一次
//                    fread(sndbuf + 5, sizeof(char), dataLen, fp);          //读取下帧文件
//                    sndbuf[0] = 1;
//                    char* p = sndbuf;
//                    memcpy(p + 1, &resendCount[i], 4);                  //(倒序)放入序号
//                    QueSndFile(sndbuf,bufLen);
//                    //int len = sendto(sndsock, sndbuf, 1019 + 5, 0, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
//                    print_buffer(sndbuf, 8);
//                    usleep(20000);
//                }
//            }else if(strstr(recvbuf,"fail")){
//                printf("file recv fail program will return\r\n");
//            }else {
//                loopcount++;
//                continue;
//            }
//            if(count == 1){
//                if(fp != 0){
//                    fclose(fp);
//                    fp = 0;
//                }
//
//                clock_t end = clock();
//                double elapsed_time = (double)(end - start) / CLOCKS_PER_SEC;
//                printf("codeTime %.2f s\n", elapsed_time);
//                
//                printf("file send complete and return\n");
//                return;
//            }
//            loopcount++;
//		}
//    }
//}


