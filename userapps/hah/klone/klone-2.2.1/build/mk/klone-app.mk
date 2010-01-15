TOP = $(shell pwd)

# if makl is not configured use `pwd`/makl/
ifndef MAKL_DIR
MAKL_DIR_BUNDLED = true
export MAKL_DIR = $(TOP)/build/makl
export MAKEFLAGS = -I $(MAKL_DIR)/mk
endif

ifdef MAKL_PLATFORM
export MAKL_PLATFORM
endif

ifdef KLONE_TARGET_PATCH_FILE
export KLONE_TARGET_PATCH_FILE
endif
ifdef KLONE_TARGET_PATCH_URI
export KLONE_TARGET_PATCH_URI
endif
ifdef KLONE_HOST_PATCH_FILE
export KLONE_HOST_PATCH_FILE
endif
ifdef KLONE_HOST_PATCH_URI
export KLONE_HOST_PATCH_URI
endif

# be sure that makl dir exists
ifneq ($(shell [ -d $(MAKL_DIR) ]; echo $$? ),0)
$(error MaKL dir not found ($(MAKL_DIR)) )
endif

ifndef KLONE_VERSION
$(error KLONE_VERSION must be defined. )
endif

ALL = host-setup klone-setup subdirs klone-first-import \
	  $(KLONE_WEBAPP) make-pre klone-make make-post klone-howto

.PHONY: $(ALL) subdirs $(SUBDIR) import

# klone compiled for the target platform
KLONE_SRC_TARGET = $(TOP)/build/target/klone-core-$(KLONE_VERSION)

# klone compiled for the host platform
KLONE_SRC_HOST = $(TOP)/build/host/klone-core-$(KLONE_VERSION)

# klone top source dir
KLONE_SRC = $(KLONE_SRC_TARGET)
export KLONE_SRC

# flags needed to compile code that use klone libs
KLONE_CFLAGS = -I$(KLONE_SRC) -I$(KLONE_SRC)/libu/include
export KLONE_CFLAGS

# un set makl vars to be sure that klone will use its own version of makl  and
# run 'make'
KLONE_MAKE = env MAKL_DIR= MAKEFLAGS= $(MAKE) -C $(KLONE_SRC)

# runs make depend in klone-x.y.z/site
KLONE_SITE_DEPEND = env MAKL_DIR=$(KLONE_SRC)/makl MAKEFLAGS="-I $(KLONE_SRC)/makl/mk/" $(MAKE) -C $(KLONE_SRC)/site depend

KLONE_WEBAPP = $(KLONE_SRC_TARGET)/webapp/Makefile-webapp

# may be set by the user
WEBAPP_DIR ?= $(TOP)/webapp

# embfs top directory
KLONE_WEBAPP_DIR = $(WEBAPP_DIR)

# download dir
KLONE_DIST_DIR ?= $(TOP)/build/dist/
XENO_DIST_DIR ?= $(KLONE_DIST_DIR)

# exported vars
export KLONE_APP_TOP = $(TOP)
export KLONE_SRC_DIR = $(KLONE_SRC)
export XENO_DIST_DIR
export KLONE_VERSION

# deprecated vars (just for backward compatibility)
export KLONE_APP_DIR = $(TOP)
export KLONE_APP_TOPDIR = $(TOP)

ifdef KLONE_CUSTOM_TC
export MAKL_PLATFORM=custom
endif

# when cross-compiling build klone also for the host platform
ifdef MAKL_PLATFORM
KLONE_TOOL ?= $(KLONE_SRC_HOST)/src/tools/klone/klone
KLONE_CONF_ARGS += --cross_compile
export KLONE = $(KLONE_TOOL)
KLONE_IMPORT_ARGS ?= \
    $(shell grep -q HAVE_LIBZ $(KLONE_SRC_HOST)/Makefile.conf &&  \
            grep -q HAVE_LIBZ $(KLONE_SRC_TARGET)/Makefile.conf && \
            echo '-z')
else
KLONE_TOOL ?= $(KLONE_SRC_TARGET)/src/tools/klone/klone
KLONE_IMPORT_ARGS ?= \
    $(shell grep -q HAVE_LIBZ $(KLONE_SRC_TARGET)/Makefile.conf && echo '-z')
endif

ifeq ($(wildcard Makefile.conf),)
help-conf:
	@echo 
	@echo "  You must first run the configure script."
	@echo 
	@echo "  Run ./configure --help for the list of options"
	@echo 
endif

ifndef MAKL_DIR_BUNDLED
include target-options.mk
else
include $(MAKL_DIR)/mk/target-options.mk
endif

fetch-options = once
host-setup-options = once
klone-setup-options = once
klone-first-import-options = once
klone-howto-options = once

KLONE_URI = http://koanlogic.com/klone/klone-$(KLONE_VERSION).tar.bz2
KLONE_TARBALL ?= $(notdir $(XENO_FETCH_URI))

toolchain:
	@echo "==> init MaKL" ; (cd $(MAKL_DIR) && $(MAKE) toolchain rc)

host-setup:
ifdef MAKL_PLATFORM
	@echo "==> setup (host)..."
	@$(MAKE) -C build/host
	@export MAKL_PLATFORM= MAKL_TC= MAKEFLAGS= && \
		(cd $(KLONE_SRC_HOST) && ./configure --disable_cxx $(CONF_ARGS) )
	@export MAKL_PLATFORM= MAKL_TC= MAKEFLAGS= && \
		( cd $(KLONE_SRC_HOST) && $(MAKE) )
