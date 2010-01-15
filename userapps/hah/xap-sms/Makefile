CC=mips-linux-gcc
XAP=sms
CFLAGS=-DIDENT -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap
OBJECTS=$(XAP).o modem.o putsms.o getsms.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) *.o *~
