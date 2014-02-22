# $Id$

all:
	@echo "Nothing to build"

install:
	install -m 755 xap-plugboard $(INSTALL_DIR)/usr/bin
	install -m 755 bscmsg $(INSTALL_DIR)/usr/bin
	install -m 755 aliasmsg $(INSTALL_DIR)/usr/bin
	install -D -m 644 plugboard.lua $(INSTALL_DIR)/etc/plugboard/plugboard.lua
	install -d $(INSTALL_DIR)/etc/plugboard/samples/
	install -m 644 samples/* $(INSTALL_DIR)/etc/plugboard/samples
	install -d $(INSTALL_DIR)/usr/share/lua/5.1/xap
	for i in `ls xap`; do install -m 644 xap/$$i $(INSTALL_DIR)/usr/share/lua/5.1/xap; done
	install -D -m 644 plugboard.ini $(INSTALL_DIR)/etc/xap.d

clean:
	@echo "Nothing to clean"
