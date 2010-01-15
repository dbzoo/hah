# $Id$

XAP=twitter
CFLAGS=-g -DLINUX -DIDENT -I$(INSTALL_DIR)/include -I$(INSTALL_DIR)/include/libxml2
#CFLAGS+=-I../xaplib -I../xapxmllib
#LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lxapxml -lxml2 -lcurl
#LDFLAGS+=-L../xaplib -L../xapxmllib
CFLAGS+=-I../xaplib
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lxml2 -lcurl
LDFLAGS+=-L../xaplib
OBJECTS=$(XAP).o tcurl.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) core
