# $Id: Makefile,v 1.1.1.1 2003/02/28 09:18:53 cmo Exp $
#

PROGS = fcp eraseall einfo
#TARGETS = jffs kernel
#TARGETS = ffs2 #kernel
SUBDIRS = util # boot
#LINUXDIR = /usr/src/linux
all:	
#	make -C $(LINUXDIR) SUBDIRS=$(shell pwd)/kernel modules
	for a in $(SUBDIRS); do \
		make -C $$a; \
	done

install: all
	for a in $(PROGS); do \
		install -m 755 util/$$a $(INSTALL_DIR)/usr/sbin; \
		$(STRIP) $(INSTALL_DIR)/usr/sbin/$$a; \
	done

dynamic: all install

jffs:
	make -C $(LINUXDIR) SUBDIRS=$(shell pwd)/fs/jffs modules

jffs2:
	make -C $(LINUXDIR) SUBDIRS=$(shell pwd)/fs/jffs2 modules

ffs2:
	make -C $(LINUXDIR) SUBDIRS=$(shell pwd)/fs/ffs2 modules

util:
	make -C util

boot:
	make -C boot

clean:
	find kernel/ -name '*.[oa]' -type f | xargs rm -f
	find fs/ -name '*.[oa]' -type f | xargs rm -f
	make -C util clean
	make -C boot clean
	for a in $(PROGS); do \
		rm -f $(INSTALL_DIR)/usr/sbin/$$a; \
	done
