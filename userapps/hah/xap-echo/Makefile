TARGET=xap-echo
CFLAGS=-Os -Wall -DLINUX -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

$(TARGET): echo.o
	$(CC) -o $(TARGET) $< $(LDFLAGS)

clean:
	rm -f $(TARGET) core *.o
