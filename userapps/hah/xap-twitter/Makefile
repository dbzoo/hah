# $Id$

XAP=twitter

# uncomment for standalone compile
#INSTALL_DIR=/usr
CFLAGS=-g -DLINUX -DIDENT -I$(INSTALL_DIR)/include
CFLAGS+=-I../xaplib2 -I../../opensource/openssl/include -I../../opensource/jsmn
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap2 -lcurl -ljsmn -lm
LDFLAGS+=-L../xaplib2 -L../../opensource/jsmn
MAIN=$(XAP).o 
OBJECTS=tcurl.o oauth.o xmalloc.o hash.o

xap-$(XAP): $(MAIN) $(OBJECTS)
	$(CC) -o xap-$(XAP) $(MAIN) $(OBJECTS) $(LDFLAGS)

deleteLastTweet: deleteLastTweet.o $(OBJECTS)
	$(CC) -o deleteLastTweet deleteLastTweet.o $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) $(MAIN) core
