XAP=hub
CFLAGS=-Os -Wall -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

xap-$(XAP): $(XAP).o
	$(CC) -o xap-$(XAP) $(XAP).o $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core *.o
