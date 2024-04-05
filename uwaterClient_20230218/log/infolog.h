#ifdef _LIBERR_H 
#define _LIBERR_H 
 
#include <stdarg.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h> 
#include <time.h> 
 
 
 
void err_ret(const char *fmt,...); 
void err_prn(const char *fmt,va_list ap, char *logfile); 
void syserror(const char *fmt,...); 
void sysinfo(const char *fmt,...); 
char * ccstr(char * str); 
char * ccstrex(char * str,char * tok); 
 
#endif _LIBERR_H 