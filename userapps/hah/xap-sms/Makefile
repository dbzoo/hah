TARGET=xap-sms
CFLAGS=-I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=sms.o modem.o putsms.o getsms.o
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	$(RM) -f $(TARGET) *.o *~
