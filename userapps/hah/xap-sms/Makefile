CC=mips-linux-gcc
XAP=sms
CFLAGS=-DIDENT -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2
OBJECTS=$(XAP).o modem.o putsms.o getsms.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) *.o *~
