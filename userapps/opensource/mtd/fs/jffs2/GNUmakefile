# GNUmakefile for JFFS2 build outside kernel tree
# $Id: GNUmakefile,v 1.1.1.1 2003/02/28 09:18:54 cmo Exp $

LINUXDIR=/usr/src/linux

ifndef VERSION

# Someone just typed 'make'

modules:
	make -C $(LINUXDIR) SUBDIRS=`pwd` modules

dep:
	make -C $(LINUXDIR) SUBDIRS=`pwd` dep

clean:
	rm -f *.o

else

ifndef CONFIG_JFFS2_FS

CC += -I$(shell pwd)/../../include

CONFIG_JFFS2_FS := m
EXTRA_CFLAGS += -DCONFIG_JFFS2_FS_DEBUG=1 -g

endif

# Check whether we've put the JFFS2 stuff in the superblock and inode unions
OUT_OF_KERNEL_CFLAGS := $(shell grep -q jffs2 $(TOPDIR)/include/linux/fs.h || echo "-DJFFS2_OUT_OF_KERNEL")
EXTRA_CFLAGS += $(OUT_OF_KERNEL_CFLAGS)


include Makefile


endif
