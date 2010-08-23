# $Id$
#
KLONE_VERSION = 2.2.1

# webapp content is in webapp/ the current dir
WEBAPP_DIR = $(CURDIR)/webapp

MAKL_PLATFORM = linux-mips

#KLONE_CONF_ARGS = --enable_debug
KLONE_CONF_ARGS += --enable_hooks

# also run make in the build directory (must be absolute paths)
SUBDIR = $(CURDIR)/liblivebox

# include directories
OPENSRC = $(CURDIR)/../../opensource
CURL=$(OPENSRC)/curl
OPENSSL=$(OPENSRC)/openssl
WEBAPP_CFLAGS = -I$(CURDIR)/include -I$(CURL)/include -I$(OPENSSL)/include
WEBAPP_LDFLAGS = -L$(CURDIR)/../../../targets/fs.src/lib -lcurl

# static libraries
WEBAPP_LDADD = $(CURDIR)/liblivebox/liblivebox.a 

include klapp.mk

klapp.mk:
	wget -O $@ -c http://koanlogic.com/klone/klapp-2.0.0.mk
