/*
 * Low-Level PCI and SB support for BCM47xx (Linux support code)
 *
 * Copyright 2003, Broadcom Corporation
 * All Rights Reserved.                
 *                                     
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;   
 * the contents of this file may not be disclosed to third parties, copied
 * or duplicated in any form, in whole or in part, without the prior      
 * written permission of Broadcom Corporation.                            
 *
 * $Id: pcibios.c,v 1.6 2003/04/23 00:17:57 mhuang Exp $ 
 */

#include <linux/config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/paccess.h>

#include <6348_intr.h>
#include <6348_map_part.h>
#include <board.h>

#define BCM6348_A0          /* apply to A0 chip only */

#define pciMemRange1        PCI_SIZE_16M  /* 16 MB */
#define pciMemBase1         0x08000000
#define pciRemap1           0x08000000
#define pciMemRange2        PCI_SIZE_16M  /* 16 MB */
#define pciMemBase2         0x09000000
#define pciRemap2           0x09000000
#define pciIoRange          PCI_SIZE_64K  /* 64 KB */
#define pciIoBase           0x0C000000
#define pciIoRemap          0x0C000000

#define pciAddrMask         0x1FFFFFFF

#define sysMemSpace1        0x10000000
#define sysMemSpace2        0x00000000

/* 
 * EBI bus clock is 33MHz and share with PCI bus
 * each clock cycle is 30ns.
 */
/* attribute memory access wait cnt for 4306 */
#define PCMCIA_ATTR_CE_HOLD     3  // data hold time 70ns
#define PCMCIA_ATTR_CE_SETUP    3  // data setup time 50ns
#define PCMCIA_ATTR_INACTIVE    6  // time between read/write cycles 180ns. For the total cycle time 600ns (cnt1+cnt2+cnt3+cnt4)
#define PCMCIA_ATTR_ACTIVE      10 // OE/WE pulse width 300ns

/* common memory access wait cnt for 4306 */
#define PCMCIA_MEM_CE_HOLD      1  // data hold time 30ns
#define PCMCIA_MEM_CE_SETUP     1  // data setup time 30ns
#define PCMCIA_MEM_INACTIVE     2  // time between read/write cycles 40ns. For the total cycle time 250ns (cnt1+cnt2+cnt3+cnt4)
#define PCMCIA_MEM_ACTIVE       5  // OE/WE pulse width 150ns

#define PCCARD_VCC_MASK     0x00070000  // Mask Reset also
#define PCCARD_VCC_33V      0x00010000
#define PCCARD_VCC_50V      0x00020000

typedef enum {
    MPI_CARDTYPE_NONE,      // No Card in slot
    MPI_CARDTYPE_PCMCIA,    // 16-bit PCMCIA card in slot    
    MPI_CARDTYPE_CARDBUS,   // 32-bit CardBus card in slot
}   CardType;

#define CARDBUS_SLOT        0    // Slot 0 is default for CardBus

#define PCI_CFG_6348(d, f, o)  ( (d << 11) | (f << 8) | (o/4 << 2) )

#define pcmciaAttrOffset    0x00200000
#define pcmciaMemOffset     0x00000000
// Needs to be right above PCI I/O space. Give 0x8000 (32K) to PCMCIA. 
#define pcmciaIoOffset      (pciIoBase + 0x80000)
// Base Address is that mapped into the MPI ChipSelect registers. 
// UBUS bridge MemoryWindow 0 outputs a 0x00 for the base.
#define pcmciaBase          0xbf000000
#define pcmciaAttr          (pcmciaAttrOffset | pcmciaBase)
#define pcmciaMem           (pcmciaMemOffset  | pcmciaBase)
#define pcmciaIo            (pcmciaIoOffset   | pcmciaBase)

#if defined(CONFIG_USB)
#if 0
#define DPRINT(x...)        printk(x)
#else
#define DPRINT(x...)
#endif
#define USB_HOST_SLOT       9

static int 
pci63xx_int_read(struct pci_dev * dev, int where, void * value, int len);
static int 
pci63xx_int_write(struct pci_dev * dev, int where, void * value, int len);
#endif

static int mpi_DetectPcCard(void);

