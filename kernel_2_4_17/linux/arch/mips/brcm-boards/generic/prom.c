/*
<:copyright-gpl 
 Copyright 2003 Broadcom Corp. All Rights Reserved. 
 
 This program is free software; you can distribute it and/or modify it 
 under the terms of the GNU General Public License (Version 2) as 
 published by the Free Software Foundation. 
 
 This program is distributed in the hope it will be useful, but WITHOUT 
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 for more details. 
 
 You should have received a copy of the GNU General Public License along 
 with this program; if not, write to the Free Software Foundation, Inc., 
 59 Temple Place - Suite 330, Boston MA 02111-1307, USA. 
:>
*/
/*
 * prom.c: PROM library initialization code.
 *
 * 02/20/2002	Song Wang	Changes for 2.4.17 kernel
 *				Add get_system_model function
 *				due to changes in /proc/cpuinfo code in proc.c
 *				Also change COMMAND_LINE_SIZE to be CL_SIZE
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/bootmem.h>
#include <linux/blk.h>
#include <asm/addrspace.h>
#include <asm/bootinfo.h>
#include <asm/cpu.h>
#include <linux/module.h>

#if defined(CONFIG_BCM96345)
#include <6345_map_part.h>
#endif

#if defined(CONFIG_BCM96348)
#include <6348_map_part.h>
#endif

#if defined(CONFIG_BCM96338)
#include <6338_map_part.h>
#endif

#include <board.h>
#include "boardparms.h"

char arcs_cmdline[CL_SIZE] __initdata = {0};
/* inv_xde */
int boot_loader_type;

extern int  do_syslog(int, char *, int);
extern void serial_init(void);
extern void __init InitNvramInfo( void );
extern void kerSysFlashInit( void );
extern unsigned long get_nvram_start_addr(void);
void __init create_root_nfs_cmdline( char *cmdline );

#define PRINTK(x)       printk(x)

#if defined(CONFIG_BCM96345)
#define CPU_CLOCK                   140000000
#define MACH_BCM                    MACH_BCM96345
#define CONFIG_BCM_ROOT_NFS_DIR     CONFIG_BCM96345_ROOT_NFS_DIR
#endif

#if defined(CONFIG_BCM96338)
#define CPU_CLOCK                   240000000
#define MACH_BCM                    MACH_BCM96338
#define CONFIG_BCM_ROOT_NFS_DIR     CONFIG_BCM96338_ROOT_NFS_DIR
#endif

#if defined(CONFIG_BCM96348)
void __init calculateCpuSpeed(void);
static unsigned long cpu_speed;
#define CPU_CLOCK                   cpu_speed
#define MACH_BCM                    MACH_BCM96348
#define CONFIG_BCM_ROOT_NFS_DIR     CONFIG_BCM96348_ROOT_NFS_DIR
#endif

const char *get_system_type(void)
{
    PNVRAM_DATA pNvramData = (PNVRAM_DATA) get_nvram_start_addr();

    return( pNvramData->szBoardId );
}

unsigned long getMemorySize(void)
{
    unsigned long ulSdramType = BOARD_SDRAM_TYPE;

    unsigned long ulSdramSize;

    switch( ulSdramType )


    {
    case BP_MEMORY_16MB_1_CHIP:
    case BP_MEMORY_16MB_2_CHIP:
        ulSdramSize = 16 * 1024 * 1024;
        break;
    case BP_MEMORY_32MB_1_CHIP:
    case BP_MEMORY_32MB_2_CHIP:
        ulSdramSize = 32 * 1024 * 1024;
        break;
    case BP_MEMORY_64MB_2_CHIP:
        ulSdramSize = 64 * 1024 * 1024;
        break;
    default:
        ulSdramSize = 8 * 1024 * 1024;
        break;
    }

    /* inv_xde */
    //    return ulSdramSize;
    if (boot_loader_type == BOOT_CFE)
      return ulSdramSize;
    else
      // assume that there is one contiguous memory map      
      return boot_mem_map.map[0].size;

}

/* --------------------------------------------------------------------------
    Name: prom_init
Abstract: 
 -------------------------------------------------------------------------- */
