XAP=bridge
CFLAGS=-DIDENT -DLINUX -I../xaplib
OBJECTS=$(XAP).o config.o cfgParser.o cfgParse.o vars.o debug.o serial.o crc16.o dump.o mem.o queue.o
LIBOBJS=../xaplib/minIni.o ../xaplib/strlcat.o  ../xaplib/strlcpy.o  ../xaplib/xapinit.o  ../xaplib/xaprx.o  ../xaplib/xaptx.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) -L$(INSTALL_DIR)/lib -lxap -lpthread

# Build a NATIVE static binary.
standalone: $(OBJECTS) $(LIBOBJS)
	gcc -o xap-$(XAP) $(OBJECTS) $(LIBOBJS) -lpthread

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) *~
