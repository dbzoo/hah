OBJECTS=tokenize.o main.o
XAPLIB=../xaplib2
CFLAGS=-I$(XAPLIB)

all: $(OBJECTS)
	$(CC) -o iServer $(OBJECTS) -L$(XAPLIB) -lxap2 -lpthread

tokenize.c: tokenize.l

clean:
	$(RM) -f *.o iServer tokenize.c *~
