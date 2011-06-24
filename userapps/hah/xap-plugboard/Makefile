all:
	install -m 755 xap-plugboard $(INSTALL_DIR)/usr/bin
	install -m 644 plugboard.lua $(INSTALL_DIR)/etc_ro_fs/plugboard
	install -m 644 samples/* $(INSTALL_DIR)/etc_ro_fs/plugboard/samples
	install -d 755 $(INSTALL_DIR)/usr/share/lua/5.1/xap
	for i in `ls xap`; do install -m 644 xap/$$i $(INSTALL_DIR)/usr/share/lua/5.1/xap; done

clean:
	echo "Nothing to clean"
