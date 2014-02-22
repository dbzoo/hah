TARGET=xap-currentcost
CFLAGS+=-I../xaplib2 -I/usr/include/libxml2 -g
LDFLAGS+=-L../xaplib2 -lxml2 -lxap2

$(TARGET): currentcost.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	install -m 644 $(TARGET).ini $(INSTALL_DIR)/etc/xap.d

clean:
	$(RM) -f $(TARGET) *.o
