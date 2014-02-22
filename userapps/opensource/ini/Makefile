# $Id$

OBJECTS=ini.o minIni.o

ini: $(OBJECTS)
	$(CC) -o ini $(OBJECTS)

clean:
	rm -f ini core $(OBJECTS)
	rm -f $(INSTALL_DIR)/usr/bin/ini
	rm -f $(INSTALL_DIR)/usr/bin/iniget
	rm -f $(INSTALL_DIR)/usr/bin/iniset

install: ini
	install -s -m 755 ini $(INSTALL_DIR)/usr/bin
	test -h $(INSTALL_DIR)/usr/bin/iniget || ln -s /usr/bin/ini $(INSTALL_DIR)/usr/bin/iniget
	test -h $(INSTALL_DIR)/usr/bin/iniset || ln -s /usr/bin/ini $(INSTALL_DIR)/usr/bin/iniset
