# $Id$
OBJECTS=xapinit.o xaprx.o xaptx.o minIni.o strlcpy.o strlcat.o
CFLAGS=-DLINUX -fPIC

xaplib.so: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libxap.so -o libxap.so $(OBJECTS)

xaplib.a: $(OBJECTS)
	$(AR) rcv libxap.a $(OBJECTS)

clean:
	rm -f $(OBJECTS) libxap.a libxap.so
