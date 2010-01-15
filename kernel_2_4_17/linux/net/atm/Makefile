#
# Makefile for the ATM Protocol Families.
#
# Note! Dependencies are done automagically by 'make dep', which also
# removes any old dependencies. DON'T put your own dependencies here
# unless it's something special (ie not a .c file).
#
# Note 2! The CFLAGS definition is now in the main makefile...

# CONFIG_MIPS_BRCM Begin Broadcom changed code.
include ../../.config
# CONFIG_MIPS_BRCM end Broadcom changed code.

O_TARGET	:= atm.o

export-objs 	:= common.o atm_misc.o raw.o resources.o ipcommon.o proc.o

# CONFIG_MIPS_BRCM Begin Broadcom changed code.
#list-multi	:= mpoa.o
multi-list 	:= mpoa.o
mpoa-objs	:= mpc.o mpoa_caches.o mpoa_proc.o
# CONFIG_MIPS_BRCM end Broadcom changed code.

obj-$(CONFIG_ATM) := addr.o pvc.o signaling.o svc.o common.o atm_misc.o raw.o resources.o

ifeq ($(CONFIG_ATM_CLIP),y)
obj-y += clip.o
NEED_IPCOM = ipcommon.o
endif

# CONFIG_MIPS_BRCM Begin Broadcom added code.
ifeq ($(CONFIG_ATM_BR2684),y)
  NEED_IPCOM = ipcommon.o
else
  ifeq ($(CONFIG_ATM_BR2684),m)
	NEED_IPCOM = ipcommon.o
  endif
endif
obj-$(CONFIG_ATM_BR2684) += br2684.o
obj-$(CONFIG_ATM_RT2684) += rt2684.o
# CONFIG_MIPS_BRCM End Broadcom added code.


ifeq ($(CONFIG_NET_SCH_ATM),y)
NEED_IPCOM = ipcommon.o
endif

obj-y += $(NEED_IPCOM)

ifeq ($(CONFIG_PROC_FS),y)
obj-y += proc.o
endif

obj-$(CONFIG_ATM_LANE) += lec.o
obj-$(CONFIG_ATM_MPOA) += mpoa.o
obj-$(CONFIG_PPPOATM) += pppoatm.o

include $(TOPDIR)/Rules.make

mpoa.o: $(mpoa-objs)
	$(LD) -r -o mpoa.o $(mpoa-objs)
