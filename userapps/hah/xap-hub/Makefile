TARGET=xap-hub
CFLAGS=-Os -Wall -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

$(TARGET): hub.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin

clean:
	rm -f $(TARGET) *.o
