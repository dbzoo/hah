# $Id$

TARGET=xap-googlecal
LDFLAGS=-L../xaplib2 -lxap2 -lgcal -lxml2 -lcurl
CFLAGS=-DDEV -I../../opensource/libgcal -I../xaplib2
OBJECTS=googlecal.o svn_date.o

$(TARGET): $(OBJECTS)
	$(CC) -o xap-$(XAP) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin

clean:
	rm -f $(TARGET) $(OBJECTS) core
