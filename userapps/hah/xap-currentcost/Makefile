XAP=currentcost
CFLAGS=-DIDENT -I$(INSTALL_DIR)/include -I$(INSTALL_DIR)/include/libxml2
LDFLAGS=-L$(INSTALL_DIR)/lib -lxml2 -lxap2
# Development flags
CFLAGS+=-I../xaplib2 -I/usr/include/libxml2 -g
LDFLAGS+=-L../xaplib2 -lxml2 -lxap2
OBJECTS=$(XAP).o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS)
