# $Id$

XAP=livebox
CFLAGS=-DIDENT -Wall -DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=$(XAP).o serial.o ini.o server.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core $(OBJECTS)
