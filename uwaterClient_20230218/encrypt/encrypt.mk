ENCRYPT_LIB = $(LIB_DIR)/libencrypt.so

SO_LIBS += $(ENCRYPT_LIB) 

ENCRYPTLIBOBJS = encrypt/encrypt.c

$(ENCRYPT_LIB): $(ENCRYPTLIBOBJS) 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(ENCRYPTLIBOBJS) 