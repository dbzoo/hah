# File: drivers/char/ser_cons/bcm963xx/Makefile
#
# Makefile for the BCM63xx serial/console driver
#

O_TARGET := sc.o

obj-y := bcm63xx_cons.o 

obj-m := $(O_TARGET)

EXTRA_CFLAGS += -I. -I$(HPATH)/asm/bcm963xx

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s

