/*forlinx serial port test*/
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <signal.h>
#include "../include/globa.h"
#define msleep(n) usleep(n*1000)

char test[100]="forlinx uart test.....bz";
volatile	int fd;
char* dev   = NULL;

pthread_mutex_t mut;
fd_set rd;
int nread,retval;
unsigned char msg[14];
struct timeval timeout={0,100};

volatile	pthread_t thread[2]; 
volatile	const int READ_THREAD_ID = 0;  
volatile	const int SEND_THREAD_ID = 1;  
volatile	int COM_READ_STATU = 1;  
volatile	int COM_SEND_STATU = 1; 

volatile	int sendnum=0,sendnum_times=0,recenum=0,recenum_times=0,cornum=0,cornum_times=0;

static speed_t getBaudrate(int baudrate)
{
	switch(baudrate) {
	case 0: return B0;
	case 50: return B50;
	case 75: return B75;
	case 110: return B110;
	case 134: return B134;
	case 150: return B150;
	case 200: return B200;
	case 300: return B300;
	case 600: return B600;
	case 1200: return B1200;
	case 1800: return B1800;
	case 2400: return B2400;
	case 4800: return B4800;
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	case 230400: return B230400;
	case 460800: return B460800;
	case 500000: return B500000;
	case 576000: return B576000;
	case 921600: return B921600;
	case 1000000: return B1000000;
	case 1152000: return B1152000;
	case 1500000: return B1500000;
	case 2000000: return B2000000;
	case 2500000: return B2500000;
	case 3000000: return B3000000;
	case 3500000: return B3500000;
	case 4000000: return B4000000;
	default: return -1;
	}
}



int OpenDev(int id)
{
	speed_t speed;
	char *Dev;
	int tmp;
    int i=0;
    int fdt,c=0,num;
    struct termios oldtio,newtio;
	
	if(id == 1)
	{
		tmp = 115200;
		Dev = "/dev/ttymxc0";
	}else if(id == 2)
	{
		tmp = 9600;
		Dev = "/dev/ttymxc1";
	}else if(id == 3)
	{
		tmp = 115200;
		Dev = "/dev/ttymxc2";
	}else if(id == 4)
	{
		tmp = 115200;
		Dev = "/dev/ttymxc3";
	}else if(id == 5)
	{
		tmp = 9600;
		Dev = "/dev/ttymxc4";
	}else if(id == 7)
	{
		tmp = 9600;
		Dev = "/dev/ttymxc6";
	}else if(id == 8)
	{
		tmp = 9600;
		Dev = "/dev/ttymxc7";
	}
    speed = getBaudrate(tmp);
    fdt=open(Dev,O_RDWR | O_NONBLOCK| O_NOCTTY | O_NDELAY);
    if(fdt<0)
    {
        perror(Dev);
        exit(1);
    }
    //save to oldtio
    tcgetattr(fdt,&oldtio);
    //clear newtio
    bzero(&newtio,sizeof(newtio));
    //newtio.c_cflag = speed|CS8|CLOCAL|CREAD|CRTSCTS;
    newtio.c_cflag = speed|CS8|CLOCAL|CREAD;
    newtio.c_iflag = IGNPAR;  
    newtio.c_oflag = 0;
    //printf("newtio.c_cflag=%x\n",newtio.c_cflag);
    tcflush(fdt,TCIFLUSH);  
    tcsetattr(fdt,TCSANOW,&newtio);  
    tcgetattr(fdt,&oldtio);
    //printf("oldtio.c_cflag=%x\n",oldtio.c_cflag);
    return fdt;
}

int read_port(int uart_id,char *msg,int len)
{   
	int readlen = 0;
	char * ptr;  
	int ret;
	ptr = msg; 
    FD_ZERO(&rd);
    FD_SET(uart_id,&rd);
   // timeout.tv_sec = 1;
   // timeout.tv_usec = 0;
	while(readlen < len){
		retval = select (uart_id+1,&rd,NULL,NULL,&timeout); 
		//printf("retval:%d\n",retval);
		
		 if(retval == -1)                              //æ£€æµ‹å¤±è´?  
        {  
            perror("select:");  
            break;  
        }  
        else if(retval > 0)            //æ£€æµ???     
        {  
		
            ret = read(uart_id,ptr,1);  
			//printf("ret:%d\n",ret);
            if(ret < 0)  
            {  
				printf("uart_id:%d\n",uart_id);
                perror("uart read err:");  
                break;  
            }  
            else if(ret == 0)  
                break;  
            //printf("ret:%d\n",ret);
            readlen += ret;                             //?´æ–°?           
            ptr     += ret;                             //?´æ–°?    
        }  
        else                                                    //è¶??  
        {  
            //printf("timeout!\n");  
            break;  
        }  
		
		
		
		
		
		
		#if 0
		switch (retval)
		{
			case 0:
			   // printf("no data input within  1s.\n");
				break;
			case -1:
				perror("select");
				break;       
			default:
			
				if((nread=read(uart_id,ptr,1))>0)
				{
					
					
					readlen += nread;
					ptr     += nread;
					
					/*
					if(recenum<65535)
					{
						recenum++;
					}
					else
					{
						recenum_times++;
						recenum=1;
					}*/
				}
				//printf("nread:%d\n",nread);
				//printf("msg[0]:%c\n",msg[0]);
			
			break;
		}
		#endif
	}
	//
	//if(retval > 0){
	//	printf("realen:%d\n",readlen);
	//printf("msg:%c\n",msg[0]);
	//}
	return readlen;
}

