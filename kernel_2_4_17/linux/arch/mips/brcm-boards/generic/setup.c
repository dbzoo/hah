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
/*
 * Generic setup routines for Broadcom MIPS boards
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/kdev_t.h>
#include <linux/types.h>
#include <linux/console.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/module.h>

#include <asm/addrspace.h>
#include <asm/bcache.h>
#include <asm/keyboard.h>
#include <asm/irq.h>
#include <asm/reboot.h>
#include <asm/gdb-stub.h>
#include <asm/mc146818rtc.h> 

/* This function should be in a board specific directory.  For now,
 * assume that all boards that include this file use a Broadcom chip
 * with a soft reset bit in the PLL control register.
 */
static void brcm_machine_restart(char *command)
{
    const unsigned long ulSoftReset = 0x00000001;
    unsigned long *pulPllCtrl = (unsigned long *) 0xfffe0008;
//    *pulPllCtrl |= ulSoftReset;
    kerSysMipsSoftReset();
}

static void brcm_machine_halt(void)
{
	printk("System halted\n");
	while (1);
}

#define ALLINTS_NOTIMER (IE_IRQ0 | IE_IRQ1 | IE_IRQ2 | IE_IRQ3 | IE_IRQ4)

void __init brcm_irq_setup(void)
{
	extern asmlinkage void brcmIRQ(void);

	/* In 2.4.3 Kernel this was done in trap_init. In 2.4.17 it was
	 * moved to SMP code, which is only used by MIPS64. In other words
	 * Linux is free, you get what you pay for */
	clear_cp0_status(ST0_BEV);
    set_except_vector(0, brcmIRQ);
	change_cp0_status(ST0_IM, ALLINTS_NOTIMER);

#ifdef CONFIG_REMOTE_DEBUG
	rs_kgdb_hook(0);
#endif
}

void __init brcm_setup(void)
{
	extern int panic_timeout;

	_machine_restart = brcm_machine_restart;
	_machine_halt = brcm_machine_halt;
	_machine_power_off = brcm_machine_halt;

	panic_timeout = 1;

}

/***************************************************************************
 * C++ New and delete operator functions
 ***************************************************************************/

/* void *operator new(unsigned int sz) */
void *_Znwj(unsigned int sz)
{
    return( kmalloc(sz, GFP_KERNEL) );
}

/* void *operator new[](unsigned int sz)*/
void *_Znaj(unsigned int sz)
{
    return( kmalloc(sz, GFP_KERNEL) );
}

/* placement new operator */
/* void *operator new (unsigned int size, void *ptr) */
void *ZnwjPv(unsigned int size, void *ptr)
{
    return ptr;
}

/* void operator delete(void *m) */
void _ZdlPv(void *m)
{
    kfree(m);
}

/* void operator delete[](void *m) */
void _ZdaPv(void *m)
{
    kfree(m);
}

EXPORT_SYMBOL(_Znwj);
EXPORT_SYMBOL(_Znaj);
EXPORT_SYMBOL(ZnwjPv);
EXPORT_SYMBOL(_ZdlPv);
EXPORT_SYMBOL(_ZdaPv);