ifdef KLONE_CUSTOM_TC
	@(cp -f $(KLONE_CUSTOM_TC) $(MAKL_DIR)/tc/custom.tc )
	@(cd $(MAKL_DIR) && $(MAKE) toolchain )
endif
endif

klone-setup:
	@echo "==> setup..."
	@$(MAKE) -C build/target
ifdef KLONE_CUSTOM_TC
	@(cd $(KLONE_SRC) && cp -f $(KLONE_CUSTOM_TC) makl/tc/custom.tc )
endif
	@(cd $(KLONE_SRC) && ./configure $(KLONE_CONF_ARGS) )
	@$(KLONE_MAKE)
	@$(KLONE_MAKE) depend

import-pre import-post:

# to be run on user request (i.e. 'make import')
import: $(KLONE_WEBAPP)
	@echo "==> importing klone-app content..."
	@$(MAKE) import-pre
	@( cd $(KLONE_SRC)/site && \
		$(KLONE_TOOL) $(KLONE_IMPORT_ARGS) -c import $(KLONE_WEBAPP_DIR) )
	@$(KLONE_MAKE)
	@$(MAKE) import-post
	@$(KLONE_SITE_DEPEND)

klone-first-import:
	@echo "==> building..."
	@$(KLONE_MAKE)
	@( $(MAKE) import )

klone-depend: $(KLONE_SRC)/Makefile.conf $(KLONE_SRC)/site/autogen.dps
	@$(KLONE_MAKE) depend

# rebuild all
rebuild:
	@$(KLONE_MAKE) clean
	@$(KLONE_MAKE) 
	@$(MAKE) import

ifneq ($(wildcard $(KLONE_WEBAPP)),)
$(KLONE_WEBAPP): Makefile
	@echo "# this file is automatically generated, don't edit" >$(KLONE_WEBAPP)
	@echo "WEBAPP_SRCS ?= webapp.c" >> $(KLONE_WEBAPP)
	@echo "WEBAPP_CFLAGS ?= $(WEBAPP_CFLAGS)" >> $(KLONE_WEBAPP)
	@echo "WEBAPP_LDFLAGS ?= $(WEBAPP_LDFLAGS)" >> $(KLONE_WEBAPP)
	@echo "WEBAPP_LDADD ?= $(WEBAPP_LDADD)" >> $(KLONE_WEBAPP)
	@touch $(KLONE_SRC)/site/pg_nop.c
	@touch $(KLONE_SRC)/site/pg_*.c
	@rm -f .wc .wc.old                                                          
else
$(KLONE_WEBAPP):
endif

make-pre make-post:

klone-make: $(KLONE_WEBAPP)
	@echo "==> building..."
	@if [ -f .wc ]; then mv .wc .wc.old ; else touch .wc.old; fi
	@find $(KLONE_WEBAPP_DIR) > .wc
	@diff -q .wc .wc.old >/dev/null; \
	    if [ $$? -ne 0 ]; then \
		    ( $(MAKE) import || (rm -f .wc .wc.old; exit 1) ) \
		fi
	@$(KLONE_MAKE)
	@ln -fs $(KLONE_SRC)/src/kloned/kloned*

subdirs: $(SUBDIR)

$(SUBDIR):
	@echo "==> making subdir $@"
	@$(MAKE) -C $@

clean:
	@echo "==> cleaning... "
	@$(MAKE) -C build/target $@
	@$(MAKE) -C build/host $@
	@for dir in $$SUBDIR; do \
	    $(MAKE) -C $$dir $@ ; \
	done
	@rm -f .wc .wc.old
	@rm -f kloned kloned.exe

dist-clean: 
	@$(MAKE) -C build/makl toolchain
	@$(MAKE) -C build/target clean
	@$(MAKE) -C build/host clean
	@$(MAKE) -C build/makl clean
	@if [ -d $(KLONE_SRC_HOST) ]; then rm -rf $(KLONE_SRC_HOST) ; fi
	@if [ -d $(KLONE_SRC_TARGET) ]; then rm -rf $(KLONE_SRC_TARGET) ; fi
	@rm -f kloned kloned.exe
	@rm -f Makefile.conf klapp_conf.h

install:
	@echo "==> installing... "
	@$(KLONE_MAKE) install

howto:
	@echo 
	@echo "    KLone daemon (kloned) is ready to be started."
	@echo 
	@echo "    You can modify web pages or configuration (kloned.conf)" \
              "editing files"
	@echo "    under $(KLONE_WEBAPP_DIR) directory. "
	@echo 
	@echo "    Run 'make' afterwards to rebuild the daemon."
	@echo 
	@echo 

klone-howto:
	@$(MAKE) howto 

configure-help:
	@echo "==> configure-help..."
	@$(MAKE) -C build/target
	@(cd $(KLONE_SRC) && ./configure --help )

import-help:
	@echo
	@echo "Import options:"
	@echo "  -b URI      base URI"
	@echo "  -x pattern  exclude all files whose URI match the given pattern (*)"
	@echo
	@echo "Options available when compiled with OpenSSL support:"
	@echo "  -e pattern  encrypt all files whose URI match the given pattern (*)"
	@echo "  -k key_file encryption key filename"
	@echo
	@echo "Options available when compiled with zlib support:"
	@echo "  -z          compress all compressable content (based on MIME types)"
	@echo "  -Z pattern  compress all files whose URI match the given pattern (*)"
	@echo
	@echo "(*) may be used more then once"
	@echo

makefile-help:
	@echo 
	@echo "Displaying $(TOP)/Makefile.help file:"
	@echo 
	@cat $(TOP)/Makefile.help

