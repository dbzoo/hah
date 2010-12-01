#------
# Load configuration
#
include ../config

#------
# Modules belonging to xaplib-core
#
OBJECTS=xap_wrap.o

# For some weird reason xap_wrap.o when dl loaded won't then LDD libxap2.so
# Include the objects for now...
EXTLIB=../../xaplib2
OBJECTS+=$(EXTLIB)/init.o $(EXTLIB)/parse.o $(EXTLIB)/timeout.o $(EXTLIB)/tx.o \
         $(EXTLIB)/rx.o $(EXTLIB)/filter.o $(EXTLIB)/bsc.o $(EXTLIB)/log.o \
         $(EXTLIB)/strlcpy.o $(EXTLIB)/strlcat.o $(EXTLIB)/minIni.o $(EXTLIB)/util.o 

all: $(XAPLIB_SO)
	install -m 755 $(XAPLIB_SO) $(PREFIX)/lib/lua/5.1
	$(STRIP) $(PREFIX)/lib/lua/5.1/$(XAPLIB_SO)

$(XAPLIB_SO): $(OBJECTS)
	$(CC) -shared $(LDFLAGS) -o $@ $(OBJECTS)

xap_wrap.o: xap_wrap.c

#------
# List of dependencies
#
xap_wrap.c: xap.i
	$(SWIG) -lua xap.i

clean:
	rm -f $(XAPLIB_SO) $(OBJECTS) xap_wrap.c

#------
# End of makefile configuration
#
