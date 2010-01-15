XAP=snoop
CFLAGS=-Os -Wall -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap

xap-$(XAP): $(XAP).o
	$(CC) -o xap-$(XAP) $(XAP).o $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core *.o
