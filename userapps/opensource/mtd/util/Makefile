
# -*- sh -*-

# $Id: Makefile,v 1.1.1.1 2003/02/28 09:18:55 cmo Exp $

# xde CFLAGS += -I../include -O2 -Wall
CFLAGS += -I../include -O2 -Wall

TARGETS = ftl_format erase eraseall nftldump nanddump doc_loadbios \
nftl_format mkfs.jffs ftl_check nandtest nandwrite mkfs.jffs2 lock unlock \
einfo mtd_debug fcp jffs2reader

SYMLINKS = crc32.h crc32.c compr_rtime.c compr_rubin.c compr.c pushpull.c pushpull.h histo_mips.h compr_rubin.h

all: $(TARGETS)

clean:
	rm -f *.o $(TARGETS) $(SYMLINKS)

mkfs.jffs2.o crc32.o: crc32.h
pushpull.o compr_rubin.o: pushpull.h
compr_rubin.o: histo_mips.h compr_rubin.h

crc32.h crc32.c compr_rtime.c compr_rubin.c compr.c pushpull.c pushpull.h histo_mips.h compr_rubin.h compr_zlib.c:
	ln -sf ../fs/jffs2/$@ $@

mkfs.jffs2: crc32.o compr_rtime.o compr_rubin.o compr.o pushpull.o mkfs.jffs2.o compr_zlib.o
	$(CC) -o $@ $^ -lz

compr.o: compr.c
	$(CC) $(CFLAGS) -Dprintk=printf -DKERN_NOTICE= -c -o $@ $<

jffs2reader: jffs2reader.c
	$(CC) $(CFLAGS) jffs2reader.c -o jffs2reader -lz 


