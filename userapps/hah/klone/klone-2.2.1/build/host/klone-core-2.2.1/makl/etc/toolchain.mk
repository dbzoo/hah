# GNU Make compatible toolchain file.
# Autogenerated by MaKL - Sun Dec 13 10:59:44 GMT 2009
export CC = cc
export CPP = ${CC} -E
export CXX = g++
export CFLAGS = -pipe
export CXXFLAGS = ${CFLAGS}
export AR = ar
export ARFLAGS = cq
export RANLIB = ranlib
export LD = ld
export LDFLAGS = 
export NM = nm
export STRIP = strip
export STRIP_FLAGS = -x
export INSTALL = install
export INSTALL_COPY = -c
export TSORT = tsort
export PRE_LDADD = -Wl,--start-group
export POST_LDADD = -Wl,--end-group
export MKINSTALLDIRS = "${MAKL_DIR}/helpers/mkinstalldirs"
export MKDEP = "${MAKL_DIR}/helpers/mkdep.gcc.sh"
export LORDER = "${MAKL_DIR}/helpers/lorder"
export AWK = awk
export CAT = cat
export CP = cp
export CUT = cut
export ECHO = echo
export GREP = grep
export MV = mv
export RM = rm
export SED = sed
export TOUCH = touch
export TR = tr
export UNAME = uname
export CC = gcc
export MKINSTALLDIRS = mkdir -p
