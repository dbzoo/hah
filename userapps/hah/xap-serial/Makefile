# $Id$

TARGET=xap-serial
CFLAGS=-I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

$(TARGET): serial.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	install -m 644 $(TARGET).ini $(INSTALL_DIR)/etc/xap.d/

clean:
	rm -f $(TARGET) *.o
