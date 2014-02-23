# $Id$

TARGET=xap-serial
CFLAGS=-I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): serial.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	rm -f $(TARGET) *.o
