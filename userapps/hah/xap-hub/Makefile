# Patrick Lidstone, Copyright(c) 2002 Patrick Lidstone
# Last modified: 18 Dec 2002
#

XAP=hub
CFLAGS=-Os -Wall -DLINUX -I$(INSTALL_DIR)/include
LDFLAGS=-L$(INSTALL_DIR)/lib -lxap

xap-$(XAP): $(XAP).o
	$(CC) -o xap-$(XAP) $(XAP).o $(LDFLAGS)

clean:
	rm -f xap-$(XAP) core *.o
