CFLAGS=-Wall -I..
LDFLAGS=-L.. -lxap2
PROGS=timeout parse filter

all: $(PROGS)

timeout: timeout.o
parse: parse.o
filter: filter.o

clean:
	rm -f $(PROGS) *.o
