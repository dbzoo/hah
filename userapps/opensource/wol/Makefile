# $Id$

wol: wol.o
	$(CC) -o wol wol.o
	$(STRIP) wol

clean:
	rm -f wol core wol.o
	rm -f $(INSTALL_DIR)/usr/bin/wol

install: wol
	install -m 755 $(OPENSOURCE_DIR)/wol/wol $(INSTALL_DIR)/usr/bin
