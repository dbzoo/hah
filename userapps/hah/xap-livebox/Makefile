# $Id$

TARGET=xap-livebox
CFLAGS=-DIDENT -Wall -DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=livebox.o serial.o ini.o server.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	install -m 644 $(TARGET).ini $(INSTALL_DIR)/etc/xap.d

clean:
	rm -f $(TARGET) core $(OBJECTS)
