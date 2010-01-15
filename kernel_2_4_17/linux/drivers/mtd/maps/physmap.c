/*
 * $Id: physmap.c,v 1.3 2004/02/26 15:17:40 xde Exp $
 *
 * Normal mappings of chips in physical memory
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>

#if defined(CONFIG_MIPS_BRCM)
#include <linux/mtd/partitions.h>
#include <asm/bcm963xx/board.h>
#include <asm/bcm963xx/bcmTag.h>
#endif
// #if defined(CONFIG_BCM96348)
// #include <linux/mtd/partitions.h>
// #include <asm/bcm96348/board.h>
// #include <asm/bcm96348/bcmTag.h>
// #endif

#define WINDOW_ADDR CONFIG_MTD_PHYSMAP_START
#define WINDOW_SIZE CONFIG_MTD_PHYSMAP_LEN
#define BUSWIDTH CONFIG_MTD_PHYSMAP_BUSWIDTH

/* inv_xde */
extern int parse_redboot_partitions(struct mtd_info *master, struct mtd_partition **pparts);
static struct mtd_partition *parsed_parts;

static struct mtd_info *mymtd;

__u8 physmap_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 physmap_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 physmap_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

void physmap_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

void physmap_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
}

void physmap_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
}

void physmap_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
}

void physmap_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	memcpy_toio(map->map_priv_1 + to, from, len);
}

struct map_info physmap_map = {
	name: "Physically mapped flash",
	size: WINDOW_SIZE,
	buswidth: BUSWIDTH,
	read8: physmap_read8,
	read16: physmap_read16,
	read32: physmap_read32,
	copy_from: physmap_copy_from,
	write8: physmap_write8,
	write16: physmap_write16,
	write32: physmap_write32,
	copy_to: physmap_copy_to
};


#if defined(CONFIG_MIPS_BRCM)
static struct mtd_partition bcm963xx_parts[] = {
        { name: "bootloader",	size: 0,	offset: 0,	mask_flags: MTD_WRITEABLE },
        { name: "rootfs",		size: 0,	offset: 0},
	{ name: "jffs2",	size: 5 * 0x10000,	offset: 57*0x10000}
};

static int bcm963xx_parts_size = sizeof(bcm963xx_parts) / sizeof(bcm963xx_parts[0]);

/* Initialize MTD partitions by reading partition sizes from the flash
 * memory map (FILE_TAG structure) which stored in the beginning of the kernel section of
 * the flash and flash address information (FLASH_ADDR_INFO structure)
 * from the flash driver.
 */
static int __init init_parts(void)
{
    int status = 0;
    PFILE_TAG pTag = NULL;
    u_int32_t rootfs_addr, kernel_addr;
    FLASH_ADDR_INFO info;

    /* inv_xde */
    if (boot_loader_type == BOOT_CFE) {
      kerSysFlashAddrInfoGet( &info );
      
      rootfs_addr = kernel_addr = 0;
    /* Read the flash memory map from flash memory. */
      if (!(pTag = kerSysImageTagGet()))
	{
	  printk("Failed to read image tag from flash\n");
	  status = -1;
	  while (1) ; 
	}
      else
	{
	  rootfs_addr = (u_int32_t) simple_strtoul(pTag->rootfsAddress, NULL, 10);
	  kernel_addr = (u_int32_t) simple_strtoul(pTag->kernelAddress, NULL, 10);
	  bcm963xx_parts[0].size = rootfs_addr - (u_int32_t) FLASH_BASE;
	  bcm963xx_parts[0].offset = 0;
	  
	  bcm963xx_parts[1].size = kernel_addr - rootfs_addr;
	  bcm963xx_parts[1].offset = bcm963xx_parts[0].offset + bcm963xx_parts[0].size; 
	  
#if defined(CONFIG_ROOT_NFS)
	  bcm963xx_parts[0].mask_flags = 0;
	  bcm963xx_parts[2].mask_flags = 0;
#endif
	}
    }

    return( status );
}
#endif

