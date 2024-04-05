#include "stdafx.h"   
#include "infolog.h"   
   
#define MAXLINELEN1 4096   
   
void err_prn(const char *fmt,va_list ap, char *logfile)   
{   
    char buf[MAXLINELEN1];   
    FILE *plf;   
       
    vsprintf(buf,fmt,ap);   
    strcat(buf,"\n");   
    fflush(stdout);   
    if(logfile!=NULL)   
        if((plf=fopen(logfile,"a"))!=NULL)   
        {   
            fputs(buf,plf);   
            fclose(plf);   
        }else   
            fputs("failed to open log file\n",stderr);   
    else   
        fputs(buf,stderr);   
    fflush(NULL);   
    return;   
}   
       
void syserror(const char *fmt,...)   
{   
    char prechar[100];   
    char *p;   
    time_t t;   
   
    time(&t);   
    p = ctime(&t);   
    p[strlen(p)-1]=0;   
    sprintf(prechar,"[%s] Error:%s",p,fmt);   
    fmt=prechar;   
   
    va_list ap;   
    va_start(ap,fmt);   
    err_prn(fmt,ap,"errlog.log");   
    va_end(ap);   
    return;   
}   
   
void sysinfo(const char *fmt,...)   
{   
    char prechar[100];   
    char *p;   
    time_t t;   
   
    time(&t);   
       
    p = ctime(&t);   
    p[strlen(p)-1]=0;   
    sprintf(prechar,"[%s] Info:%s",p,fmt);   
    fmt=prechar;   
    va_list ap;   
    va_start(ap,fmt);   
    err_prn(fmt,ap,"infolog.log");   
    va_end(ap);   
    return;   
}   
   
char * ccstr(char * str)   
{   
    char *tok = " ",*p;     
    char bufline[1000];   
    int len;   
       
    strcpy(bufline,str);   
    p = strtok(bufline,tok);   
   
    if ( p == NULL)   
    {   
        str[0] = 0;   
        return(str);   
    }   
   
    if ((len = strlen(bufline) - strlen(p)) > 0)   
    {   
        memcpy(str,str+len,strlen(str));   
        p=NULL;   
    }   
       
    p = strtok( str, tok );     
    while( p != NULL )     
    {     
        if( p != str )     
            strcat( str, p );     
        p = strtok( NULL, tok );     
    }     
    return( str );     
   
}   
   
char * ccstrex(char * str,char * tok)   
{   
    char *p;     
    char bufline[1000];   
    int len;   
       
    strcpy(bufline,str);   
    p = strtok(bufline,tok);   
   
    if ( p == NULL)   
    {   
        str[0] = 0;   
        return(str);   
    }   
   
    if ((len = strlen(bufline) - strlen(p)) > 0)   
    {   
        memcpy(str,str+len,strlen(str));   
        p=NULL;   
    }   
       
    p = strtok( str, tok );     
    while( p != NULL )     
    {     
        if( p != str )     
            strcat( str, p );     
        p = strtok( NULL, tok );     
    }     
    return( str );     
   
}  