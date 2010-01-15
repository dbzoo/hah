dwbtool:
	CC=gcc make -C $(OPENSOURCE_DIR)/dwbtool
	install -m 755 $(OPENSOURCE_DIR)/dwbtool/dwbtool .

cramfs:
	CC=gcc make -C $(OPENSOURCE_DIR)/cramfs-1.1
	install -m 755 $(OPENSOURCE_DIR)/cramfs-1.1/mkcramfs .

clean:
	make -C $(OPENSOURCE_DIR)/dwbtool clean
	make -C $(OPENSOURCE_DIR)/cramfs-1.1 clean
	rm -f dwbtool mkcramfs
