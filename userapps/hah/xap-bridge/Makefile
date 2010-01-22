XAP=bridge
CFLAGS=-DIDENT -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap
#CFLAGS=-DIDENT -DLINUX -I../xaplib -g
#LDFLAGS=-L../xaplib -lxap
OBJECTS=$(XAP).o config.o cfgParser.o cfgParse.o vars.o debug.o serial.o crc16.o dump.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) *~
