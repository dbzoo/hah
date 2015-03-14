TARGET=xap-mail
CFLAGS=-Os -Wall -DLINUX -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2 -lcrypto -lcurl -lm
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): mail.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	rm -f $(TARGET) core *.o
