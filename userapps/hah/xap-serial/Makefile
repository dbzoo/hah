# $Id$

XAP=serial
CFLAGS=-I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2
#DEV
CFLAGS+=-g -I../xaplib2
LDFLAGS+=-L../xaplib2 -lxap2

OBJECTS=$(XAP).o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core $(OBJECTS)
