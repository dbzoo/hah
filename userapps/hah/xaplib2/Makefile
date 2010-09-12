# $Id$
OBJECTS=init.o parse.o timeout.o tx.o rx.o filter.o bsc.o strlcpy.o strlcat.o
CFLAGS=-g -Wall

all: demo

init.c: xap.h
parse.c: xap.h
timeout.c: xap.h
bsc.c: xap.h bsc.h
tx.c: xap.h
rx.c: xap.h
filter.c: xap.h

demo: demo.o $(OBJECTS)
	$(CC) -o demo demo.o $(OBJECTS) 

xaplib2.so: $(OBJECTS)
	$(CC) -shared -Wl,-soname,libxap2.so -o libxap2.so $(OBJECTS)

xaplib2.a: $(OBJECTS)
	$(AR) rcv libxap2.a $(OBJECTS)

clean:
	rm -f $(OBJECTS) libxap2.a libxap2.so demo.o
