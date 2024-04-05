#ifndef ___IPC_DEF_H___
#define ___IPC_DEF_H___



#define IPCMSG_ENABLE_CRC	0


#define IPCMSG_DEBUG	0

#define IPC_INFO_PRINT		0
#define IPC_DEBUG_PRINT		0


#define	SHM_WRITE_TIME_OUT_MS	5000

#undef	IPC_PRINTF
#define IPC_PRINTF(fmt, ...)	printf(fmt, ##__VA_ARGS__)


#if	IPC_INFO_PRINT | IPC_DEBUG_PRINT
#define Info_Printf(fmt, ...)	IPC_PRINTF("%s : "fmt, __FUNCTION__, ##__VA_ARGS__)
#else
#define Info_Printf(fmt, ...)
#endif

#if	IPC_DEBUG_PRINT
#define Debug_Printf(fmt, ...)	IPC_PRINTF("---%s  [line %d]--- : "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define Debug_Printf(fmt, ...)
#endif


#endif

