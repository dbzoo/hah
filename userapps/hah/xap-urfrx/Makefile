# $Id$
TARGET=xap-urfrx
CFLAGS=-g -Wall -DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

OBJECTS = rfrx.o urfdecoder.o
GENOBJ = urfdecoder.o genurf.o

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin

genurf: $(GENOBJ)
	$(CC) -o $@ $(GENOBJ) $(LDFLAGS)

clean:
	$(RM) -f $(TARGET) genurf *.o core *~
