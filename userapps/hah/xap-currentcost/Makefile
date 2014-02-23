TARGET=xap-currentcost
CFLAGS+=-I../xaplib2 -I/usr/include/libxml2 -g
LDFLAGS+=-L../xaplib2 -lxml2 -lxap2
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): currentcost.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 $(TARGET).ini $(INI)

clean:
	$(RM) -f $(TARGET) *.o
