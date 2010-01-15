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
SRCS = comms.c minIni.c const.c hooks.c utils.c

# compilation flags
CFLAGS += -I ../include $(KLONE_CFLAGS)

include lib.mk
