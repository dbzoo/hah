CFLAGS=-DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap -llua -lluarex -lm
OBJECTS=xap-plugboard.o msg_cache.o

xap-plugboard: $(OBJECTS)
	$(CC) -o xap-plugboard $(CFLAGS) $(OBJECTS) $(LDFLAGS)
	rm -rf $(INSTALL_DIR)/etc_ro_fs/plugboard/samples
	cp -r samples $(INSTALL_DIR)/etc_ro_fs/plugboard

lposix.o: lposix.c
	$(CC) -c -DREX_LIBNAME=\"rex\" $(CFLAGS) lposix.c 

clean:
	rm -f $(OBJECTS) xap-plugboard *~
