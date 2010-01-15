/*
 * A simple flash mapping code for BCM963xx board flash memory
 * It is simple because it only treats all the flash memory as ROM
 * It is used with chips/map_rom.c
 *
 *  Song Wang (songw@broadcom.com)
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <asm/io.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/config.h>

#if defined(CONFIG_MIPS_BRCM)
#include <asm/bcm963xx/board.h>
#include <asm/bcm963xx/bcmTag.h>
#endif

static struct mtd_info *mymtd;

__u8 brcm_physmap_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 brcm_physmap_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 brcm_physmap_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

void brcm_physmap_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	memcpy_fromio(to, map->map_priv_1 + from, len);
}

struct map_info brcm_physmap_map = {
	name: "Physically mapped flash",
	buswidth: 2,
	read8: brcm_physmap_read8,
	read16: brcm_physmap_read16,
	read32: brcm_physmap_read32,
	copy_from: brcm_physmap_copy_from
};

int __init init_brcm_physmap(void)
{
        PFILE_TAG pTag = NULL;
        u_int32_t rootfs_addr, kernel_addr;
        FLASH_ADDR_INFO info;

        kerSysFlashAddrInfoGet( &info );

        /* Read the flash memory map from flash memory. */
        if (!(pTag = kerSysImageTagGet())) {
                printk("Failed to read image tag from flash\n");
                return -EIO;
        }

        rootfs_addr = (u_int32_t) simple_strtoul(pTag->rootfsAddress, NULL, 10);
        kernel_addr = (u_int32_t) simple_strtoul(pTag->kernelAddress, NULL, 10);
	
	brcm_physmap_map.size = kernel_addr - rootfs_addr;
	brcm_physmap_map.map_priv_1 = (unsigned long)rootfs_addr;

	if (!brcm_physmap_map.map_priv_1) {
		printk("Wrong rootfs starting address\n");
		return -EIO;
	}
	
	if (brcm_physmap_map.size <= 0) {
		printk("Wrong rootfs size\n");
		return -EIO;
	}	
	
	mymtd = do_map_probe("map_rom", &brcm_physmap_map);
	if (mymtd) {
		mymtd->module = THIS_MODULE;
		add_mtd_device(mymtd);

		return 0;
	}

	return -ENXIO;
}

static void __exit cleanup_brcm_physmap(void)
{
	if (mymtd) {
		del_mtd_device(mymtd);
		map_destroy(mymtd);
	}
	if (brcm_physmap_map.map_priv_1) {
		brcm_physmap_map.map_priv_1 = 0;
	}
}

module_init(init_brcm_physmap);
module_exit(cleanup_brcm_physmap);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Song Wang songw@broadcom.com");
MODULE_DESCRIPTION("Configurable MTD map driver for read-only root file system");
