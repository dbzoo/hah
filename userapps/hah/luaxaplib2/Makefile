#------
# Load configuration
#
include config

INSTALL_XAPLIB_LIB=$(INSTALL_TOP_LIB)

all clean:
	cd src; $(MAKE) $@

#------
# Files to install
#
TO_TOP_SHARE:= \
	xaplib.lua

#------
# Install LuaXaplib according to recommendation
#
install: all
#	cd src; mkdir -p $(INSTALL_TOP_SHARE)
#	cd src; $(INSTALL_DATA) $(TO_TOP_SHARE) $(INSTALL_TOP_SHARE)
	cd src; mkdir -p $(INSTALL_TOP_LIB)
	cd src; $(INSTALL_EXEC) $(XAPLIB_SO) $(INSTALL_XAPLIB_LIB)

#------
# End of makefile
#
