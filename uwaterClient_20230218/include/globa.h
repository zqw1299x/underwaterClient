#define RNS_LEN  1024

#define GLOBAL_SHM_SIZE		0x10000
#define GLOBAL_SHM_KEY		999

#define S_Z_1KB		0x400
#define S_Z_1MB		0x100000

#define MAX_Packet	50*S_Z_1MB
extern pthread_mutex_t mut;
extern pthread_mutex_t udp;


typedef struct uart_S
{
	int fd_1;
	int fd_2;
	int fd_3;
	int fd_4;
	int fd_5;
    int fd_7;
	int fd_8;
	int reserve;

}uart_s;



typedef struct system_parameter_s{

	int watchdog;//watchdog  0:off 1:on
	int monitor;
	int mcu_ver;//software
	int HW_ver;//hardware
}system_parameter_t;

typedef struct encrypt_parameter_s{

	unsigned char key[100];
	unsigned char name[10];
	unsigned char username[20];
	unsigned char password[20];
}encrypt_parameter_t;


typedef struct connect_parameter_s{

	unsigned char elec_info;

}connect_parameter_t;

extern connect_parameter_t *connect_para;
extern system_parameter_t *system_para;
extern encrypt_parameter_t *myencrypt;
extern uart_s uart;
void open_global_shm(void);
char *get_encrypt_version(void);
void otp_encrypt_buf(unsigned char *file_from,int input_len,unsigned char *to_file,unsigned char *key,int key_len,int type);
void otp_encrypt(unsigned char *file_from,unsigned char *to_file,unsigned char *key,int key_len,int type);
void dev_init(void);
int send_message_to_uart(int uart_id,char *buf,int len);
int get_message_from_uart(int uart_id,char *buf,int len);

void close_file(void);
void log_file(void);
void write_log(const char *msg, ...);
