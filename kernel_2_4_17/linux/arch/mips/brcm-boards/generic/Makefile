#
# Makefile for generic Broadcom MIPS boards
#
# Copyright (C) 2001 Broadcom Corporation
#
# $Id: Makefile,v 1.3 2004/02/26 15:14:47 xde Exp $
#

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

O_TARGET	:= brcm.o

export-objs	:= irq.o setup.o prom.o
obj-y		:= setup.o int-handler.o irq.o time.o prom.o ser_init.o

ifdef CONFIG_PCI
obj-y += pcibios.o
endif

ifdef CONFIG_REMOTE_DEBUG
obj-y += dbg_io.o
endif

include $(TOPDIR)/Rules.make
SRCBASE		:= $(TOPDIR)
EXTRA_CFLAGS	+= -I$(TOPDIR)/targets -I$(SRCBASE)/include -I$(HPATH)/asm/bcm963xx -DDBG

ifeq "$(ADSL)" "ANNEX_B"
EXTRA_CFLAGS += -DADSL_ANNEXB
endif
ifeq "$(ADSL)" "SADSL"
EXTRA_CFLAGS += -DADSL_SADSL
endif
ifeq "$(ADSL)" "ANNEX_C"
EXTRA_CFLAGS += -DADSL_ANNEXC
endif

vpath %.c $(SRCBASE)/shared

