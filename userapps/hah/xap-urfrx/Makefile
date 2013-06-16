CFLAGS=-g -Wall -DIDENT -I../xaplib2
LDFLAGS=-L../xaplib2 -lxap2

OBJECTS = rfrx.o urfdecoder.o
GENOBJ = urfdecoder.o genurf.o
MAIN=xap-urfrx

$(MAIN): $(OBJECTS)
	$(CC) -o $(MAIN) $(OBJECTS) $(LDFLAGS)


genurf: $(GENOBJ)
	$(CC) -o genurf $(GENOBJ) $(LDFLAGS)

clean:
	$(RM) -f $(MAIN) genurf *.o core *~
