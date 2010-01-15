all: ledctrl

ledctrl: ledctrl.c
	$(CC) -o ledctrl ledctrl.c -I $(INC_KERNEL_PATH)

install: all
	install -m 755 ledctrl $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/ledctrl

clean:
	rm -f ledctrl
	rm -f $(INSTALL_DIR)/bin/ledctrl
