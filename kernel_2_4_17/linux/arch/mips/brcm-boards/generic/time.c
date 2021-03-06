/*
 * Carsten Langgaard, carstenl@mips.com
 * Copyright (C) 1999,2000 MIPS Technologies, Inc.  All rights reserved.
 *
 * ########################################################################
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 * ########################################################################
 *
 * Setting up the clock on the MIPS boards.
 *
 */

#include <linux/config.h>
#include <linux/init.h>
#include <linux/kernel_stat.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>

#include <asm/mipsregs.h>
#include <asm/ptrace.h>
#include <asm/div64.h>

#include <linux/timex.h>

#if defined CONFIG_BCM96345
#include <6345_map_part.h>
#include <6345_intr.h>
#endif
#if defined CONFIG_BCM96348
#include <6348_map_part.h>
#include <6348_intr.h>
#endif
#if defined CONFIG_BCM96338
#include <6338_map_part.h>
#include <6338_intr.h>
#endif

unsigned long r4k_interval;	/* Amount to increment compare reg each time */
static unsigned long r4k_cur;	/* What counter should be at next timer irq */

/* Cycle counter value at the previous timer interrupt.. */
static unsigned int timerhi = 0, timerlo = 0;

extern volatile unsigned long wall_jiffies;
extern rwlock_t xtime_lock;

/* Optional board-specific timer routine */
void (*board_timer_interrupt)(int irq, void *dev_id, struct pt_regs * regs);

static inline void ack_r4ktimer(unsigned long newval)
{
	write_32bit_cp0_register(CP0_COMPARE, newval);
}

/*
 * There are a lot of conceptually broken versions of the MIPS timer interrupt
 * handler floating around.  This one is rather different, but the algorithm
 * is provably more robust.
 */
void brcm_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
	unsigned int count;

	if (r4k_interval == 0)
		goto null;

	do {
		do_timer(regs);

		if (board_timer_interrupt)
			board_timer_interrupt(irq, dev_id, regs);

		r4k_cur += r4k_interval;
		ack_r4ktimer(r4k_cur);

	} while (((count = (unsigned long)read_32bit_cp0_register(CP0_COUNT))
		  - r4k_cur) < 0x7fffffff);

	if (!jiffies) {
		/*
		 * If jiffies has overflowed in this timer_interrupt we must
		 * update the timer[hi]/[lo] to make do_fast_gettimeoffset()
		 * quotient calc still valid. -arca
		 */
		timerhi = timerlo = 0;
	} else {
		/*
		 * The cycle counter is only 32 bit which is good for about
		 * a minute at current count rates of upto 150MHz or so.
		 */
		timerhi += (count < timerlo);	/* Wrap around */
		timerlo = count;
	}

	return;

null:
	ack_r4ktimer(0);
}

static struct irqaction brcm_timer_action = {
	handler: brcm_timer_interrupt,
	flags: SA_INTERRUPT,
	mask: 0,
	name: "timer",
	next: NULL,
	dev_id: brcm_timer_interrupt,
};

void __init time_init(void)
{
	r4k_cur = (read_32bit_cp0_register(CP0_COUNT) + r4k_interval);
	write_32bit_cp0_register(CP0_COMPARE, r4k_cur);

	setup_irq(MIPS_TIMER_INT, &brcm_timer_action);
	set_cp0_status(IE_IRQ5);
}

/* This is for machines which generate the exact clock. */
#define USECS_PER_JIFFY (1000000/HZ)
#define USECS_PER_JIFFY_FRAC (0x100000000*1000000/HZ&0xffffffff)

static void call_do_div64_32( unsigned long *res, unsigned int high,
    unsigned int low, unsigned long base )
{
    do_div64_32(*res, high, low, base);
}

/*
 * FIXME: Does playing with the RP bit in c0_status interfere with this code?
 */
