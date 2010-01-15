CFLAGS=-I$(INSTALL_DIR)/include
OBJECTS=lposix.o common.o

libluarex.a: $(OBJECTS)
	$(AR) rcv $@ $?

lposix.o: lposix.c
	$(CC) -c -DREX_LIBNAME=\"rex\" $(CFLAGS) lposix.c 

clean:
	rm -f $(OBJECTS) *~
