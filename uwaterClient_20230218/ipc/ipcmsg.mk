IPCMSG_LIB = $(LIB_DIR)/libipcmsg.so

SO_LIBS += $(IPCMSG_LIB) 

IPCMSGLIBOBJS = ipc/system_v_shmem.c ipc/posix_shmem.c  ipc/shmem_obj.c \
		 ipc/shmem_msg.c  ipc/ipcmsg.c ipc/ipcmsg_query_req.c  ipc/shmmsg_client.c
		 
$(IPCMSG_LIB): $(IPCMSGLIBOBJS) 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(IPCMSGLIBOBJS) 

	