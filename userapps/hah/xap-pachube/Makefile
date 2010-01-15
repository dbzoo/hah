XAP=pachube
CFLAGS=-DIDENT -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lcurl
OBJECTS=$(XAP).o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f *.o xap-$(XAP)
