CRYPTDIR=libtomcrypt-1.17
MATHDIR=libtommath-0.41
CPPFLAGS=-I $(CRYPTDIR)/src/headers -I $(MATHDIR) -O3 -s -DLTM_DESC
LDFLAGS= -L $(CRYPTDIR) -L $(MATHDIR) -ltomcrypt -ltommath 

all: dwbtool

dwbtool: $(CRYPTDIR)/libtomcrypt.a $(MATHDIR)/libtommath.a
	g++ -o dwbtool $(CPPFLAGS) dwbtool.cpp $(LDFLAGS)

$(CRYPTDIR)/libtomcrypt.a:
	(cd $(CRYPTDIR); CFLAGS="-DLTM_DESC -I../$(MATHDIR)" make)

$(MATHDIR)/libtommath.a:
	(cd $(MATHDIR); make)

clean:
	(cd $(CRYPTDIR); make clean)
	(cd $(MATHDIR); make clean)
	rm -f *.o dwbtool
