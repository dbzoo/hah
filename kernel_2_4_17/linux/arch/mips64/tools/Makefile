#
# Makefile for MIPS kernel build tools.
#
# Copyright (C) 1996 David S. Miller (dm@engr.sgi.com)
# Copyright (C) 1997, 1999 Ralf Baechle (ralf@gnu.org)
# Copyright (C) 1999 Silicon Graphics, Inc.
#
TARGET	:= $(TOPDIR)/include/asm-$(ARCH)/offset.h

.S.s:
	$(CPP) $(CFLAGS) $< -o $*.s
.S.o:
	$(CC) $(CFLAGS) -c $< -o $*.o

$(TARGET): offset.h
	cmp -s $^ $@ || (cp $^ $(TARGET).new && mv $(TARGET).new $(TARGET))

offset.h: offset.s
	sed -n '/^@@@/s///p' $^ >$@

offset.s: offset.c $(TOPDIR)/include/linux/autoconf.h

clean:
	rm -f offset.[hs] $(TARGET).new
	
mrproper: clean
	rm -f $(TARGET)

include $(TOPDIR)/Rules.make
