# $Id$

TARGET=xap-xively
CFLAGS=-DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2
OBJECTS=main.o xively.o mem.o dscache.o pachulib.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin

clean:
	$(RM) -f *.o $(TARGET)
