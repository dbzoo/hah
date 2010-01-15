XAP=currentcost
CFLAGS=-DIDENT -DLINUX -I$(INSTALL_DIR)/include -I$(INSTALL_DIR)/include/libxml2
LDFLAGS=-L$(INSTALL_DIR)/lib -lxml2 -lxap
OBJECTS=$(XAP).o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS)
