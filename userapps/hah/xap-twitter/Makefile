# $Id$

XAP=twitter

CFLAGS=-DLINUX -DIDENT
CFLAGS+=-I../xaplib2 -I../../opensource/jsmn
LDFLAGS=-lxap2 -lcrypto -lcurl -ljsmn -lm
LDFLAGS+=-L../xaplib2 -L../../opensource/jsmn
MAIN=$(XAP).o 
OBJECTS=tcurl.o oauth.o xmalloc.o hash.o oauth_http.o

xap-$(XAP): $(MAIN) $(OBJECTS)
	$(CC) -o xap-$(XAP) $(MAIN) $(OBJECTS) $(LDFLAGS)

deleteLastTweet: deleteLastTweet.o $(OBJECTS)
	$(CC) -o deleteLastTweet deleteLastTweet.o $(OBJECTS) $(LDFLAGS)

clean:
	$(RM) -f xap-$(XAP) $(OBJECTS) $(MAIN) core