void write_port(int uart_id,char *msg,int len)
{
	write(uart_id, msg, len);
}
#if 1
void* com_read(void* pstatu)  
{
	int o;
	while(COM_READ_STATU)
	{
		pthread_mutex_lock(&mut);
    		read_port(uart.fd_5,NULL,0);
    		pthread_mutex_unlock(&mut);
	}
    pthread_exit(NULL);
}

void* com_send(void* p)  
{
	int ii;
	while(COM_SEND_STATU)
	{		
		write(fd, test, strlen(test));
		
		if(sendnum<65535)
			{
				sendnum++;
			}
			else
			{
				sendnum_times++;
				sendnum=1;
			}
		msleep(500);
			
	}
	pthread_exit(NULL);	
}
  
int start_thread_func(void*(*func)(void*), pthread_t* pthread, void* par, int* COM_STATU)  
{  
        *COM_STATU = 1;  
        memset(pthread, 0, sizeof(pthread_t));  
        int temp;  
            /*creat thread*/  
	if((temp = pthread_create(pthread, NULL, func, par)) != 0)  
	{
		printf("creat thread failer!\n");
	}  
	else  
        {  
            int id = pthread_self();  
            printf("%s,creat thread %lu sucess\n",dev, *pthread);  
        }  
        return temp;  
} 

void SignHandler(int iSignNo)
{
	int tmp_t=0;
	
	COM_SEND_STATU = 0; 
	msleep(1000);
	COM_READ_STATU = 0;  
        
	msleep(1000);

	//printf("%s,stop send,sendnum=%d,receivenum=%d\n",dev,sendnum*32,recenum);
	while(tmp_t<50000)
  	{
    		read_port(uart.fd_5,NULL,0);
		tmp_t++;
         }
	printf("\n%s,Send: %d ,Receive: %d \n",dev,(sendnum_times*65535+sendnum)*32,(recenum_times*65535+recenum));
	exit(1);
	
}
#endif
void dev_init(void)
{

	//char *dev ="/dev/ttymxc1"; 
	//signal(SIGINT,SignHandler);
	dev   = "/dev/ttymxc4";
    if(dev==NULL)
    {
		printf("Please input serial device name ,for exmaple /dev/ttymxc2.\n");
	 	exit(1);
    }

	//fd = OpenDev(dev);
	//uart.fd_1 = OpenDev(1);
	//uart.fd_2 = OpenDev(2);
	//uart.fd_3 = OpenDev(3);
	//uart.fd_4 = OpenDev(4);
	//uart.fd_5 = OpenDev(5);
    uart.fd_7 = OpenDev(7);
	uart.fd_8 = OpenDev(8);
	//printf("fd5:%d\n",uar t.fd_5);
	//printf("\nWelcome to TTYtest! Press Ctrl + 'c' to stop.\n\n");
    pthread_mutex_init(&mut,NULL); 
 /*
	 if(start_thread_func(com_read, (pthread_t*)&thread[READ_THREAD_ID],  (int *)&COM_READ_STATU, (int *)&COM_READ_STATU) != 0)  
        {  
            printf("error to leave\n");  
            return -1;  
        } 
		
		while(1){
			memset(msg,0,14);
int len = get_message_from_uart(uart.fd_5,msg,14);
//send_message_to_uart(uart.fd_5,"hello",5);
if(len > 0){
printf("len:%d\n",len);
printf("msg:%s\n",msg);
}
usleep(100);
}
*/
  /*
	 if(start_thread_func(com_send, (pthread_t*)&thread[SEND_THREAD_ID], (int *)&COM_SEND_STATU, (int *)&COM_SEND_STATU) != 0)
        {  
            printf("error to leave\n");  
            return -1;  
        }  
*/


}


int send_message_to_uart(int uart_id,char *buf,int len)
{
	write_port(uart_id,buf,len);
}
int get_message_from_uart(int uart_id,char *buf,int len)
{
	return read_port(uart_id,buf,len);
}



