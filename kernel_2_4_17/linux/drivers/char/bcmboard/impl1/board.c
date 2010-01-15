/*
<:copyright-gpl 
 Copyright 2002 Broadcom Corp. All Rights Reserved. 
 
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
/***************************************************************************
 * File Name  : board.c
 *
 * Description: This file contains Linux character device driver entry 
 *              for the board related ioctl calls: flash, get free kernel
 *              page and dump kernel memory, etc.
 *
 * Created on :  2/20/2002  seanl:  use cfiflash.c, cfliflash.h (AMD specific)
 *
 ***************************************************************************/


/* Includes. */
#include <linux/fs.h>
#include <linux/iobuf.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <asm/uaccess.h>

#if defined(CONFIG_BCM96348)
#include <6348_map_part.h>
#endif
#if defined(CONFIG_BCM96345)
#include <6345_map_part.h>
#endif
#if defined(CONFIG_BCM96338)
#include <6338_map_part.h>
#endif

#include <board.h>
#include <bcmTag.h>
#include "boardparms.h"
#include "cfiflash.h"

/* Typedefs. */
// used to be the last octet. Now changed to the first 5 bits of the the forth octet
// to reduced the duplicated MAC addresses.
#define CHANGED_OCTET   3
#define SHIFT_BITS      3

typedef struct
{
    unsigned long ulId;
    char chInUse;
    char chReserved[3];
} MAC_ADDR_INFO, *PMAC_ADDR_INFO;

typedef struct
{
    unsigned long ulSdramSize;
    unsigned long ulPsiSize;
    unsigned long ulNumMacAddrs;
    unsigned long ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN];
    MAC_ADDR_INFO MacAddrs[1];
} NVRAM_INFO, *PNVRAM_INFO;


static LED_MAP_PAIR LedMapping[] =
{   // led name     Initial state       physical pin (ledMask)
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0},
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0},
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0},
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0},
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0},
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0}, 
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0}, 
    {kLedEnd,       kLedStateOff,       0, 0, 0, 0} // NOTE: kLedEnd has to be at the end.
};

/* Externs. */
extern unsigned int nr_free_pages (void);
extern const char *get_system_type(void);
extern void kerSysFlashInit(void);
extern unsigned long get_nvram_start_addr(void);
extern unsigned long get_scratch_pad_start_addr(void);
extern unsigned long getMemorySize(void);
extern void __init boardLedInit(PLED_MAP_PAIR);
extern void boardLedCtrl(BOARD_LED_NAME, BOARD_LED_STATE);
extern void kerSysLedRegisterHandler( BOARD_LED_NAME ledName,
    HANDLE_LED_FUNC ledHwFunc, int ledFailType );

/* Prototypes. */
void __init InitNvramInfo( void );
static int board_open( struct inode *inode, struct file *filp );
static int board_ioctl( struct inode *inode, struct file *flip,
                        unsigned int command, unsigned long arg );

static PNVRAM_INFO g_pNvramInfo = NULL;
static int g_ledInitialized = 0;

static struct file_operations board_fops =
{
  open:       board_open,
  ioctl:      board_ioctl,
};

uint32 board_major = 0;


#if defined(MODULE)
int init_module(void)
{
    return( brcm_board_init() );              
}

void cleanup_module(void)
{
    if (MOD_IN_USE)
        printk("brcm flash: cleanup_module failed because module is in use\n");
    else
        brcm_board_cleanup();
}
#endif //MODULE 



