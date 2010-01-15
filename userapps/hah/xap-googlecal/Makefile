# $Id$

XAP=googlecal
CFLAGS=-g -DLINUX -DIDENT -I$(INSTALL_DIR)/include -I$(INSTALL_DIR)/include/libxml2
#CFLAGS+=-I../xaplib -I../xapxmllib
#LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lxapxml -lgcal -lxml2 -lcurl
#LDFLAGS+=-L../xaplib -L../xapxmllib
CFLAGS+=-I../xaplib
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -lgcal -lxml2 -lcurl
LDFLAGS+=-L../xaplib
OBJECTS=$(XAP).o svn_date.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) core
