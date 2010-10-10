# $Id$

XAP=googlecal
CFLAGS=-I$(INSTALL_DIR)/include -I$(INSTALL_DIR)/include/libxml2
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2 -lgcal -lxml2 -lcurl
#LDFLAGS+=-L../xaplib2
#CFLAGS+=-I../xaplib2
OBJECTS=$(XAP).o svn_date.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) core
