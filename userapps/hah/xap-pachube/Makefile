XAP=xap-pachube
CFLAGS=-DIDENT -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib
#CFLAGS=-I../xaplib2
#LDFLAGS=-L../xaplib2
LDFLAGS+=-lxap2
OBJECTS=main.o pachube.o mem.o dscache.o pachulib.o

$(XAP): $(OBJECTS)
	$(CC) -o $(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f *.o xap-$(XAP)
