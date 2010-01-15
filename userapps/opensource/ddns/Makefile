#
# ddns client update utility
# Charles 08/19/2003
#
CFLAGS+=-Wall

OBJS = ddns.o

all: ddns

install::
	install -m 755 ddns $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/ddns

dynamic: all install

.c.o:
	$(CC) $(CFLAGS) -c $<

ddns: ddns.o
	$(CC) $(CFLAGS) -o ddns ddns.o

clean:
	-rm -f ddns *.o