static int __init brcm_board_init( void )
{
#if 0
    typedef int (*BP_LED_FUNC) (unsigned short *);

    static struct BpLedInformation
    {
        BOARD_LED_NAME ledName;
        BP_LED_FUNC bpFunc;
        BP_LED_FUNC bpFuncFail;
    } bpLedInfo[] =
    {{kLedAdsl, BpGetAdslLedGpio, BpGetAdslFailLedGpio},
     {kLedWireless, BpGetWirelessLedGpio, NULL},
     {kLedUsb, BpGetUsbLedGpio, NULL},
     {kLedHpna, BpGetHpnaLedGpio, NULL},
     {kLedWanData, BpGetWanDataLedGpio, NULL},
     {kLedPPP, BpGetPppLedGpio, BpGetPppFailLedGpio},
     {kLedVoip, BpGetVoipLedGpio, NULL},
     {kLedEnd, NULL, NULL}
    };
#endif 
    int ret;
        
    ret = register_chrdev(BOARD_DRV_MAJOR, "bcrmboard", &board_fops );
    if (ret < 0)
        printk( "brcm_board_init(major %d): fail to register device.\n",BOARD_DRV_MAJOR);
    else 
    {
        PLED_MAP_PAIR pLedMap = LedMapping;
        unsigned short gpio;
//        struct BpLedInformation *pInfo;

        printk("brcmboard: brcm_board_init entry\n");
        board_major = BOARD_DRV_MAJOR;

	/* inv_xde */
	if (boot_loader_type == BOOT_CFE)
	  InitNvramInfo();


	/* CMO -- remplir le mapping des leds pLedMap */
#if !defined(CONFIG_BCM96338)
	GPIO->GPIOMode &= ~(0x000F0000);
#endif

	pLedMap[0].ledName = kLedSelfTest ; 
	pLedMap[0].ledMask = GPIO_NUM_TO_MASK(0);
	pLedMap[0].ledActiveLow = 0;

	pLedMap[1].ledName = kLedAdsl;
	pLedMap[1].ledMask = GPIO_NUM_TO_MASK(1);
	pLedMap[1].ledActiveLow = 0;

	pLedMap[2].ledName = kLedLan ; 
	pLedMap[2].ledMask = GPIO_NUM_TO_MASK(2);
	pLedMap[2].ledActiveLow = 0;

	pLedMap[3].ledName = kLedVoip  ; 
	pLedMap[3].ledMask = GPIO_NUM_TO_MASK(3);
	pLedMap[3].ledActiveLow = 0;

	pLedMap[4].ledName = kLedWireless ;
	pLedMap[4].ledMask = GPIO_NUM_TO_MASK(4);
	pLedMap[4].ledActiveLow = 0;

	pLedMap[5].ledName = kLedEnd;
#if 0 
        for( pInfo = bpLedInfo; pInfo->ledName != kLedEnd; pInfo++ )
        {
            if( pInfo->bpFunc && (*pInfo->bpFunc) (&gpio) == BP_SUCCESS )
            {
                pLedMap->ledName = pInfo->ledName;
                pLedMap->ledMask = GPIO_NUM_TO_MASK(gpio);
                pLedMap->ledActiveLow = (gpio & BP_ACTIVE_LOW) ? 1 : 0;
            }
            if( pInfo->bpFuncFail && (*pInfo->bpFuncFail) (&gpio) == BP_SUCCESS )
            {
                pLedMap->ledName = pInfo->ledName;
                pLedMap->ledMaskFail = GPIO_NUM_TO_MASK(gpio);
                pLedMap->ledActiveLowFail = (gpio & BP_ACTIVE_LOW) ? 1 : 0;
            }
            if( pLedMap->ledName != kLedEnd )
                pLedMap++;
        }
#endif 
        boardLedInit(LedMapping);
        g_ledInitialized = 1;
    }

    boardLedCtrl(kLedSelfTest, kLedStateFastBlinkContinues);

    return ret;
} 

void __init InitNvramInfo( void )
{
    PNVRAM_DATA pNvramData = (PNVRAM_DATA) get_nvram_start_addr();
    unsigned long ulNumMacAddrs = pNvramData->ulNumMacAddrs;

    if( ulNumMacAddrs > 0 && ulNumMacAddrs <= NVRAM_MAC_COUNT_MAX )
    {
        unsigned long ulNvramInfoSize =
            sizeof(NVRAM_INFO) + ((sizeof(MAC_ADDR_INFO) - 1) * ulNumMacAddrs);

        g_pNvramInfo = (PNVRAM_INFO) kmalloc( ulNvramInfoSize, GFP_KERNEL );

        if( g_pNvramInfo )
        {
            unsigned long ulPsiSize;
            if( BpGetPsiSize( &ulPsiSize ) != BP_SUCCESS )
                ulPsiSize = NVRAM_PSI_DEFAULT;
            memset( g_pNvramInfo, 0x00, ulNvramInfoSize );
            g_pNvramInfo->ulPsiSize = ulPsiSize * 1024;
            g_pNvramInfo->ulNumMacAddrs = pNvramData->ulNumMacAddrs;
            memcpy( g_pNvramInfo->ucaBaseMacAddr, pNvramData->ucaBaseMacAddr,
                NVRAM_MAC_ADDRESS_LEN );
            g_pNvramInfo->ulSdramSize = getMemorySize();
        }
        else
            printk("ERROR - Could not allocate memory for NVRAM data\n");
    }
    else
        printk("ERROR - Invalid number of MAC addresses (%ld) is configured.\n",
            ulNumMacAddrs);
}

