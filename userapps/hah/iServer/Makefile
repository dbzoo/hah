TARGET=iServer
OBJECTS=tokenize.o main.o queue.o mem.o
XAPLIB=../xaplib2
CFLAGS=-I$(XAPLIB)
LEX=flex
INI=$(INSTALL_DIR)/etc/xap.d/iserver.ini

$(TARGET): $(OBJECTS)
	$(CC) -o $(TARGET) $(OBJECTS) -L$(XAPLIB) -lxap2 -lpthread

install: $(TARGET)
	install -s -m 755 $(TARGET) $(INSTALL_DIR)/usr/bin
	test -f $(INI) || install -m 644 iserver.ini $(INI)

static: $(OBJECTS)
	$(CC) -static -o $(TARGET) $(OBJECTS) -L$(XAPLIB) -lxap2 -lpthread

tokenize.c: tokenize.l

loadTest: loadTest.o
	$(CC) -o loadTest loadTest.o -L$(XAPLIB) -lxap2

loadClient: loadClient.o
	$(CC) -o loadClient loadClient.o -L$(XAPLIB) -lxap2

clean:
	$(RM) -f *.o $(TARGET) loadClient loadTest tokenize.c *~
