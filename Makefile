# $Id$

.PHONY: package clean

BUILD_DIR = $(shell pwd)
USERAPPS_DIR = $(BUILD_DIR)/userapps
TARGET_DIR = $(BUILD_DIR)/build
INSTALL_DIR = $(TARGET_DIR)/sysroot

CONFIG_SHELL := $(shell if [ -x "$$BASH" ]; then echo $$BASH; \
          else if [ -x /bin/bash ]; then echo /bin/bash; \
          else echo sh; fi ; fi)

export OPENSOURCE_DIR=$(USERAPPS_DIR)/opensource
SUBDIRS_OPENSOURCE = \
        $(OPENSOURCE_DIR)/ini \
	$(OPENSOURCE_DIR)/jsmn

HAH_DIR=$(USERAPPS_DIR)/hah
SUBDIRS_HAH = \
	$(HAH_DIR)/xaplib2 \
	$(HAH_DIR)/xap-hub \
	$(HAH_DIR)/xap-livebox \
	$(HAH_DIR)/xap-snoop \
	$(HAH_DIR)/xap-xively \
	$(HAH_DIR)/xap-sms \
	$(HAH_DIR)/xap-currentcost \
	$(HAH_DIR)/xap-googlecal \
	$(HAH_DIR)/xap-twitter \
	$(HAH_DIR)/xap-plugboard \
	$(HAH_DIR)/xap-serial \
	$(HAH_DIR)/iServer \
	$(HAH_DIR)/xap-flash \
	$(HAH_DIR)/xap-urfrx \
	$(HAH_DIR)/klone

SUBDIRS = $(SUBDIRS_OPENSOURCE) $(SUBDIRS_HAH)

OPENSOURCE_APPS = ini jsmn
HAH_APPS = xaplib2 xap-hub xap-livebox xap-snoop xap-xively xap-sms iServer \
	xap-currentcost xap-twitter xap-serial xap-plugboard \
	xap-flash xap-urfrx klone

ALLAPPS = $(OPENSOURCE_APPS) $(HAH_APPS)

all: app

app: prebuild $(ALLAPPS)

prebuild:
	mkdir -p $(INSTALL_DIR)/usr/bin $(INSTALL_DIR)/usr/lib $(INSTALL_DIR)/etc

jsmn:
	$(MAKE) -C $(OPENSOURCE_DIR)/jsmn

xaplib2:
	$(MAKE) -C $(HAH_DIR)/xaplib2
	install -s -m 644 $(HAH_DIR)/xaplib2/libxap2.so $(INSTALL_DIR)/usr/lib

xap-hub: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-hub
	install -s -m 755 $(HAH_DIR)/xap-hub/xap-hub $(INSTALL_DIR)/usr/bin

xap-urfrx: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-urfrx
	install -s -m 755 $(HAH_DIR)/xap-urfrx/xap-urfrx $(INSTALL_DIR)/usr/bin

xap-livebox: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-livebox
	install -s -m 755 $(HAH_DIR)/xap-livebox/xap-livebox $(INSTALL_DIR)/usr/bin

xap-sms: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-sms
	install -s -m 755 $(HAH_DIR)/xap-sms/xap-sms $(INSTALL_DIR)/usr/bin

xap-snoop: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-snoop
	install -s -m 755 $(HAH_DIR)/xap-snoop/xap-snoop $(INSTALL_DIR)/usr/bin

xap-serial: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-serial
	install -s -m 755 $(HAH_DIR)/xap-serial/xap-serial $(INSTALL_DIR)/usr/bin

xap-xively: xaplib2
	$(MAKE) -C $(HAH_DIR)/xap-xively
	install -s -m 755 $(HAH_DIR)/xap-xively/xap-xively $(INSTALL_DIR)/usr/bin

xap-currentcost: xaplib2 
	$(MAKE) -C $(HAH_DIR)/xap-currentcost
	install -s -m 755 $(HAH_DIR)/xap-currentcost/xap-currentcost $(INSTALL_DIR)/usr/bin

xap-googlecal: xaplib2 libgcal
	$(MAKE) -C $(HAH_DIR)/xap-googlecal
	install -s -m 755 $(HAH_DIR)/xap-googlecal/xap-googlecal $(INSTALL_DIR)/usr/bin

xap-twitter: xaplib2 jsmn
	$(MAKE) -C $(HAH_DIR)/xap-twitter
	install -s -m 755 $(HAH_DIR)/xap-twitter/xap-twitter $(INSTALL_DIR)/usr/bin

xap-plugboard:
	$(MAKE) -C $(HAH_DIR)/xap-plugboard

xap-flash:
	$(MAKE) -C $(HAH_DIR)/xap-flash

iServer: xaplib2
	$(MAKE) -C $(HAH_DIR)/iServer
	install -s -m 755 $(HAH_DIR)/iServer/iServer $(INSTALL_DIR)/usr/bin

klone:
	$(MAKE) -C $(HAH_DIR)/klone
	install -s -m 755 $(HAH_DIR)/klone/kloned $(INSTALL_DIR)/usr/bin
	install -m 644 $(HAH_DIR)/klone/webapp/etc/kloned.conf $(INSTALL_DIR)/etc

ini:
	$(MAKE) -C $(OPENSOURCE_DIR)/ini install

bb-package:
	rm -rf $(INSTALL_DIR)/DEBIAN
	mkdir $(INSTALL_DIR)/DEBIAN
	cp -r package/beaglebone/* $(INSTALL_DIR)/DEBIAN
	cp -r package/etc $(INSTALL_DIR)
	dpkg-deb -b $(INSTALL_DIR) $(TARGET_DIR)

clean: app_clean
	rm -fr $(INSTALL_DIR)

app_clean:
	for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean ;\
	done

export  INSTALL_DIR
