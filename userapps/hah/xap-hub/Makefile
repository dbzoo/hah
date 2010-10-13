XAP=hub
CFLAGS=-Os -Wall -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2
CFLAGS+=-I../xaplib2
LDFLAGS+=-L../xaplib2

xap-$(XAP): $(XAP).o
	$(CC) -o xap-$(XAP) $(XAP).o $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core *.o