void __init 
prom_init(int argc, const char **argv)
{
    extern ulong r4k_interval;

    serial_init();

    /* inv_xde */
    if ((argv > 0x80000000) && (argv < 0x82000000)) {
      strncpy(arcs_cmdline, argv[1], CL_SIZE);
      //printk("cmd_line : %s\n", argv[1]);
      printk("arcs_cmdline: %s\n", &(arcs_cmdline[0]));
    }

    if (strncmp(arcs_cmdline, "boot_loader=RedBoot", 19) != 0) {
      printk("Boot loader : CFE\n");
      boot_loader_type =  BOOT_CFE;
    }
    else {
      printk("Boot loader : REDBOOT\n");
      boot_loader_type = BOOT_REDBOOT;
    }

    if (boot_loader_type == BOOT_CFE)
      kerSysFlashInit();

    do_syslog(8, NULL, 8);

    printk( "%s prom init\n", get_system_type() );

    mips_cpu.options |= MIPS_CPU_4KTLB;
    mips_cpu.options |= MIPS_CPU_4KEX;

    PERF->IrqMask = 0;

    /* inv_xde */
    if (boot_loader_type == BOOT_CFE) {
#if defined(CONFIG_ROOT_NFS)
    create_root_nfs_cmdline( arcs_cmdline );
#elif defined(CONFIG_ROOT_FLASHFS)
    strcpy(arcs_cmdline, CONFIG_ROOT_FLASHFS);
#endif
    }
#define ADSL_SDRAM_IMAGE_SIZE 0x40000
    /* inv_xde */
    //add_memory_region(0, (getMemorySize() - ADSL_SDRAM_IMAGE_SIZE), BOOT_MEM_RAM);
    if (boot_loader_type == BOOT_CFE)
      add_memory_region(0, (getMemorySize() - ADSL_SDRAM_IMAGE_SIZE), BOOT_MEM_RAM);
    else
      // inv_xde : default SDRAM size is 16MBit : it is overwritten by kernel
      // command line if mem=xxM is noticed
      add_memory_region(0, (0x01000000 - ADSL_SDRAM_IMAGE_SIZE), BOOT_MEM_RAM);

#if defined(CONFIG_BCM96348)
    calculateCpuSpeed();
#endif
    /* Count register increments every other clock */
    r4k_interval = CPU_CLOCK / HZ / 2;

    mips_machgroup = MACH_GROUP_BRCM;
    mips_machtype = MACH_BCM;

    #ifdef CONFIG_BLK_DEV_INITRD
/*
    {
        #define ADDRESS		0x00000000

	    u32 * initrd_header = (u32 *)ADDRESS;
	    if (initrd_header[0] == 0x494E5244) {
		    initrd_start = (u32) &initrd_header[2];
		    initrd_end = initrd_start + initrd_header[1];
		    printk("Initrd detected @ %08X size %08X\n", 
		                                (u32)initrd_header, initrd_header[1]);
	    } else {
		    initrd_start = initrd_end = 0;
		    printk("NO initrd detected @ %08X: %08X %08X\n", 
						    (u32)initrd_header, initrd_header[0], initrd_header[1]);
	    }
    }
*/
    #endif
}

/* --------------------------------------------------------------------------
    Name: prom_free_prom_memory
Abstract: 
 -------------------------------------------------------------------------- */
void __init 
prom_free_prom_memory(void)
{

}


#if defined(CONFIG_ROOT_NFS)
/* This function reads in a line that looks something like this:
 *
 *
 * CFE bootline=bcmEnet(0,0)host:vmlinux e=192.169.0.100:ffffff00 h=192.169.0.1
 *
 *
 * and retuns in the cmdline parameter some that looks like this:
 *
 * CONFIG_CMDLINE="root=/dev/nfs nfsroot=192.168.0.1:/opt/targets/96345R/fs
 * ip=192.168.0.100:192.168.0.1::255.255.255.0::eth0:off rw"
 */
#define BOOT_LINE_ADDR   0x0
#define HEXDIGIT(d) ((d >= '0' && d <= '9') ? (d - '0') : ((d | 0x20) - 'W'))
#define HEXBYTE(b)  (HEXDIGIT((b)[0]) << 4) + HEXDIGIT((b)[1])
extern unsigned long get_nvram_start_addr(void);

void __init create_root_nfs_cmdline( char *cmdline )
{
    char root_nfs_cl[] = "root=/dev/nfs nfsroot=%s:" CONFIG_BCM_ROOT_NFS_DIR
        " ip=%s:%s::%s::eth0:off rw";

    char *localip = NULL;
    char *hostip = NULL;
    char mask[16] = "";
    PNVRAM_DATA pNvramData = (PNVRAM_DATA) get_nvram_start_addr();
    char bootline[128] = "";
    char *p = bootline;

    memcpy(bootline, pNvramData->szBootline, sizeof(bootline));
    while( *p )
    {
        if( p[0] == 'e' && p[1] == '=' )
        {
            /* Found local ip address */
            p += 2;
            localip = p;
            while( *p && *p != ' ' && *p != ':' )
                p++;
            if( *p == ':' )
            {
                /* Found network mask (eg FFFFFF00 */
                *p++ = '\0';
                sprintf( mask, "%u.%u.%u.%u", HEXBYTE(p), HEXBYTE(p + 2),
                HEXBYTE(p + 4), HEXBYTE(p + 6) );
                p += 4;
            }
            else if( *p == ' ' )
                *p++ = '\0';
        }
        else if( p[0] == 'h' && p[1] == '=' )
        {
            /* Found host ip address */
            p += 2;
            hostip = p;
            while( *p && *p != ' ' )
                p++;
            if( *p == ' ' )
                    *p++ = '\0';
        }
        else 
            p++;
    }

    if( localip && hostip ) 
        sprintf( cmdline, root_nfs_cl, hostip, localip, hostip, mask );
}
#endif

#if defined(CONFIG_BCM96348)
/*  *********************************************************************
    *  calculateCpuSpeed()
    *      Calculate the BCM6348 CPU speed by reading the PLL strap register
    *      and applying the following formula:
    *      cpu_clk = (.25 * 64MHz freq) * (N1 + 1) * (N2 + 2) / (M1_CPU + 1)
    *  Input parameters:
    *      none
    *  Return value:
    *      none
    ********************************************************************* */
void __init calculateCpuSpeed(void)
{
    UINT32 pllStrap = PERF->PllStrap;
    int n1 = (pllStrap & PLL_N1_MASK) >> PLL_N1_SHFT;
    int n2 = (pllStrap & PLL_N2_MASK) >> PLL_N2_SHFT;
    int m1cpu = (pllStrap & PLL_M1_CPU_MASK) >> PLL_M1_CPU_SHFT;

    cpu_speed = (16 * (n1 + 1) * (n2 + 2) / (m1cpu + 1)) * 1000000;
}
#endif


EXPORT_SYMBOL(boot_loader_type);
