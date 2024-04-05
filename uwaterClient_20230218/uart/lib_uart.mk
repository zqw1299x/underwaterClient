UART_LIB = $(LIB_DIR)/libuart.so

SO_LIBS += $(UART_LIB) 

UARTLIBOBJS = uart/uart.c

$(UART_LIB): $(UARTLIBOBJS) 
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $(UARTLIBOBJS) 