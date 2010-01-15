# $Id: GNUmakefile,v 1.1.1.1 2003/02/28 09:18:53 cmo Exp $

ifndef CONFIG_MTD

# We're being invoked outside a normal kernel build. Fake it

CONFIG_MTD_DOC1000 := m
CONFIG_MTD_DOC2000 := m
CONFIG_MTD_DOC2001 := m
CONFIG_MTD_DOCPROBE := m
CONFIG_MTD_SLRAM := m
CONFIG_MTD_PMC551 := m
CONFIG_MTD_MTDRAM := m

CFLAGS_mtdram.o := -DCONFIG_MTDRAM_TOTAL_SIZE=4096 -DCONFIG_MTDRAM_ERASE_SIZE=128

endif

# Normal case - build in-kernel

ifeq ($(VERSION),2)
 ifneq ($(PATCHLEVEL),4)
  ifneq ($(PATCHLEVEL),5)
   OLDTOPDIR := $(TOPDIR)
   TOPDIR := $(shell pwd)/..
  endif
 endif
endif

include Makefile


