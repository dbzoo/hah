# $Id$
OBJECTS=init.o parse.o timeout.o tx.o rx.o filter.o bsc.o log.o strlcpy.o strlcat.o minIni.o
CFLAGS=-Wall -fPIC

libxap2.so: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libxap2.so -o libxap2.so $(OBJECTS)

libxap2.a: $(OBJECTS)
	$(AR) rcv libxap2.a $(OBJECTS)

demo: demo.o libxap2.so
	$(CC) -o demo demo.o -L. -lxap2

clean:
	rm -f $(OBJECTS) libxap2.a libxap2.so demo.o

init.o: init.c xap.h
parse.o: parse.c xap.h
timeout.o: timeout.c xap.h
tx.o: tx.c xap.h
rx.o: rx.c xap.h
filter.o: filter.c xap.h
bsc.o: bsc.c xap.h bsc.h
log.o: log.c log.h
minIni.o: minIni.c minGlue.h

demo.o: demo.c xap.h bsc.h