int __init init_physmap(void)
{
    /* WINDOW_SIZE should be mymtd->size after mtd is initialized -- and this line is moved to there */

#if !defined(CONFIG_MIPS_BRCM)  
    printk(KERN_NOTICE "physmap flash device: %x at %x\n", WINDOW_SIZE, WINDOW_ADDR);
#endif
#if defined(CONFIG_MIPS_BRCM)
	if( init_parts() != 0 )
		return -EIO;

	/* In BCM96345 board with 2M flash, we use the physical address directly. 
	 * The mapped window starts from offset 3*64k=0x30000, which is used by the bootloader.
	 * ends before (NVRAM-8k and PSI-16k)
	 * Song Wang (songw@broadcom.com)
	 */
	physmap_map.map_priv_1 = (unsigned long)(WINDOW_ADDR);
#else
	physmap_map.map_priv_1 = (unsigned long)ioremap(WINDOW_ADDR, WINDOW_SIZE);
#endif

	/* inv_xde */
	if (boot_loader_type == BOOT_CFE) {
	  if (!physmap_map.map_priv_1) {
	    printk("Failed to ioremap\n");
	    return -EIO;
	  }
	  mymtd = do_map_probe("cfi_probe", &physmap_map);
	  if (!mymtd)
	    mymtd = do_map_probe("jedec_probe", &physmap_map);
	  if (mymtd) {
	    mymtd->module = THIS_MODULE;
	    
	    add_mtd_device(mymtd);
#if defined(CONFIG_MIPS_BRCM)
	    printk(KERN_NOTICE "physmap flash device: 0x%x at 0x%x\n", mymtd->size, WINDOW_ADDR);
	    add_mtd_partitions(mymtd, bcm963xx_parts, bcm963xx_parts_size);
#endif
	    return 0;
	  }
	  
	}
	else {
	  if (!physmap_map.map_priv_1) {
	    printk("Failed to ioremap\n");
	    return -EIO;
	  }
	  
	  /*
	   * Now let's probe for the actual flash.  Do it here since
	   * specific machine settings might have been set above.
	   */
	  printk(KERN_NOTICE "DBW flash: probing %d-bit flash bus\n", physmap_map.buswidth*8);
	  
	  mymtd = do_map_probe("amd_flash", &physmap_map);
	  if (!mymtd)
	    mymtd = do_map_probe("jedec_probe", &physmap_map);
	  if (!mymtd)
	    mymtd = do_map_probe("cfi_probe", &physmap_map);
	  
	  printk(KERN_NOTICE " mymtd is : %p\n",mymtd);
	  
	  if (!mymtd)
	    return -ENXIO;
	  mymtd->module = THIS_MODULE;
	  
	  /* inv_xde */
	  if (mymtd->size > 0x00400000) {
	    printk("Support for extended flash memory size : 0x%08X ; ONLY 64MBIT SUPPORT\n", mymtd->size);
	    physmap_map.map_priv_1 = (unsigned long)(0xBE400000);
	  }
	  
	  struct mtd_partition *parts;
	  int nb_parts = 0;
	  int parsed_nr_parts = 0;
	  char *part_type;
	  
#ifdef CONFIG_MTD_REDBOOT_PARTS
	  if (parsed_nr_parts == 0) {
	    int ret = parse_redboot_partitions(mymtd, &parsed_parts);
	    //printk("After parse_redboot_partitions\n");
	    if (ret > 0) {
	      part_type = "RedBoot";
	      parsed_nr_parts = ret;
	    }
	  }
#endif	  
	  if (parsed_nr_parts > 0) {
	    parts = parsed_parts;
	    nb_parts = parsed_nr_parts;
	  }
	  
	  if (nb_parts == 0) {
	    printk(KERN_NOTICE "blue_3g flash: no partition info available, registering whole flash at once\n");
	    add_mtd_device(mymtd);
	  } else {
	    printk(KERN_NOTICE "Using %s partition definition\n", part_type);
	    add_mtd_partitions(mymtd, parts, nb_parts);
	  }
	}

#if !defined(CONFIG_MIPS_BRCM)
	iounmap((void *)physmap_map.map_priv_1);
#endif
	
	
	return -ENXIO;
}

static void __exit cleanup_physmap(void)
{
	if (mymtd) {
		del_mtd_device(mymtd);
		map_destroy(mymtd);
	}
	if (physmap_map.map_priv_1) {
		iounmap((void *)physmap_map.map_priv_1);
		physmap_map.map_priv_1 = 0;
	}
}

module_init(init_physmap);
module_exit(cleanup_physmap);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("David Woodhouse <dwmw2@infradead.org>");
MODULE_DESCRIPTION("Generic configurable MTD map driver");
