CFLAGS=-I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -llua -lluarex -lm
OBJECTS=lua.o

lua: $(OBJECTS)
	$(CC) -o $@ $? $(LDFLAGS)

clean:
	rm -f $(OBJECTS) *~