/* Global SB handle */
spinlock_t mpi_lock = SPIN_LOCK_UNLOCKED;
static volatile MpiRegisters * mpi = (MpiRegisters *)(MPI_BASE);

static void mpi_SetupPciConfigAccess(uint32 addr)
{
    mpi->l2pcfgctl = (DIR_CFG_SEL | DIR_CFG_USEREG | addr) & ~CONFIG_TYPE;
}

static void mpi_ClearPciConfigAccess(void)
{
    mpi->l2pcfgctl = 0x00000000;
}

static void mpi_SetLocalPciConfigReg(uint32 reg, uint32 value)
{
    /* write index then value */
    mpi->pcicfgcntrl = PCI_CFG_REG_WRITE_EN + reg;;
    mpi->pcicfgdata = value;
}

static uint32 mpi_GetLocalPciConfigReg(uint32 reg)
{
    /* write index then get value */
    mpi->pcicfgcntrl = PCI_CFG_REG_WRITE_EN + reg;;
    return mpi->pcicfgdata;
}

static int mpi_GetPciConfigReg(int bus, int dev, int func, int offset, void *data, int len)
{
    volatile unsigned char *ioBase = (unsigned char *)(mpi->l2piobase | 0xA0000000);
    uint8 *p8 = (uint8 *)data;
    uint16 *p16 = (uint16 *)data;
    uint32 *p32 = (uint32 *)data;
    int bitoffset;
    uint32 regval;

    memset(data, 0xff, len);

    if (bus)
        return -1;

    mpi_SetupPciConfigAccess(PCI_CFG_6348(dev, func, offset));
    regval = *(uint32 *)ioBase;
    bitoffset = (offset % 4) * 8;
    switch(len) {
        case 1:
            *p8 =  (regval >> bitoffset);
            break;
        case 2:
            *p16 = (regval >> bitoffset);
            break;
        case 4:
            *p32 = (regval >> bitoffset);
            break;
        default:
            break;
    }
    mpi_ClearPciConfigAccess();

    return 0;
}

static int mpi_SetPciConfigReg(int bus, int dev, int func, int offset, void *data, int len)
{
    volatile unsigned char *ioBase = (unsigned char *)(mpi->l2piobase | 0xA0000000);
    uint32 data8 = *(uint8 *)data;
    uint32 data16 = *(uint16 *)data;
    uint32 data32 = *(uint32 *)data;
    int bitoffset;
    uint32 regval;
    uint32 mask;

    if (bus)
        return -1;

    mpi_SetupPciConfigAccess(PCI_CFG_6348(dev, func, offset));
    regval = *(uint32 *)ioBase;
    bitoffset = (offset % 4) * 8;
    switch(len) {
        case 1:
            mask = 0x000000ff << bitoffset;
            regval &= ~mask;
            regval |= (data8 << bitoffset);
            *(uint32 *)ioBase = regval;
            break;
        case 2:
            mask = 0x0000ffff << bitoffset;
            regval &= ~mask;
            regval |= (data16 << bitoffset);
            *(uint32 *)ioBase = regval;
            break;
        case 4:
            *(uint32 *)ioBase = data32;
            break;
        default:
            break;
    }
    mpi_ClearPciConfigAccess();

    return 0;
}

