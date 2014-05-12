# $Id$

TARGET=xap-mail
CFLAGS=-Os -Wall -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2 -lcurl -lm
INI=$(INSTALL_DIR)/etc/xap.d/$(TARGET).ini

$(TARGET): mail.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

install: $(TARGET)
	install -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	$(STRIP) $(INSTALL_DIR)/usr/bin/$(TARGET)

clean:
	rm -f $(TARGET) core *.o
