THREAD_LIB = $(LIB_DIR)/libthread.so

SO_LIBS += $(THREAD_LIB) 

THREADLIBOBJS = utils/thread/thread_pool.c

$(THREAD_LIB): $(THREADLIBOBJS) 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(THREADLIBOBJS) 

	