static int
mpi_read_config_byte(struct pci_dev *dev, int where, u8 *value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_read(dev, where, value, 1);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_GetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, value, sizeof(*value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static int
mpi_read_config_word(struct pci_dev *dev, int where, u16 *value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_read(dev, where, value, 2);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_GetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, value, sizeof(*value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static int
mpi_read_config_dword(struct pci_dev *dev, int where, u32 *value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_read(dev, where, value, 4);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_GetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, value, sizeof(*value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static int
mpi_write_config_byte(struct pci_dev *dev, int where, u8 value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_write(dev, where, &value, 1);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_SetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, (void *)&value, sizeof(value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static int
mpi_write_config_word(struct pci_dev *dev, int where, u16 value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_write(dev, where, &value, 2);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_SetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, (void *)&value, sizeof(value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static int
mpi_write_config_dword(struct pci_dev *dev, int where, u32 value)
{
    unsigned long flags;
    int ret;

#if defined(CONFIG_USB)
    if (PCI_SLOT(dev->devfn) == USB_HOST_SLOT)
        return pci63xx_int_write(dev, where, &value, 4);
#endif

    spin_lock_irqsave(&mpi_lock, flags);
    ret = mpi_SetPciConfigReg(dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn), where, (void *)&value, sizeof(value));
    spin_unlock_irqrestore(&mpi_lock, flags);
    return ret ? PCIBIOS_DEVICE_NOT_FOUND : PCIBIOS_SUCCESSFUL;
}

static struct pci_ops pcibios_ops = {
    mpi_read_config_byte,
    mpi_read_config_word,
    mpi_read_config_dword,
    mpi_write_config_byte,
    mpi_write_config_word,
    mpi_write_config_dword
};

static int mpi_init(void)
{
    unsigned long data;
    /*
     * Init the pci interface 
     */
    data = GPIO->GPIOMode; // GPIO mode register
    data |= GROUP2_PCI | GROUP1_MII_PCCARD; // PCI internal arbiter + Cardbus
    GPIO->GPIOMode = data; // PCI internal arbiter

    /*
     * In the BCM6348 CardBus support is defaulted to Slot 0
     * because there is no external IDSEL for CardBus.  To disable
     * the CardBus and allow a standard PCI card in Slot 0 
     * set the cbus_idsel field to 0x1f.
    */
    /*
    uData = mpi->pcmcia_cntl1;
    uData |= CARDBUS_IDSEL;
    mpi->pcmcia_cntl1 = uData;
    */

    // UBUS to PCI address range
    // 16MBytes Memory Window 1. Mask determines which bits are decoded.
    mpi->l2pmrange1 = pciMemRange1;
    // UBUS to PCI Memory base address. This is akin to the ChipSelect base
    // register. 
    mpi->l2pmbase1 = pciMemBase1 & pciAddrMask;
    // UBUS to PCI Remap Address. Replaces the masked address bits in the
    // range register with this setting. 
    // Also, enable direct I/O and direct Memory accesses
    mpi->l2pmremap1 = (pciRemap1 | MEM_WINDOW_EN);

    // 16MBytes Memory Window 2
    mpi->l2pmrange2 = pciMemRange2;
    // UBUS to PCI Memory base address. 
    mpi->l2pmbase2 = pciMemBase2 & pciAddrMask;
    // UBUS to PCI Remap Address
    mpi->l2pmremap2 = (pciRemap2 | MEM_WINDOW_EN);

    // Setup PCI I/O Window range. Give 32K to PCI I/O
    mpi->l2piorange = pciIoRange;
    // UBUS to PCI I/O base address 
    mpi->l2piobase = pciIoBase & pciAddrMask;
    // UBUS to PCI I/O Window remap
    mpi->l2pioremap = (pciIoRemap | MEM_WINDOW_EN);

    // enable PCI related GPIO pins and data swap between system and PCI bus
    mpi->locbuscntrl = (EN_PCI_GPIO | DIR_U2P_NOSWAP);

    /* Enable 6348 BusMaster and Memory access mode */
    data = mpi_GetLocalPciConfigReg(PCI_COMMAND);
    data |= (PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
    mpi_SetLocalPciConfigReg(PCI_COMMAND, data);

    /* Configure two 16 MByte PCI to System memory regions. */
    /* These memory regions are used when PCI device is a bus master */
    /* Accesses to the SDRAM from PCI bus will be "byte swapped" for this region */
    mpi_SetLocalPciConfigReg(PCI_BASE_ADDRESS_3, sysMemSpace1);
    mpi->sp0remap = 0x0;

    /* Accesses to the SDRAM from PCI bus will not be "byte swapped" for this region */
    mpi_SetLocalPciConfigReg(PCI_BASE_ADDRESS_4, sysMemSpace2);
    mpi->sp1remap = 0x0;
    mpi->pcimodesel |= (PCI_BAR2_NOSWAP | 0x40);

    /*
     * Change 6348 PCI Cfg Reg. offset 0x40 to PCI memory read retry count infinity
     * by set 0 in bit 8~15.  This resolve read Bcm4306 srom return 0xffff in
     * first read.
     */
    data = mpi_GetLocalPciConfigReg(BRCM_PCI_CONFIG_TIMER);
    data &= ~BRCM_PCI_CONFIG_TIMER_RETRY_MASK;
    data |= 0x00000080;
    mpi_SetLocalPciConfigReg(BRCM_PCI_CONFIG_TIMER, data);

    /* enable pci interrupt */
    mpi->locintstat |= (EXT_PCI_INT << 16);

    mpi_DetectPcCard();

    return 0;
}

void __init
pcibios_init(void)
{
    ulong flags;

    spin_lock_init(&mpi_lock);

    spin_lock_irqsave(&mpi_lock, flags);
    mpi_init();
    spin_unlock_irqrestore(&mpi_lock, flags);

    /* Scan the SB bus */
    pci_scan_bus(0, &pcibios_ops, NULL);
}

char * __init
pcibios_setup(char *str)
{
    if (!strncmp(str, "ban=", 4)) {
        //mpi_ban(simple_strtoul(str + 4, NULL, 0));
        return NULL;
    }

    return (str);
}

static u32 pci_iobase = pciIoRemap;
static u32 cb_membase = pciRemap1;
static u32 pci_membase = pciRemap2;

void __init
pcibios_fixup_bus(struct pci_bus *b)
{
    struct list_head *ln;
    struct pci_dev *d;
    struct resource *res;
    int pos, size;
    u32 *base;
    char irq;
    u32 *membase;

    printk("PCI: Fixing up bus %d\n", b->number);

    /* Fix up external PCI */
    for (ln=b->devices.next; ln != &b->devices; ln=ln->next) {
        d = pci_dev_b(ln);
#if defined(CONFIG_USB)
        if (PCI_SLOT(d->devfn) == USB_HOST_SLOT) {
            // No need to allocate PCI memory resources
            d->resource[0].start = USB_HOST_BASE;
            d->resource[0].end   = USB_HOST_BASE + 0x7FF;
            d->irq = INTERRUPT_ID_USBH;

            PERF->blkEnables |= USBH_CLK_EN;
            mdelay(100);
            *USBH_NON_OHCI = NON_OHCI_BYTE_SWAP;

            // Enable Device. Do we need to do anything?
            
            continue;
        }
#endif
        if (PCI_SLOT(d->devfn) == 0)
            membase = &cb_membase;
        else
            membase = &pci_membase;

        /* Fix up resource bases */
        for (pos = 0; pos < 4; pos++) {
            res = &d->resource[pos];
            base = (res->flags & IORESOURCE_IO) ? &pci_iobase : membase;
            if (res->end) {
                size = res->end - res->start + 1;
                if (*base & (size - 1))
                    *base = (*base + size) & ~(size - 1);
                res->start = *base;
                res->end = res->start + size - 1;
                *base += size;
                pci_write_config_dword(d, PCI_BASE_ADDRESS_0 + (pos << 2), res->start);
            }
        }
        /* Fix up interrupt lines */
        pci_write_config_byte(d, PCI_INTERRUPT_LINE, INTERRUPT_ID_MPI);
        pci_read_config_byte(d, PCI_INTERRUPT_LINE, &irq);
        d->irq = irq;
    }
}

unsigned int pcibios_assign_all_busses(void)
{
    return 1;
}

void pcibios_align_resource(void *data, struct resource *res,
			    unsigned long size)
{
}

int pcibios_enable_resources(struct pci_dev *dev)
{
    u16 cmd, old_cmd;
    int idx;
    struct resource *r;

    if (dev->bus->number != 0)
        return 0;

    pci_read_config_word(dev, PCI_COMMAND, &cmd);
    old_cmd = cmd;
    for(idx=0; idx<5; idx++) {
        r = &dev->resource[idx];
        if (r->flags & IORESOURCE_IO)
            cmd |= PCI_COMMAND_IO;
        if (r->flags & IORESOURCE_MEM)
            cmd |= PCI_COMMAND_MEMORY;
    }
    if (dev->resource[PCI_ROM_RESOURCE].start)
        cmd |= PCI_COMMAND_MEMORY;
    if (cmd != old_cmd) {
        printk("PCI: Enabling device %s (%04x -> %04x)\n", dev->slot_name, old_cmd, cmd);
        pci_write_config_word(dev, PCI_COMMAND, cmd);
    }
    return 0;
}

int pcibios_enable_device(struct pci_dev *dev)
{
    /* External PCI device enable */
    if (dev->bus->number == 0)
        return pcibios_enable_resources(dev);

    return 0;
}

void pcibios_update_resource(struct pci_dev *dev, struct resource *root,
			     struct resource *res, int resource)
{
    unsigned long where, size;
    u32 reg;
    /* External PCI only */
    if (dev->bus->number != 0)
        return;

    where = PCI_BASE_ADDRESS_0 + (resource * 4);
    size = res->end - res->start;
    pci_read_config_dword(dev, where, &reg);
    reg = (reg & size) | (((u32)(res->start - root->start)) & ~size);
    pci_write_config_dword(dev, where, reg);
}

static void __init quirk_mpi_bridge(struct pci_dev *dev)
{
    return;
}	

struct pci_fixup pcibios_fixups[] = {
    { PCI_FIXUP_HEADER, PCI_ANY_ID, PCI_ANY_ID, quirk_mpi_bridge },
    { 0 }
};


#if defined(CONFIG_USB)
/* --------------------------------------------------------------------------
    Name: pci63xx_int_write
Abstract: PCI Config write on internal device(s)
 -------------------------------------------------------------------------- */
static int 
pci63xx_int_write(struct pci_dev * dev, int where, void * value, int len)
{
    if (PCI_SLOT(dev->devfn) != USB_HOST_SLOT) {
        return PCIBIOS_SUCCESSFUL;
    }

    switch (len) {
        case 1:
            DPRINT("W => Slot: %d Where: %2X Len: %d Data: %02X\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint8 *)value);
            break;
        case 2:
            DPRINT("W => Slot: %d Where: %2X Len: %d Data: %04X\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint16 *)value);
            break;
        case 4:
            DPRINT("W => Slot: %d Where: %2X Len: %d Data: %08lX\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint32 *)value);
            break;
        default:
            break;
    }

    return PCIBIOS_SUCCESSFUL;
}

/* --------------------------------------------------------------------------
    Name: pci63xx_int_read
Abstract: PCI Config read on internal device(s)
 -------------------------------------------------------------------------- */
static int 
pci63xx_int_read(struct pci_dev * dev, int where, void * value, int len)
{
    uint32 retValue = 0xFFFFFFFF;

    if (PCI_SLOT(dev->devfn) != USB_HOST_SLOT) {
        return PCIBIOS_SUCCESSFUL;
    }

    // For now, this is specific to the USB Host controller. We can
    // make it more general if we have to...
    // Emulate PCI Config accesses
    switch (where) {
        case PCI_VENDOR_ID:     
        case PCI_DEVICE_ID:
            retValue = PCI_VENDOR_ID_BROADCOM | 0x63000000;
            break;
        case PCI_COMMAND:
        case PCI_STATUS:
            retValue = (0x0006 << 16) | (0x0000);
            break;
        case PCI_CLASS_REVISION:
        case PCI_CLASS_DEVICE:
            retValue = (PCI_CLASS_SERIAL_USB << 16) | (0x10 << 8) | 0x01;
            break;
        case PCI_BASE_ADDRESS_0:
            retValue = USB_HOST_BASE;
            break;
        case PCI_CACHE_LINE_SIZE:
        case PCI_LATENCY_TIMER:
            retValue = 0;
            break;
        case PCI_HEADER_TYPE:
            retValue = PCI_HEADER_TYPE_NORMAL;
            break;
        case PCI_SUBSYSTEM_VENDOR_ID:
            retValue = PCI_VENDOR_ID_BROADCOM;
            break;
        case PCI_SUBSYSTEM_ID:
            retValue = 0x6300;
            break;
        case PCI_INTERRUPT_LINE:
            retValue = INTERRUPT_ID_USBH; 
            break;
        default:
            break;
    }

    switch (len) {
        case 1:
            *(uint8 *)value = (uint8) (retValue & 0xff);
            DPRINT("R <= Slot: %d Where: %2X Len: %d Data: %02X\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint8 *)value);
            break;
        case 2:
            *(uint16 *)value = (uint16) (retValue & 0xffff);
            DPRINT("R <= Slot: %d Where: %2X Len: %d Data: %04X\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint16 *)value);
            break;
        case 4:
            *(uint32 *)value = (uint32) retValue;
            DPRINT("R <= Slot: %d Where: %2X Len: %d Data: %08lX\n", 
                PCI_SLOT(dev->devfn), where, len, *(uint32 *)value);
            break;
        default:
            break;
    }

    return PCIBIOS_SUCCESSFUL;
}
#endif

/*
 * mpi_ResetPcCard: Set/Reset the PcCard
 */
static void mpi_ResetPcCard(int cardtype, BOOL bReset)
{

    if (cardtype == MPI_CARDTYPE_NONE) {
        return;
    }

    if (cardtype == MPI_CARDTYPE_CARDBUS) {
        bReset = ! bReset;
    }

    if (bReset) {
        mpi->pcmcia_cntl1 = (mpi->pcmcia_cntl1 & ~PCCARD_CARD_RESET);
    } else {
        mpi->pcmcia_cntl1 = (mpi->pcmcia_cntl1 | PCCARD_CARD_RESET);
    }
}

/*
 * mpi_ConfigCs: Configure an MPI/EBI chip select
 */
static void mpi_ConfigCs(uint32 cs, uint32 base, uint32 size, uint32 flags)
{
    mpi->cs[cs].base = ((base & 0x1FFFFFFF) | size);
    mpi->cs[cs].config = flags;
}

/*
 * mpi_InitPcmciaSpace
 */
static void mpi_InitPcmciaSpace(void)
{
    // ChipSelect 4 controls PCMCIA Memory accesses
    mpi_ConfigCs(PCMCIA_COMMON_BASE, pcmciaMem, EBI_SIZE_1M, (EBI_WORD_WIDE|EBI_ENABLE));
    // ChipSelect 5 controls PCMCIA Attribute accesses
    mpi_ConfigCs(PCMCIA_ATTRIBUTE_BASE, pcmciaAttr, EBI_SIZE_1M, (EBI_WORD_WIDE|EBI_ENABLE));
    // ChipSelect 6 controls PCMCIA I/O accesses
    mpi_ConfigCs(PCMCIA_IO_BASE, pcmciaIo, EBI_SIZE_64K, (EBI_WORD_WIDE|EBI_ENABLE));

    mpi->pcmcia_cntl2 = ((PCMCIA_ATTR_ACTIVE << RW_ACTIVE_CNT_BIT) | 
                         (PCMCIA_ATTR_INACTIVE << INACTIVE_CNT_BIT) | 
                         (PCMCIA_ATTR_CE_SETUP << CE_SETUP_CNT_BIT) | 
                         (PCMCIA_ATTR_CE_HOLD << CE_HOLD_CNT_BIT));

    mpi->pcmcia_cntl2 |= (PCMCIA_HALFWORD_EN | PCMCIA_BYTESWAP_DIS);
}

/*
 * cardtype_vcc_detect: PC Card's card detect and voltage sense connection
 * 
 *   CD1#/      CD2#/     VS1#/     VS2#/    Card       Initial Vcc
 *  CCD1#      CCD2#     CVS1      CVS2      Type
 *
 *   GND        GND       open      open     16-bit     5 vdc
 *
 *   GND        GND       GND       open     16-bit     3.3 vdc
 *
 *   GND        GND       open      GND      16-bit     x.x vdc
 *
 *   GND        GND       GND       GND      16-bit     3.3 & x.x vdc
 *
 *====================================================================
 *
 *   CVS1       GND       CCD1#     open     CardBus    3.3 vdc
 *
 *   GND        CVS2      open      CCD2#    CardBus    x.x vdc
 *
 *   GND        CVS1      CCD2#     open     CardBus    y.y vdc
 *
 *   GND        CVS2      GND       CCD2#    CardBus    3.3 & x.x vdc
 *
 *   CVS2       GND       open      CCD1#    CardBus    x.x & y.y vdc
 *
 *   GND        CVS1      CCD2#     open     CardBus    3.3, x.x & y.y vdc
 *
 */
static int cardtype_vcc_detect(void)
{
    uint32 data32;
    int cardtype;

    cardtype = MPI_CARDTYPE_NONE;
    mpi->pcmcia_cntl1 = 0x0000A000; // Turn on the output enables and drive
                                        // the CVS pins to 0.
    data32 = mpi->pcmcia_cntl1;
    switch (data32 & 0x00000003)  // Test CD1# and CD2#, see if card is plugged in.
    {
    case 0x00000003:  // No Card is in the slot.
        printk("mpi: No Card is in the PCMCIA slot\n");
        break;

    case 0x00000002:  // Partial insertion, No CD2#.
        printk("mpi: Card in the PCMCIA slot partial insertion, no CD2 signal\n");
        break;

    case 0x00000001:  // Partial insertion, No CD1#.
        printk("mpi: Card in the PCMCIA slot partial insertion, no CD1 signal\n");
        break;

    case 0x00000000:
        mpi->pcmcia_cntl1 = 0x0000A0C0; // Turn off the CVS output enables and
                                        // float the CVS pins.
        mdelay(1);
        data32 = mpi->pcmcia_cntl1;
        // Read the Register.
        switch (data32 & 0x0000000C)  // See what is on the CVS pins.
        {
        case 0x00000000: // CVS1 and CVS2 are tied to ground, only 1 option.
            printk("mpi: Detected 3.3 & x.x 16-bit PCMCIA card\n");
            cardtype = MPI_CARDTYPE_PCMCIA;
            break;
          
        case 0x00000004: // CVS1 is open or tied to CCD1/CCD2 and CVS2 is tied to ground.
                         // 2 valid voltage options.
            switch (data32 & 0x00000003)  // Test the values of CCD1 and CCD2.
            {
            case 0x00000003:  // CCD1 and CCD2 are tied to 1 of the CVS pins.
                              // This is not a valid combination.
                printk("mpi: Unknown card plugged into slot\n"); 
                break;
      
            case 0x00000002:  // CCD2 is tied to either CVS1 or CVS2. 
                mpi->pcmcia_cntl1 = 0x0000A080; // Drive CVS1 to a 0.
                mdelay(1);
                data32 = mpi->pcmcia_cntl1;
                if (data32 & 0x00000002) { // CCD2 is tied to CVS2, not valid.
                    printk("mpi: Unknown card plugged into slot\n"); 
                } else {                   // CCD2 is tied to CVS1.
                    printk("mpi: Detected 3.3, x.x and y.y Cardbus card\n");
                    cardtype = MPI_CARDTYPE_CARDBUS;
                }
                break;
                
            case 0x00000001: // CCD1 is tied to either CVS1 or CVS2.
                             // This is not a valid combination.
                printk("mpi: Unknown card plugged into slot\n"); 
                break;
                
            case 0x00000000:  // CCD1 and CCD2 are tied to ground.
                printk("mpi: Detected x.x vdc 16-bit PCMCIA card\n");
                cardtype = MPI_CARDTYPE_PCMCIA;
                break;
            }
            break;
          
        case 0x00000008: // CVS2 is open or tied to CCD1/CCD2 and CVS1 is tied to ground.
                         // 2 valid voltage options.
            switch (data32 & 0x00000003)  // Test the values of CCD1 and CCD2.
            {
            case 0x00000003:  // CCD1 and CCD2 are tied to 1 of the CVS pins.
                              // This is not a valid combination.
                printk("mpi: Unknown card plugged into slot\n"); 
                break;
      
            case 0x00000002:  // CCD2 is tied to either CVS1 or CVS2.
                mpi->pcmcia_cntl1 = 0x0000A040; // Drive CVS2 to a 0.
                mdelay(1);
                data32 = mpi->pcmcia_cntl1;
                if (data32 & 0x00000002) { // CCD2 is tied to CVS1, not valid.
                    printk("mpi: Unknown card plugged into slot\n"); 
                } else {// CCD2 is tied to CVS2.
                    printk("mpi: Detected 3.3 and x.x Cardbus card\n");
                    cardtype = MPI_CARDTYPE_CARDBUS;
                }
                break;

            case 0x00000001: // CCD1 is tied to either CVS1 or CVS2.
                             // This is not a valid combination.
                printk("mpi: Unknown card plugged into slot\n"); 
                break;

            case 0x00000000:  // CCD1 and CCD2 are tied to ground.
                cardtype = MPI_CARDTYPE_PCMCIA;
                printk("mpi: Detected 3.3 vdc 16-bit PCMCIA card\n");
                break;
            }
            break;
          
        case 0x0000000C:  // CVS1 and CVS2 are open or tied to CCD1/CCD2.
                          // 5 valid voltage options.
      
            switch (data32 & 0x00000003)  // Test the values of CCD1 and CCD2.
            {
            case 0x00000003:  // CCD1 and CCD2 are tied to 1 of the CVS pins.
                              // This is not a valid combination.
                printk("mpi: Unknown card plugged into slot\n"); 
                break;
      
            case 0x00000002:  // CCD2 is tied to either CVS1 or CVS2.
                              // CCD1 is tied to ground.
                mpi->pcmcia_cntl1 = 0x0000A040; // Drive CVS2 to a 0.
                mdelay(1);
                data32 = mpi->pcmcia_cntl1;
                if (data32 & 0x00000002) {  // CCD2 is tied to CVS1.
                    printk("mpi: Detected y.y vdc Cardbus card\n");
                } else {                    // CCD2 is tied to CVS2.
                    printk("mpi: Detected x.x vdc Cardbus card\n");
                }
                cardtype = MPI_CARDTYPE_CARDBUS;
                break;
      
            case 0x00000001: // CCD1 is tied to either CVS1 or CVS2.
                             // CCD2 is tied to ground.
      
                mpi->pcmcia_cntl1 = 0x0000A040; // Drive CVS2 to a 0.
                mdelay(1);
                data32 = mpi->pcmcia_cntl1;
                if (data32 & 0x00000001) {// CCD1 is tied to CVS1.
                    printk("mpi: Detected 3.3 vdc Cardbus card\n");
                } else {                    // CCD1 is tied to CVS2.
                    printk("mpi: Detected x.x and y.y Cardbus card\n");
                }
                cardtype = MPI_CARDTYPE_CARDBUS;
                break;
      
            case 0x00000000:  // CCD1 and CCD2 are tied to ground.
                cardtype = MPI_CARDTYPE_PCMCIA;
                printk("mpi: Detected 5 vdc 16-bit PCMCIA card\n");
                break;
            }
            break;
      
        default:
            printk("mpi: Unknown card plugged into slot\n"); 
            break;
        
        }
    }
    return cardtype;
}

/*
 * mpi_DetectPcCard: Detect the plugged in PC-Card
 * Return: < 0 => Unknown card detected
 *         0 => No card detected
 *         1 => 16-bit card detected
 *         2 => 32-bit CardBus card detected
 */
static int mpi_DetectPcCard(void)
{
    int cardtype;

    cardtype = cardtype_vcc_detect();
    switch(cardtype) {
        case MPI_CARDTYPE_PCMCIA:
            mpi->pcmcia_cntl1 &= ~0x0000e000; // disable enable bits
            //mpi->pcmcia_cntl1 = (mpi->pcmcia_cntl1 & ~PCCARD_CARD_RESET);
            mpi->pcmcia_cntl1 |= (PCMCIA_ENABLE | PCMCIA_GPIO_ENABLE);
            mpi_InitPcmciaSpace();
            mpi_ResetPcCard(cardtype, FALSE);
            // Hold card in reset for 10ms
            mdelay(10);
            mpi_ResetPcCard(cardtype, TRUE);
            // Let card come out of reset
            mdelay(100);
            break;
        case MPI_CARDTYPE_CARDBUS:
            // 8 => CardBus Enable
            // 1 => PCI Slot Number
            // C => Float VS1 & VS2
            mpi->pcmcia_cntl1 = (mpi->pcmcia_cntl1 & 0xFFFF0000) | 
                                CARDBUS_ENABLE | 
                                (CARDBUS_SLOT << 8)| 
                                VS2_OEN |
                                VS1_OEN;
            /* access to this memory window will be to/from CardBus */
            mpi->l2pmremap1 |= CARDBUS_MEM;

            // Need to reset the Cardbus Card. There's no CardManager to do this, 
            // and we need to be ready for PCI configuration. 
            mpi_ResetPcCard(cardtype, FALSE);
            // Hold card in reset for 10ms
            mdelay(10);
            mpi_ResetPcCard(cardtype, TRUE);
            // Let card come out of reset
            mdelay(100);
            break;
        default:
            break;
    }
    return cardtype;
}
