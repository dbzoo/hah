# File: drivers/char/bcmboard/bcm963xx/Makefile
#
# Makefile for the flash driver
#

O_TARGET := brcm_board.o

export-objs	:= board.o

obj-y := board.o cfiflash.o bcm63xx_flash.o bcm63xx_led.o

obj-m := $(O_TARGET)


EXTRA_CFLAGS += -I. -I$(HPATH)/asm/bcm963xx -I$(TOPDIR)/targets -fno-exceptions
# -fno-rtti

include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s

