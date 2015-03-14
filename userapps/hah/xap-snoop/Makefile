TARGET=xap-snoop
CFLAGS=-Os -Wall -DLINUX -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

$(TARGET): snoop.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin

clean:
	rm -f $(TARGET) core *.o
