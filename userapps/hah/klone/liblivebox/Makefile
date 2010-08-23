# always include common.mk in MaKL-based makefiles
include common.mk

# include klone-related variables
include ../vars.mk

# uncommment to enable debugging
#KLONE_CONF_ARGS = --enable_debug
KLONE_CONF_ARGS += --enable_hooks

# name of the library
LIB = livebox

# list of source files
SRCS = comms.c minIni.c const.c hooks.c utils.c tcurl.c oauth.c hash.c xmalloc.c

# compilation flags
OPENSRC = ../../../opensource
CURL=$(OPENSRC)/curl/include
OPENSSL=$(OPENSRC)/openssl/include
CFLAGS += -I../include -I$(CURL) -I$(OPENSSL) $(KLONE_CFLAGS)

include lib.mk