void __exit brcm_board_cleanup( void )
{
    printk("brcm_board_cleanup()\n");

    if (board_major != -1) 
    {
        unregister_chrdev(board_major, "board_ioctl");
    }
} 

static int board_open( struct inode *inode, struct file *filp )
{
    return( 0 );
} 


//**************************************************************************************
// Utitlities for dump memory, free kernel pages, mips soft reset, etc.
//**************************************************************************************

/***********************************************************************
 * Function Name: dumpaddr
 * Description  : Display a hex dump of the specified address.
 ***********************************************************************/
void dumpaddr( unsigned char *pAddr, int nLen )
{
    static char szHexChars[] = "0123456789abcdef";
    char szLine[80];
    char *p = szLine;
    unsigned char ch, *q;
    int i, j;
    unsigned long ul;

    while( nLen > 0 )
    {
        sprintf( szLine, "%8.8lx: ", (unsigned long) pAddr );
        p = szLine + strlen(szLine);

        for(i = 0; i < 16 && nLen > 0; i += sizeof(long), nLen -= sizeof(long))
        {
            ul = *(unsigned long *) &pAddr[i];
            q = (unsigned char *) &ul;
            for( j = 0; j < sizeof(long); j++ )
            {
                *p++ = szHexChars[q[j] >> 4];
                *p++ = szHexChars[q[j] & 0x0f];
                *p++ = ' ';
            }
        }

        for( j = 0; j < 16 - i; j++ )
            *p++ = ' ', *p++ = ' ', *p++ = ' ';

        *p++ = ' ', *p++ = ' ', *p++ = ' ';

        for( j = 0; j < i; j++ )
        {
            ch = pAddr[j];
            *p++ = (ch > ' ' && ch < '~') ? ch : '.';
        }

        *p++ = '\0';
        printk( "%s\r\n", szLine );

        pAddr += i;
    }
    printk( "\r\n" );
} /* dumpaddr */


void kerSysMipsSoftReset(void)
{
#if defined(CONFIG_BCM96348)
    typedef void (*FNPTR) (void);
    FNPTR bootaddr = (FNPTR) FLASH_BASE;
    int i;

    /* Disable interrupts. */
    __cli();

    /* Reset all blocks. */
    PERF->BlockSoftReset &= ~BSR_ALL_BLOCKS;
    for( i = 0; i < 1000000; i++ )
        ;
    PERF->BlockSoftReset |= BSR_ALL_BLOCKS;

    PERF->pll_control |= SOFT_RESET;    // soft reset mips
    /* Jump to the power on address. */
    //(*bootaddr) ();
#else
    PERF->pll_control |= SOFT_RESET;    // soft reset mips
#endif
}


