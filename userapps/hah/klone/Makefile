# $Id$
#
SHELL=/bin/bash
KLONE_VERSION = 2.3.0

# webapp content is in webapp/ the current dir
WEBAPP_DIR = $(shell pwd)/webapp

#MAKL_PLATFORM = linux

#KLONE_CONF_ARGS = --enable_debug
KLONE_CONF_ARGS += --enable_hooks --enable_sup_fs --disable-lib=openssl --disable-lib=z --disable_cxx --disable-lib=openssl_psk

# also run make in the build directory (must be absolute paths)
SUBDIR = $(shell pwd)/liblivebox

# include directories
WEBAPP_CFLAGS = -I$(CURDIR)/include
WEBAPP_LDFLAGS = -lcrypto -lcurl -lm

# static libraries
WEBAPP_LDADD = $(shell pwd)/liblivebox/liblivebox.a 

include klapp.mk

clean-pre:
	rm -rf klone-$(KLONE_VERSION)

klapp.mk:
	wget -O $@ -c http://koanlogic.com/klone/klapp-2.2.1.mk
