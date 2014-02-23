# $Id$

TARGET=xap-twitter

CFLAGS=-DLINUX -DIDENT
CFLAGS+=-I../xaplib2 -I../../opensource/jsmn
LDFLAGS=-lxap2 -lcrypto -lcurl -ljsmn -lm
LDFLAGS+=-L../xaplib2 -L../../opensource/jsmn
OBJECTS=twitter.o tcurl.o oauth.o xmalloc.o hash.o oauth_http.o
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	$(RM) -f $(TARGET) *.o core
