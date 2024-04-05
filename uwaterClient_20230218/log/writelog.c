/*日志记录*/
//#include "writelog.h"   
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include <fcntl.h>
#include <stdarg.h>

int file_fd = -1;
void  log_file(void)
{
    char file_path[512] = {0};
    char build_path[512] = {0};
    char filetime[32] = {0};
    struct tm tm_time;
    time_t t_log;
 
//    assert(getcwd(file_path, 512) != NULL);    //当前目录
//    if (file_path[strlen(file_path) - 1] != '/') {
//        file_path[strlen(file_path)] = '/';
//    }

    memcpy(file_path,"/root/runLog/",13);
    if(access(file_path, F_OK) != 0) {     //目录不存在	
		memset(build_path,0,512);
		sprintf(build_path,"mkdir -p %s",file_path);
 		assert(system(build_path) !=0 );
    }
 
    t_log = time(NULL);
    localtime_r(&t_log, &tm_time);
    strftime(filetime, sizeof(filetime), "%Y%m%d%H%M%S", &tm_time); //日志的时间
      
    strcat(file_path, "log_");
    strcat(file_path, filetime);
    strcat(file_path, ".log");
 
    file_fd = open(file_path, O_RDWR|O_CREAT|O_APPEND, 0666);
    assert(file_fd != -1);
}
void write_cmd(const char *fmt,...)
{
    va_list ap;
    va_start(ap,fmt);
    vprintf(fmt,ap);
    va_end(ap);
}
void write_log(const char *msg, ...)
{
    char final[2048] = {0};
    va_list vl_list;
    va_start(vl_list, msg);
    char content[1024] = {0};
    vsprintf(content, msg, vl_list);
    va_end(vl_list);

    time_t  time_write;
    struct tm tm_Log;
    time_write = time(NULL);
    localtime_r(&time_write, &tm_Log);
    strftime(final, sizeof(final), "[%Y-%m-%d %H:%M:%S] ", &tm_Log);

    strncat(final, content, strlen(content));
    assert(msg != NULL && file_fd != -1);
    assert( write(file_fd, final, strlen(final)) == strlen(final));
}
void close_file(void)
{
    close(file_fd);
}
    