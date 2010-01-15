# $Id: Makefile 43 2009-11-21 16:42:02Z brett $

XAP=livebox
CFLAGS=-g -DLINUX -DIDENT -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap
OBJECTS=$(XAP).o reply.o server.o endpoint.o ini.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

poke:	poke.c
	$(CC) -g -o poke poke.c

clean:
	rm -f xap-$(XAP) core $(OBJECTS)
