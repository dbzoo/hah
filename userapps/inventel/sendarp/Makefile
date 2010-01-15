all: sendarp

sendarp: sendarp.c
	$(CC) -o sendarp sendarp.c -I $(INC_KERNEL_PATH)

install: all
	install -m 755 sendarp $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/sendarp

clean:
	rm -f sendarp
	rm -f $(INSTALL_DIR)/bin/sendarp
