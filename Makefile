# $Id$

.PHONY: clean

BUILD_DIR = $(shell pwd)
USERAPPS_DIR = $(BUILD_DIR)/userapps
TARGET_DIR = $(BUILD_DIR)/build
INSTALL_DIR ?= $(TARGET_DIR)/sysroot

OPENSOURCE_DIR=$(USERAPPS_DIR)/opensource
SUBDIRS_OPENSOURCE = \
        $(OPENSOURCE_DIR)/ini \
	$(OPENSOURCE_DIR)/jsmn \
	$(OPENSOURCE_DIR)/penlight \
	$(OPENSOURCE_DIR)/mqtt_lua

HAH_DIR=$(USERAPPS_DIR)/hah
SUBDIRS_HAH = \
	$(HAH_DIR)/xaplib2 \
	$(HAH_DIR)/xap-hub \
	$(HAH_DIR)/xap-livebox \
	$(HAH_DIR)/xap-snoop \
	$(HAH_DIR)/xap-xively \
	$(HAH_DIR)/xap-sms \
	$(HAH_DIR)/xap-currentcost \
	$(HAH_DIR)/xap-twitter \
	$(HAH_DIR)/xap-plugboard \
	$(HAH_DIR)/xap-serial \
	$(HAH_DIR)/iServer \
	$(HAH_DIR)/xap-flash \
	$(HAH_DIR)/xap-urfrx \
	$(HAH_DIR)/xap-mail \
	$(HAH_DIR)/klone

SUBDIRS = $(SUBDIRS_OPENSOURCE) $(SUBDIRS_HAH)

OPENSOURCE_APPS = ini jsmn penlight mqtt_lua
HAH_APPS = xaplib2 xap-hub xap-livebox xap-snoop xap-xively xap-sms iServer \
	xap-currentcost xap-twitter xap-serial xap-plugboard \
	xap-flash xap-urfrx xap-mail klone

ALLAPPS = $(OPENSOURCE_APPS) $(HAH_APPS)

help:
	@echo "List of valid targets:"
	@echo "  all                Build all programs"
	@echo
	@echo "  install            Build all programs and install into"
	@echo "                     $(INSTALL_DIR)"
	@echo
	@echo "                     You may override for direct installation"
	@echo "                     INSTALL_DIR=/ make install"
	@echo
	@echo "  arm-deb            Build a Debian installation package into"
	@echo "                     $(TARGET_DIR)"
	@echo
	@echo "  clean              clean all"
	@echo
	@echo "  help               display this help"
	@echo 

all: $(ALLAPPS)

xap-twitter: jsmn

$(OPENSOURCE_APPS):
	$(MAKE) -C $(OPENSOURCE_DIR)/$@

$(HAH_APPS): xaplib2
	$(MAKE) -C $(HAH_DIR)/$@

install: 
	@mkdir -p $(INSTALL_DIR)/usr/bin $(INSTALL_DIR)/usr/lib $(INSTALL_DIR)/etc/xap.d
	@for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir install ;\
	done
	install -m 644 package/etc/xap.d/build $(INSTALL_DIR)/etc/xap.d/
	install -D -m 755 package/etc/init.d/xap $(INSTALL_DIR)/etc/init.d/xap

arm-deb: install
	install -d $(INSTALL_DIR)/DEBIAN
	install -m 644 package/arm-DEBIAN/conffiles $(INSTALL_DIR)/DEBIAN/
	install -m 644 package/arm-DEBIAN/control $(INSTALL_DIR)/DEBIAN/
	install -m 755 package/arm-DEBIAN/postinst $(INSTALL_DIR)/DEBIAN/
	install -m 755 package/arm-DEBIAN/postrm $(INSTALL_DIR)/DEBIAN/
	install -m 755 package/arm-DEBIAN/prerm $(INSTALL_DIR)/DEBIAN/
	dpkg-deb -b $(INSTALL_DIR) $(TARGET_DIR)

clean: app_clean
	rm -fr $(INSTALL_DIR)

app_clean:
	@for dir in $(SUBDIRS); do \
	  $(MAKE) -C $$dir clean ;\
	done

export INSTALL_DIR
