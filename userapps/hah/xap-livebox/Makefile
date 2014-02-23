# $Id$

TARGET=xap-livebox
CFLAGS=-DIDENT -Wall -DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=livebox.o serial.o ini.o server.o
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	rm -f $(TARGET) core $(OBJECTS)
