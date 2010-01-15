#!/bin/sh

if [ $# -lt 1 ]
then
	echo "Usage: " $0 "kernel-path [patch-file]"
	exit 1
fi

cd `dirname $0`
THISDIR=`pwd`
TOPDIR=`dirname $THISDIR`

LINUXDIR=$1
if [ ! -f $LINUXDIR/Makefile ]
then
	echo "Directory" $LINUXDIR "doesn't exist, or doesn't contain Linux sources"
	exit 1
fi

cd $LINUXDIR

#
#	make symlinks
#

mkdir -p drivers/mtd
mkdir -p drivers/mtd/chips
mkdir -p drivers/mtd/devices
mkdir -p drivers/mtd/maps
mkdir -p drivers/mtd/nand
mkdir -p include/linux/mtd
mkdir -p fs/jffs
ln -sf $TOPDIR/drivers/mtd/*.[ch] drivers/mtd
ln -sf $TOPDIR/drivers/mtd/*akefile $TOPDIR/drivers/mtd/Rules.make $TOPDIR/drivers/mtd/Config.in drivers/mtd
ln -sf $TOPDIR/drivers/mtd/chips/*.[ch] drivers/mtd/chips
ln -sf $TOPDIR/drivers/mtd/chips/*akefile $TOPDIR/drivers/mtd/chips/Config.in drivers/mtd/chips
ln -sf $TOPDIR/drivers/mtd/devices/*.[ch] drivers/mtd/devices
ln -sf $TOPDIR/drivers/mtd/devices/*akefile $TOPDIR/drivers/mtd/devices/Config.in drivers/mtd/devices
ln -sf $TOPDIR/drivers/mtd/maps/*.[ch] drivers/mtd/maps
ln -sf $TOPDIR/drivers/mtd/maps/*akefile $TOPDIR/drivers/mtd/maps/Config.in drivers/mtd/maps
ln -sf $TOPDIR/drivers/mtd/nand/*.[ch] drivers/mtd/nand
ln -sf $TOPDIR/drivers/mtd/nand/*akefile $TOPDIR/drivers/mtd/nand/Config.in drivers/mtd/nand
ln -sf $TOPDIR/fs/jffs/*.[ch] fs/jffs
ln -sf $TOPDIR/fs/jffs/*akefile fs/jffs
ln -sf $TOPDIR/include/linux/jffs.h include/linux
ln -sf $TOPDIR/include/linux/mtd/*.h include/linux/mtd

#
#	kernel patches
#
test $# -lt 2 && exit 0
PATCHFILE=$2

if [ ! -f $PATCHFILE ]
then
	PATCHFILE=$THISDIR/$PATCHFILE
fi

if [ ! -f $PATCHFILE ]
then
	echo "patch-file" $PATCHFILE "not found."
	exit 1
fi

echo "Patching the kernel ..."
patch -p1 < $PATCHFILE

exit 0