static unsigned long do_fast_gettimeoffset(void)
{
	u32 count;
	unsigned long res, tmp;

	/* Last jiffy when do_fast_gettimeoffset() was called. */
	static unsigned long last_jiffies=0;
	unsigned long quotient;

	/*
	 * Cached "1/(clocks per usec)*2^32" value.
	 * It has to be recalculated once each jiffy.
	 */
	static unsigned long cached_quotient=0;

	tmp = jiffies;

	quotient = cached_quotient;

	if (tmp && last_jiffies != tmp) {
		last_jiffies = tmp;
#ifdef CONFIG_CPU_MIPS32
		if (last_jiffies != 0) {

			unsigned long r0;
			/* gcc 3.0.1 gets an internal compiler error if there are two
			 * do_div64_32 inline macros.  To work around this problem,
			 * do_div64_32 is called as a function.
			 */
			call_do_div64_32(&r0, timerhi, timerlo, tmp);
			call_do_div64_32(&quotient, USECS_PER_JIFFY,
				    USECS_PER_JIFFY_FRAC, r0);

			cached_quotient = quotient;

		}
#else
		__asm__(".set\tnoreorder\n\t"
			".set\tnoat\n\t"
			".set\tmips3\n\t"
			"lwu\t%0,%2\n\t"
			"dsll32\t$1,%1,0\n\t"
			"or\t$1,$1,%0\n\t"
			"ddivu\t$0,$1,%3\n\t"
			"mflo\t$1\n\t"
			"dsll32\t%0,%4,0\n\t"
			"nop\n\t"
			"ddivu\t$0,%0,$1\n\t"
			"mflo\t%0\n\t"
			".set\tmips0\n\t"
			".set\tat\n\t"
			".set\treorder"
			:"=&r" (quotient)
			:"r" (timerhi),
			 "m" (timerlo),
			 "r" (tmp),
			 "r" (USECS_PER_JIFFY)
			:"$1");
		cached_quotient = quotient;
#endif
	}

	/* Get last timer tick in absolute kernel time */
	count = read_32bit_cp0_register(CP0_COUNT);

	/* .. relative to previous jiffy (32 bits is enough) */
	count -= timerlo;

	__asm__("multu\t%1,%2\n\t"
		"mfhi\t%0"
		:"=r" (res)
		:"r" (count),
		 "r" (quotient));

	/*
 	 * Due to possible jiffies inconsistencies, we need to check 
	 * the result so that we'll get a timer that is monotonic.
	 */
	if (res >= USECS_PER_JIFFY)
		res = USECS_PER_JIFFY-1;

	return res;
}

void do_gettimeofday(struct timeval *tv)
{
	unsigned int flags;

	read_lock_irqsave (&xtime_lock, flags);
	*tv = xtime;
	tv->tv_usec += do_fast_gettimeoffset();

	/*
	 * xtime is atomically updated in timer_bh. jiffies - wall_jiffies
	 * is nonzero if the timer bottom half hasnt executed yet.
	 */
	if (jiffies - wall_jiffies)
		tv->tv_usec += USECS_PER_JIFFY;

	read_unlock_irqrestore (&xtime_lock, flags);

	if (tv->tv_usec >= 1000000) {
		tv->tv_usec -= 1000000;
		tv->tv_sec++;
	}
}

void do_settimeofday(struct timeval *tv)
{
	write_lock_irq (&xtime_lock);

	/* This is revolting. We need to set the xtime.tv_usec correctly.
	 * However, the value in this location is is value at the last tick.
	 * Discover what correction gettimeofday would have done, and then
	 * undo it!
	 */
	tv->tv_usec -= do_fast_gettimeoffset();

	if (tv->tv_usec < 0) {
		tv->tv_usec += 1000000;
		tv->tv_sec--;
	}

	xtime = *tv;
	time_adjust = 0;		/* stop active adjtime() */
	time_status |= STA_UNSYNC;
	time_maxerror = NTP_PHASE_LIMIT;
	time_esterror = NTP_PHASE_LIMIT;

	write_unlock_irq (&xtime_lock);
}
