OBJECTS=tokenize.o main.o queue.o mem.o
XAPLIB=../xaplib2
CFLAGS=-I$(XAPLIB) -g
LEX=flex

all: $(OBJECTS)
	$(CC) -o iServer $(OBJECTS) -L$(XAPLIB) -lxap2 -lpthread

static: $(OBJECTS)
	$(CC) -static -o iServer $(OBJECTS) -L$(XAPLIB) -lxap2 -lpthread

tokenize.c: tokenize.l

loadTest: loadTest.o
	$(CC) -o loadTest loadTest.o -L$(XAPLIB) -lxap2

loadClient: loadClient.o
	$(CC) -o loadClient loadClient.o -L$(XAPLIB) -lxap2

clean:
	$(RM) -f *.o iServer loadClient loadTest tokenize.c *~
