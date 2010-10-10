# $Id$

XAP=twitter

# uncomment for standalone compile
#INSTALL_DIR=/usr
CFLAGS=-g -DLINUX -DIDENT -I$(INSTALL_DIR)/include
CFLAGS+=-I../xaplib2 -I../../opensource/openssl/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2 -lcurl -lm
LDFLAGS+=-L../xaplib2
OBJECTS=$(XAP).o tcurl.o oauth.o xmalloc.o hash.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) core
