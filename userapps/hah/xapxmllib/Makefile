# $Id$
OBJECTS=alias.o
LDFLAGS=-L$(INSTALL_DIR)/lib -lxml2
CFLAGS=-DLINUX -fPIC -I$(INSTALL_DIR)/include/libxml2 -I$(INSTALL_DIR)/include

xaplib.so: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libxapxml.so -o libxapxml.so $(OBJECTS) $(LDFLAGS)

clean:
	rm -f $(OBJECTS) libxapxml.so
