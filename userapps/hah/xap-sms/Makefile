# $Id$
TARGET=xap-sms
CFLAGS=-DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=sms.o modem.o putsms.o getsms.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	install -m 644 $(TARGET).ini $(INSTALL_DIR)/etc/xap.d

clean:
	$(RM) -f $(TARGET) *.o *~
