XAP=currentcost
CFLAGS+=-I../xaplib2 -I/usr/include/libxml2 -g
LDFLAGS+=-L../xaplib2 -lxml2 -lxap2
OBJECTS=$(XAP).o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS)