int kerSysGetMacAddress( unsigned char *pucaMacAddr, unsigned long ulId )
{
    int nRet = 0;
    PMAC_ADDR_INFO pMai = NULL;
    PMAC_ADDR_INFO pMaiFreeNoId = NULL;
    PMAC_ADDR_INFO pMaiFreeId = NULL;
    unsigned long i = 0, ulIdxNoId = 0, ulIdxId = 0, shiftedIdx = 0;
    
    /* CMO -- Fix le problème avec les adresses mac que l'on n'arrive pas
     *  * à relire plusieurs fois */
    /* inv_xde */
    if (boot_loader_type == BOOT_CFE)
      memcpy( pucaMacAddr, g_pNvramInfo->ucaBaseMacAddr,
	      NVRAM_MAC_ADDRESS_LEN );
    else {
      pucaMacAddr[0] = 0x00;
      pucaMacAddr[1] = 0x07;
      pucaMacAddr[2] = 0x3A;
      pucaMacAddr[3] = 0xFF;
      pucaMacAddr[4] = 0xFF;
      pucaMacAddr[5] = 0xFF;
    }
    
    return nRet;	

#if 0    
    for( i = 0, pMai = g_pNvramInfo->MacAddrs; i < g_pNvramInfo->ulNumMacAddrs;
        i++, pMai++ )
    {
        if( ulId == pMai->ulId || ulId == MAC_ADDRESS_ANY )
        {
            /* This MAC address has been used by the caller in the past. */
            memcpy( pucaMacAddr, g_pNvramInfo->ucaBaseMacAddr,
                NVRAM_MAC_ADDRESS_LEN );
            shiftedIdx = i;
            pucaMacAddr[NVRAM_MAC_ADDRESS_LEN - CHANGED_OCTET] += (shiftedIdx << SHIFT_BITS);
            pMai->chInUse = 1;
            pMaiFreeNoId = pMaiFreeId = NULL;
            break;
        }
        else
            if( pMai->chInUse == 0 )
            {
                if( pMai->ulId == 0 && pMaiFreeNoId == NULL )
                {
                    /* This is an available MAC address that has never been
                     * used.
                     */
                    pMaiFreeNoId = pMai;
                    ulIdxNoId = i;
                }
                else
                    if( pMai->ulId != 0 && pMaiFreeId == NULL )
                    {
                        /* This is an available MAC address that has been used
                         * before.  Use addresses that have never been used
                         * first, before using this one.
                         */
                        pMaiFreeId = pMai;
                        ulIdxId = i;
                    }
            }
    }

    if( pMaiFreeNoId || pMaiFreeId )
    {
        /* An available MAC address was found. */
        memcpy(pucaMacAddr, g_pNvramInfo->ucaBaseMacAddr,NVRAM_MAC_ADDRESS_LEN);
        if( pMaiFreeNoId )
        {
            shiftedIdx = ulIdxNoId;
            pucaMacAddr[NVRAM_MAC_ADDRESS_LEN - CHANGED_OCTET] += (shiftedIdx << SHIFT_BITS);
            pMaiFreeNoId->ulId = ulId;
            pMaiFreeNoId->chInUse = 1;
        }
        else
        {
            shiftedIdx = ulIdxId;
            pucaMacAddr[NVRAM_MAC_ADDRESS_LEN - CHANGED_OCTET] += (shiftedIdx << SHIFT_BITS);
            pMaiFreeId->ulId = ulId;
            pMaiFreeId->chInUse = 1;
        }
    }
    else
        if( i == g_pNvramInfo->ulNumMacAddrs )
            nRet = -EADDRNOTAVAIL;

    return( nRet );
#endif
} /* kerSysGetMacAddr */

int kerSysReleaseMacAddress( unsigned char *pucaMacAddr )
{
    int nRet = -EINVAL;
    unsigned long ulIdx = 0;
    int idx = (pucaMacAddr[NVRAM_MAC_ADDRESS_LEN - CHANGED_OCTET] -
        g_pNvramInfo->ucaBaseMacAddr[NVRAM_MAC_ADDRESS_LEN - CHANGED_OCTET]);

    // if overflow 255 (negitive), add 256 to have the correct index
    if (idx < 0)
        idx += 256;
    ulIdx = (unsigned long) (idx >> SHIFT_BITS);

    if( ulIdx < g_pNvramInfo->ulNumMacAddrs )
    {
        PMAC_ADDR_INFO pMai = &g_pNvramInfo->MacAddrs[ulIdx];
        if( pMai->chInUse == 1 )
        {
            pMai->chInUse = 0;
            nRet = 0;
        }
    }

    return( nRet );
} /* kerSysReleaseMacAddr */

int kerSysGetSdramSize( void )
{
  /* inv_xde : retourne la taille de SDRAM du système ;
     correspond à l'adresse de base pour la SDRAM utilisée par le processeur ADSL */
  if (boot_loader_type == BOOT_CFE) {
    return( (int) g_pNvramInfo->ulSdramSize );
  }
  else {
    printk("kerSysGetSdramSize : 0x%08X\n", (int)getMemorySize() + 0x00040000);
    return((int)getMemorySize() + 0x00040000);
  }
} /* kerSysGetSdramSize */

void kerSysLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    if (g_ledInitialized)
      boardLedCtrl(ledName, ledState);
}

//********************************************************************************************
// misc. ioctl calls come to here. (flash, led, reset, kernel memory access, etc.)
//********************************************************************************************
static int board_ioctl( struct inode *inode, struct file *flip,
                        unsigned int command, unsigned long arg )
{
    int ret = 0;
    BOARD_IOCTL_PARMS ctrlParms;
    unsigned char ucaMacAddr[NVRAM_MAC_ADDRESS_LEN];
    int allowedSize;

    /* inv_xde */
    switch (command) 
    {
        case BOARD_IOCTL_FLASH_INIT:
            // not used for now.  kerSysBcmImageInit();
            break;


        case BOARD_IOCTL_FLASH_WRITE:
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0)
            {
                NVRAM_DATA SaveNvramData;
                PNVRAM_DATA pNvramData = (PNVRAM_DATA) get_nvram_start_addr();

                switch (ctrlParms.action)
                {
                    case SCRATCH_PAD:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysScratchPadSet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case PERSISTENT:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysPersistentSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;
               
                    case NVRAM:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysNvRamSet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case BCM_IMAGE_CFE:
		      if (boot_loader_type == BOOT_CFE) {
                        if( ctrlParms.strLen <= 0 || ctrlParms.strLen > FLASH45_LENGTH_BOOT_ROM )
                        {
                            printk("Illegal CFE size [%d]. Size allowed: [%d]\n",
                                ctrlParms.strLen, FLASH45_LENGTH_BOOT_ROM);
                            ret = -1;
                            break;
                        }

                        // save NVRAM data into a local structure
                        memcpy( &SaveNvramData, pNvramData, sizeof(NVRAM_DATA) );

                        ret = kerSysBcmImageSet(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen);

                        // if nvram is not valid, restore the current nvram settings
                        if( BpSetBoardId( pNvramData->szBoardId ) != BP_SUCCESS &&
                            *(unsigned long *) pNvramData == NVRAM_DATA_ID )
                        {
                            kerSysNvRamSet((char *) &SaveNvramData, sizeof(SaveNvramData), 0);
                        }
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;
                        
                    case BCM_IMAGE_FS:
		      if (boot_loader_type == BOOT_CFE) {
                        allowedSize = (int) flash_get_total_size() - FLASH_RESERVED_AT_END - TAG_LEN - FLASH45_LENGTH_BOOT_ROM;
                        if( ctrlParms.strLen <= 0 || ctrlParms.strLen > allowedSize)
                        {
                            printk("Illegal root file system size [%d]. Size allowed: [%d]\n",
                                ctrlParms.strLen,  ctrlParms.offset - allowedSize);
                            ret = -1;
                            break;
                        }
                        ret = kerSysBcmImageSet(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen);
                        kerSysMipsSoftReset();
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case BCM_IMAGE_KERNEL:
		      if (boot_loader_type == BOOT_CFE) {
                        allowedSize = (int) flash_get_total_size() - FLASH_RESERVED_AT_END - TAG_LEN - ctrlParms.offset;

                        printk("kernel size = [%d]. Allowed size = [%d]\n", ctrlParms.strLen, allowedSize);

                        if( ctrlParms.strLen <= 0 || ctrlParms.strLen > allowedSize )
                        {
                            printk("Kernel size is over the limit by [%d] bytes\n", 
                                ctrlParms.strLen - allowedSize);
                            ret = -1;
                            break;
                        }
                        ret = kerSysBcmImageSet(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case BCM_IMAGE_WHOLE:
		      if (boot_loader_type == BOOT_CFE) {
                        if(ctrlParms.strLen <= 0)
                        {
                            printk("Illegal flash image size [%d].\n", ctrlParms.strLen);
                            ret = -1;
                            break;
                        }

                        // save NVRAM data into a local structure
                        memcpy( &SaveNvramData, pNvramData, sizeof(NVRAM_DATA) );

                        ret = kerSysBcmImageSet(ctrlParms.offset, ctrlParms.string, ctrlParms.strLen);

                        // if nvram is not valid, restore the current nvram settings
                        if( BpSetBoardId( pNvramData->szBoardId ) != BP_SUCCESS &&
                            *(unsigned long *) pNvramData == NVRAM_DATA_ID )
                        {
                            kerSysNvRamSet((char *) &SaveNvramData, sizeof(SaveNvramData), 0);
                        }

                        kerSysMipsSoftReset();
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    default:
                        ret = -EINVAL;
                        printk("flash_ioctl_command: invalid command %d\n", ctrlParms.action);
                        break;
                }
                ctrlParms.result = ret;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            }
            else
                ret = -EFAULT;
            break;

        case BOARD_IOCTL_FLASH_READ:
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
                switch (ctrlParms.action)
                {
                    case SCRATCH_PAD:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysScratchPadGet(ctrlParms.string, ctrlParms.buf, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case PERSISTENT:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysPersistentGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case NVRAM:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysNvRamGet(ctrlParms.string, ctrlParms.strLen, ctrlParms.offset);
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    case FLASH_SIZE:
		      if (boot_loader_type == BOOT_CFE) {
                        ret = kerSysFlashSizeGet();
		      }
		      else {
			printk("RedBoot :  not supported\n");
			return(-EINVAL);
		      }
		      break;

                    default:
                        ret = -EINVAL;
                        printk("Not supported.  invalid command %d\n", ctrlParms.action);
                        break;
                }
                ctrlParms.result = ret;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            }
            else
                ret = -EFAULT;
            break;

        case BOARD_IOCTL_GET_NR_PAGES:
            ctrlParms.result = nr_free_pages() + atomic_read(&page_cache_size);
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
            break;

        case BOARD_IOCTL_DUMP_ADDR:
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
                dumpaddr( (unsigned char *) ctrlParms.string, ctrlParms.strLen );
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else
                ret = -EFAULT;
            break;

        case BOARD_IOCTL_SET_MEMORY:
	  if (boot_loader_type == BOOT_CFE) {
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
                unsigned long  *pul = (unsigned long *)  ctrlParms.string;
                unsigned short *pus = (unsigned short *) ctrlParms.string;
                unsigned char  *puc = (unsigned char *)  ctrlParms.string;
                switch( ctrlParms.strLen )
                {
                    case 4:
                        *pul = (unsigned long) ctrlParms.offset;
                        break;
                    case 2:
                        *pus = (unsigned short) ctrlParms.offset;
                        break;
                    case 1:
                        *puc = (unsigned char) ctrlParms.offset;
                        break;
                }
                dumpaddr( (unsigned char *) ctrlParms.string, sizeof(long) );
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else
                ret = -EFAULT;
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;
      
        case BOARD_IOCTL_MIPS_SOFT_RESET:
            kerSysMipsSoftReset();
            break;

        case BOARD_IOCTL_LED_CTRL:
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
	            kerSysLedCtrl((BOARD_LED_NAME)ctrlParms.strLen, (BOARD_LED_STATE)ctrlParms.offset);
	            ret = 0;
	        }
            break;

        case BOARD_IOCTL_GET_ID:
	  if (boot_loader_type == BOOT_CFE) {
            if (copy_from_user((void*)&ctrlParms, (void*)arg,
			       sizeof(ctrlParms)) == 0) 
	      {
                if( ctrlParms.string )
		  {
                    char *p = (char *) get_system_type();
                    if( strlen(p) + 1 < ctrlParms.strLen )
		      ctrlParms.strLen = strlen(p) + 1;
                    __copy_to_user(ctrlParms.string, p, ctrlParms.strLen);
		  }
		
                ctrlParms.result = 0;
                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
			       sizeof(BOARD_IOCTL_PARMS));
	      }
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;

        case BOARD_IOCTL_GET_MAC_ADDRESS:
	  if (boot_loader_type == BOOT_CFE) {
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
                ctrlParms.result = kerSysGetMacAddress( ucaMacAddr,
                    ctrlParms.offset );

                if( ctrlParms.result == 0 )
                {
                    __copy_to_user(ctrlParms.string, ucaMacAddr,
                        sizeof(ucaMacAddr));
                }

                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                    sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else
	      ret = -EFAULT;
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;
	  
        case BOARD_IOCTL_RELEASE_MAC_ADDRESS:
	  if (boot_loader_type == BOOT_CFE) {
            if (copy_from_user((void*)&ctrlParms, (void*)arg, sizeof(ctrlParms)) == 0) 
            {
                if (copy_from_user((void*)ucaMacAddr, (void*)ctrlParms.string, \
                     NVRAM_MAC_ADDRESS_LEN) == 0) 
                {
                    ctrlParms.result = kerSysReleaseMacAddress( ucaMacAddr );
                }
                else
                {
                    ctrlParms.result = -EACCES;
                }

                __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms,
                    sizeof(BOARD_IOCTL_PARMS));
                ret = 0;
            }
            else
                ret = -EFAULT;
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;

        case BOARD_IOCTL_GET_PSI_SIZE:
	  if (boot_loader_type == BOOT_CFE) {
            ctrlParms.result = (int) g_pNvramInfo->ulPsiSize;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;

        case BOARD_IOCTL_GET_SDRAM_SIZE:
	  if (boot_loader_type == BOOT_CFE) {
            ctrlParms.result = (int) g_pNvramInfo->ulSdramSize;
            __copy_to_user((BOARD_IOCTL_PARMS*)arg, &ctrlParms, sizeof(BOARD_IOCTL_PARMS));
            ret = 0;
	  }
	  else {
	    printk("RedBoot :  not supported\n");
	    return(-EINVAL);
	  }
	  break;

        default:
            ret = -EINVAL;
            printk("board_ioctl: invalid command %x, cmd %d .\n",command,_IOC_NR(command));
            break;

  } /* switch */

  return (ret);

} /* board_ioctl */


/***************************************************************************
 * MACRO to call driver initialization and cleanup functions.
 ***************************************************************************/
module_init( brcm_board_init );
module_exit( brcm_board_cleanup );

EXPORT_SYMBOL(kerSysNvRamGet);
EXPORT_SYMBOL(dumpaddr);
EXPORT_SYMBOL(kerSysGetMacAddress);
EXPORT_SYMBOL(kerSysReleaseMacAddress);
EXPORT_SYMBOL(kerSysGetSdramSize);
EXPORT_SYMBOL(kerSysLedCtrl);
EXPORT_SYMBOL(kerSysLedRegisterHwHandler);
EXPORT_SYMBOL(BpGetBoardIds);
EXPORT_SYMBOL(BpGetSdramSize);
EXPORT_SYMBOL(BpGetPsiSize);
EXPORT_SYMBOL(BpGetEthernetMacInfo);
EXPORT_SYMBOL(BpGetRj11InnerOuterPairGpios);
EXPORT_SYMBOL(BpGetPressAndHoldResetGpio);
EXPORT_SYMBOL(BpGetVoipResetGpio);
EXPORT_SYMBOL(BpGetVoipIntrGpio);
EXPORT_SYMBOL(BpGetPcmciaResetGpio);
EXPORT_SYMBOL(BpGetRtsCtsUartGpios);
#if 0 
EXPORT_SYMBOL(BpGetAdslLedGpio);
EXPORT_SYMBOL(BpGetAdslFailLedGpio);
EXPORT_SYMBOL(BpGetWirelessLedGpio);
EXPORT_SYMBOL(BpGetUsbLedGpio);
EXPORT_SYMBOL(BpGetHpnaLedGpio);
EXPORT_SYMBOL(BpGetWanDataLedGpio);
EXPORT_SYMBOL(BpGetPppLedGpio);
EXPORT_SYMBOL(BpGetPppFailLedGpio);
EXPORT_SYMBOL(BpGetVoipLedGpio);
#endif
EXPORT_SYMBOL(BpGetWirelessExtIntr);
EXPORT_SYMBOL(BpGetAdslDyingGaspExtIntr);
EXPORT_SYMBOL(BpGetVoipExtIntr);
EXPORT_SYMBOL(BpGetHpnaExtIntr);
EXPORT_SYMBOL(BpGetHpnaChipSelect);
EXPORT_SYMBOL(BpGetVoipChipSelect);

