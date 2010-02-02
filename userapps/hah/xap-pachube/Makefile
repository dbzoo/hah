XAP=xap-pachube
CFLAGS=-DIDENT -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lcurl
OBJECTS=main.o pachube.o mem.o debug.o dscache.o

$(XAP): $(OBJECTS)
	$(CC) -o $(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f *.o xap-$(XAP)
