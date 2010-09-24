CFLAGS=-Wall -I..
LDFLAGS=-L.. -lxap2
PROGS=timeout parse filter demo

all: $(PROGS)

timeout: timeout.o
parse: parse.o
filter: filter.o
demo: demo.o

clean:
	rm -f $(PROGS) *.o
