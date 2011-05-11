all:
	install -m 755 xap-plugboard $(INSTALL_DIR)/usr/bin
	install -m 644 plugboard.lua $(INSTALL_DIR)/etc_ro_fs/plugboard
	install -m 644 samples/* $(INSTALL_DIR)/etc_ro_fs/plugboard/samples
	install -m 644 xap.lua $(INSTALL_DIR)/usr/share/lua/5.1

clean:
	echo "Nothing to clean"
