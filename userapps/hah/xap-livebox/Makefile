# $Id$

XAP=livebox
CFLAGS=-DIDENT -I$(INSTALL_DIR)/include
#LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2
LDFLAGS=../xaplib2/libxap2.a

#DEV
#CFLAGS=-g -Wall -DIDENT -I../xaplib2
#LDFLAGS=-L../xaplib2 -lxap2

OBJECTS=$(XAP).o serial.o ini.o server.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core $(OBJECTS)
