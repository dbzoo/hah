XAP=sms
CFLAGS=-DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=$(XAP).o modem.o putsms.o getsms.o

xap-$(XAP): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) *.o *~
