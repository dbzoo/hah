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
//**************************************************************************************
// File Name  : bcm6345_cons.c
//
// Description: Serial port driver for BCM96353 board based bcm1100 which based on dz.c 
//              (DECStations equiped with the DZ chipset.)  Some of the functions are 
//              probably not got called and later on if code size is an issue, they can be
//              removed.
//
// Updates    : 08/18/2001  seanl.  Created.
//**************************************************************************************



//#define AMO
#ifdef AMO
#define PRINT_AMO(arg...)  do {printk( KERN_DEBUG arg); printk( "\n" ); } while (0)
#else
#define PRINT_AMO(arg...)
#endif

#define CARDNAME 	"BCM6345_CONS"
#define VERSION     "1.0"
#define VER_STR     "Module " __FILE__ " v" VERSION "\n"


#ifdef MODULE
#include <linux/module.h>
#include <linux/version.h>
#else
#define MOD_INC_USE_COUNT
#define MOD_DEC_USE_COUNT
#endif

#include <linux/kernel.h>
#include <linux/init.h> 
#include <linux/malloc.h>
#include <linux/interrupt.h>

/* for definition of struct console */
#if defined(CONFIG_SERIAL_CONSOLE)
#include <linux/console.h>
#endif  

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial.h>

#include <asm/uaccess.h>

#include <board.h>
#include <bcmtypes.h>

#if defined(CONFIG_BCM96348)
#include <6348_map_part.h>
#include <6348_intr.h>
#endif

#if defined(CONFIG_BCM96345)
#include <6345_map_part.h>
#include <6345_intr.h>
#endif

#if defined(CONFIG_BCM96338)
#include <6338_map_part.h>
#include <6338_intr.h>
#endif


#include "bcm63xx_cons.h"

extern void _putc(char);
extern void _puts(const char *);



//choix entre ttyS0 et ttyS1
int tty_selector = 0;

static int tty_selector_proc(struct file *file, const char *buf, unsigned long count, void *data)
{
  const char *p = buf;
  signed int i=0;
  signed int res;
  if (!p || !count)
    return 0;
  res=sscanf (p,"%d",&i);
  if ((res == 1) && (i >= 0) && (i < DZ_NB_PORT)) tty_selector=i;
  return count ;
}

static int proc_read_sdram_cas(char *page, char **start, off_t offset, int count, int *eof, void *priv)
{
  char *ptr = page;
  int len=0;

  ptr += sprintf( ptr, "%d\n", (*((volatile unsigned long *)0xfffe0008) >> 2) & 1 );

  len = ptr - page;

  if (len <= count + offset)
    *eof = 1;

  *start = page + offset;
  len -= offset;

  if (len > count)
    len = count;
  if (len < 0)
    len = 0;

  return len;
}



#if 0
static int change_fbus_speed_proc(struct file *file, const char *buf, unsigned long count, void *data)
{
  const char *p = buf;
  signed int i=0;
  signed int res;
  unsigned long tmpVal;
  if (!p || !count)
    return 0;


  res=sscanf (p,"%d",&i);
  if (res == 1)    switch (i)
  {

    case 88:
      printk("set FBUS=88Mhz\n");
    tmpVal = *((volatile unsigned long *)0xfffe0008) ;
    tmpVal &= ~(0X0F << 20);
    tmpVal |= 4 << 20; //PLL_FBDIV = 2
    *((volatile unsigned long *)0xfffe0008) = tmpVal ;
    FPERIPH = 88 * 500000; //le driver d'uart à besoin de cette valeur
      break;

    case 100:
      printk("set FBUS=100Mhz\n");
    tmpVal = *((volatile unsigned long *)0xfffe0008) ;
    tmpVal &= ~(0X0F << 20);
    tmpVal |= 2 << 20; //PLL_FBDIV = 2
    *((volatile unsigned long *)0xfffe0008) = tmpVal ;
    FPERIPH = 100 * 500000; //le driver d'uart à besoin de cette valeur
      break;

    case 117:
      printk("set FBUS=117Mhz\n");
    tmpVal = *((volatile unsigned long *)0xfffe0008) ;
    tmpVal &= ~(0X0F << 20);
    tmpVal |= 1 << 20; //PLL_FBDIV = 1
    *((volatile unsigned long *)0xfffe0008) = tmpVal ;
    FPERIPH = 117 * 500000; //le driver d'uart à besoin de cette valeur
      break;

    default:
      printk("invalid FBUS\n");
  } else {
    printk("invalid FBUS\n");
  }


  return count ;
}
#endif

#if (DZ_NB_PORT == 1)
#define DZ_CURRENT_LINE  0
#else
#define DZ_CURRENT_LINE tty_selector
#endif

typedef struct dz_serial {
	unsigned                port;                /* base address for the port */
	int                     type;
	int                     flags; 
	int						irq;
	volatile Uart *         baud_base;
	int                     blocked_open;
	unsigned short          close_delay;
	unsigned short          closing_wait;
	unsigned short          line;                /* port/line number */
	unsigned short          cflags;              /* line configuration flag */
	unsigned short          x_char;              /* xon/xoff character */
	unsigned short          read_status_mask;    /* mask for read condition */
	unsigned short          ignore_status_mask;  /* mask for ignore condition */
	unsigned long           event;               /* mask used in BH */
	unsigned char           *xmit_buf;           /* Transmit buffer */
	int                     xmit_head;           /* Position of the head */
	int                     xmit_tail;           /* Position of the tail */
	int                     xmit_cnt;            /* Count of the chars in the buffer */
  int                     wakeup_tty;
	int                     count;               /* indicates how many times it has been opened */
	int                     magic;

  int use_ctsrts;

	struct async_icount     icount;              /* keep track of things ... */
	struct tty_struct       *tty;                /* tty associated */
	struct tq_struct        tqueue;              /* Queue for BH */
	struct tq_struct        tqueue_hangup;
	struct termios          normal_termios;
	struct termios          callout_termios;
	wait_queue_head_t       open_wait;
	wait_queue_head_t       close_wait;

	long                    session;             /* Session of opening process */
	long                    pgrp;                /* pgrp of opening process */

	unsigned char           is_initialized;
}	Context;

unsigned char dz_common_tx_buff[DZ_XMIT_SIZE]; //un seul buffer pour les N ttys qui sont sur le meme uart

#define DZ_TX_FIFO_SIZE  16
#define DZ_TX_FIFO_THRESHOLD  8
#define DZ_RX_FIFO_SIZE  16
#define DZ_RX_FIFO_THRESHOLD   3



/*---------------------------------------------------------------------*/
/* Define bits in the Interrupt Enable register                        */
/*---------------------------------------------------------------------*/
/* Enable receive interrupt              */
#if DZ_RX_FIFO_THRESHOLD <= 1
#define		RXINT	(RXFIFONE /* | RXOVFERR*/)
#else
#define     RXINT (/*RXOVFERR |*/ RXFIFOTO | RXFIFOTH)
#endif

/* Enable transmit interrupt             */
#define		TXINT    (TXFIFOEMT|TXUNDERR|TXOVFERR|0x8) 

/* Enable receiver line status interrupt */
#define		LSINT    (RXBRK|RXPARERR|RXFRAMERR)

#define stUart ((volatile Uart * const) UART_BASE)
#define BCM_NUM_UARTS		DZ_NB_PORT

DECLARE_TASK_QUEUE(tq_serial);

//wait_queue_head_t keypress_wait; 
static struct dz_serial *lines[ DZ_NB_PORT ];
static unsigned char tmp_buffer[ DZ_XMIT_SIZE ];


#ifdef AMO

#define TRACE_LEN 80
#define AMO_REC(arg...)  do { snprintf( (char *)ser_stat.t[ser_stat.t_c++],T_L, arg ); ser_stat.t_c %=T_N; } while (0)
#define AMO_EVAL(arg...)  arg

#define T_L  80
#define T_N  80
static struct {
  int tx_int_count;
  int tx_byte_count;
  int transmit_chars_call;
  int wakeup_tty;
  const char *state;

  char t[T_L][T_N];
  int t_c;

} ser_stat;
#else
#define AMO_REC(arg...)
#define AMO_EVAL(arg...)
#endif

/*
 * ------------------------------------------------------------
 * dz_in () and dz_out ()
 *
 * These routines are used to access the registers of the DZ 
 * chip, hiding relocation differences between implementation.
 * ------------------------------------------------------------
 */

static inline unsigned short dz_in (struct dz_serial *info, unsigned offset)
{
	volatile unsigned short *addr = (volatile unsigned short *)(info->port + offset);
	return *addr;
}

static inline void dz_out (struct dz_serial *info, unsigned offset, unsigned short value)
{
	volatile unsigned short *addr = (volatile unsigned short *)(info->port + offset);
	*addr = value;
}

/*
 * ------------------------------------------------------------
 * rs_stop () and rs_start ()
 *
 * These routines are called before setting or resetting 
 * tty->stopped. They enable or disable transmitter interrupts, 
 * as necessary.
 * ------------------------------------------------------------
 */
static void 
dz_stop (struct tty_struct *tty)
{
	struct dz_serial *info; 
//	unsigned short mask, tmp;

	AMO_REC("dz_stop");

	if (tty == 0) 
	    return; 
 
	info = (struct dz_serial *)tty->driver_data; 
	     
//	mask = 1 << info->line;
//	tmp = dz_in (info, DZ_TCR);       /* read the TX flag */

//    tmp &= ~mask;                   /* clear the TX flag */
//    dz_out (info, DZ_TCR, tmp);
}  

static void 
bcm_start (struct tty_struct *tty)
{
    //_puts(CARDNAME " Start\n");
}  

/*
 * ------------------------------------------------------------
 * Here starts the interrupt handling routines.  All of the 
 * following subroutines are declared as inline and are folded 
 * into bcm_interrupt.  They were separated out for readability's 
 * sake. 
 *
 * Note: rs_interrupt() is a "fast" interrupt, which means that it
 * runs with interrupts turned off.  People who may want to modify
 * rs_interrupt() should try to keep the interrupt handler as fast as
 * possible.  After you are done making modifications, it is not a bad
 * idea to do:
 * 
 * gcc -S -DKERNEL -Wall -Wstrict-prototypes -O6 -fomit-frame-pointer dz.c
 *
 * and look at the resulting assemble code in serial.s.
 *
 * ------------------------------------------------------------
 */

/*
 * ------------------------------------------------------------
 * dz_sched_event ()
 *
 * This routine is used by the interrupt handler to schedule
 * processing in the software interrupt portion of the driver.
 * ------------------------------------------------------------
 */
static inline void 
dz_sched_event (struct dz_serial *info, int event)
{
  info->event |= 1 << event;
  queue_task (&info->tqueue, &tq_serial);
  mark_bh (SERIAL_BH);
}


/*
 * ------------------------------------------------------------
 * transmit_chars ()
 *
 * This routine deals with outputs

 * return 1 if at least one byte transmitted
 * ------------------------------------------------------------
 */
static inline int
transmit_chars (struct dz_serial * info)
{
  int result = 0;
  AMO_EVAL( int nb = 0;  ser_stat.transmit_chars_call++;  )


  //interruption (for the UART) should be disabled here
  do {
    if (stUart->txf_levl < DZ_TX_FIFO_SIZE)
    {
     //there is still room in the fifo
      if (info->xmit_cnt)
      {
        AMO_EVAL( nb++; ser_stat.tx_byte_count++; )
        stUart->Data = info->xmit_buf[ info->xmit_tail ++ ];
        info->xmit_tail &= (DZ_XMIT_SIZE - 1);
        info->xmit_cnt--;
	result = 1;
      }
      else
      {
        //we have nothing more to send and the fifo is not full. It's useless to request a interruption
        UART->intMask &= ~TXINT; //(0x8);
	PRINT_AMO("end transmit chars");
        break;
      }
    }
    else
    {
      //no more room in the fifo, need a interruption when fifo level reach the threshold
      UART->intMask |= TXINT;//(0x8);
      break;
    }
  } while (1);

  AMO_REC("transmit %d chars", nb);
  return result;
  //signal new room to the tty
#if 0
  if (/*info->wakeup_tty && */(info->xmit_cnt < WAKEUP_CHARS))
  {
    AMO_EVAL( ser_stat.wakeup_tty++; )
    info->wakeup_tty = 0;
    AMO_REC("tty wakeup");
    dz_sched_event( info, DZ_EVENT_WRITE_WAKEUP );
  }
#endif
}




/*
 * ------------------------------------------------------------
 * receive_char ()
 *
 * This routine deals with inputs from any lines.
 * return 1 if at least one byte received
 * ------------------------------------------------------------
 */
static inline int
receive_chars (struct dz_serial * info)
{
	struct tty_struct *tty = 0;
	struct async_icount * icount;
	int ignore = 0;
	int status, tmp;
	UCHAR ch = 0;
	int result = 0;

	tty = info->tty;                  /* now tty points to the proper dev */
	if (! tty)
	{
		//reset the fifo
		stUart->fifoctl |= (1 << 6);
		stUart->fifoctl &= ~(1 << 6);
		return;
	}
	AMO_REC("receive_chars");
    while ((status = stUart->intStatus) & (/*RXINT | */RXFIFONE | RXOVFERR)) { //pour l'instant, on ne fait des choses que s'il y a des characteres. on oublie les erreurs
    		result = 1;
     		//if (status & RXFIFONE)
            		ch = stUart->Data;  // Read the character

		icount = &info->icount;


		if (tty->flip.count >= TTY_FLIPBUF_SIZE)
		{
			printk("no room in ldisc\n");
			break;
		}

		*tty->flip.char_buf_ptr = ch;
		*tty->flip.flag_buf_ptr = 0;
		icount->rx++;

		if (status & RXBRK) {
	    	*tty->flip.flag_buf_ptr = TTY_BREAK;
			icount->brk++;
		} 		

		// keep track of the statistics
		if (status & (RXFRAMERR | RXPARERR | RXOVFERR)) {
        		if (status & RXPARERR)                /* parity error */
        			icount->parity++;
      			else if (status & RXFRAMERR)           /* frame error */
        			icount->frame++;

      			if (status & RXOVFERR) {
				// Overflow. Reset the RX FIFO
				printk("cons: RX overflow");
               		 	stUart->fifoctl |= RSTRXFIFOS;
        			icount->overrun++;
      			}

				// check to see if we should ignore the character
				// and mask off conditions that should be ignored
			if (status & info->ignore_status_mask) {
				if (++ignore > 100 ) break;
					goto ignore_char;
			}

			// Mask off the error conditions we want to ignore
			tmp = status & info->read_status_mask;

			if (tmp & RXPARERR) {
				*tty->flip.flag_buf_ptr = TTY_PARITY;
			} else if (tmp & RXFRAMERR) {
				*tty->flip.flag_buf_ptr = TTY_FRAME;
			}
			if (tmp & RXOVFERR) {
				if (tty->flip.count < TTY_FLIPBUF_SIZE) {
					tty->flip.count++;
					tty->flip.flag_buf_ptr++;
					tty->flip.char_buf_ptr++;
					*tty->flip.flag_buf_ptr = TTY_OVERRUN;
				}
			}
		}
	    tty->flip.flag_buf_ptr++;
	    tty->flip.char_buf_ptr++;
	    tty->flip.count++;

	} 
ignore_char:
return result;
/*
	if (tty) {
    	   tty_flip_buffer_push(tty);
	   }
	   */
}


/*
 * ------------------------------------------------------------
 * check_modem_status ()
 *
 * Only valid for the MODEM line duh !
 * ------------------------------------------------------------
 */
static inline void 
check_modem_status (struct dz_serial *info)
{
  unsigned short status;

  /* if not ne modem line just return */
  if (info->line != DZ_MODEM) return;

  status = dz_in (info, DZ_MSR);
  
  /* it's easy, since DSR2 is the only bit in the register */
  if (status) info->icount.dsr++;
}


/*
 * ------------------------------------------------------------
 * bcm_interrupt ()
 *
 * this is the main interrupt routine for the DZ chip.
 * It deals with the multiple ports.
 * ------------------------------------------------------------
 */
static void 
bcm_interrupt (int irq, void * dev, struct pt_regs * regs)
{
  struct dz_serial * info = lines[ DZ_CURRENT_LINE ];
  UINT16  intStat;
  int tmp;

  int need_tty_wakeup_event = 0;
  int need_tty_flip_buff_push = 0;

  /* get pending interrupt flags from UART  */
  /* Mask with only the serial interrupts that are enabled */
  intStat = stUart->intStatus & (stUart->intMask /*| RXFIFONE | RXOVFERR*/);
  
  // Clear the interrupt
  BcmHalInterruptEnable (INTERRUPT_ID_UART);

  while (intStat)
  {
    if ((intStat & (1 << 0)) && info->use_ctsrts)
    {
      //DeltaIP
      if (UART->DeltaIP_SyncIP & 2)
      {
        //stop transmitter
	PRINT_AMO("CTS->stop TX");
        UART->control &= ~ TXEN;
      }
      else
      {
        //enable transmitter
	PRINT_AMO("CTS->enable TX");
        UART->control |= TXEN;
      }
    }
    else if (intStat & RXINT /*RXFIFONE*/)
    {
      need_tty_flip_buff_push |= receive_chars (info);
    }
    else if (intStat & TXINT) 
    {
      AMO_EVAL( ser_stat.tx_int_count++; )
      need_tty_wakeup_event |= transmit_chars (info);
    }
    else
    {
     /* don't know what it was, so let's mask it */
      PRINT_AMO("cons: undef int");
      stUart->intMask &= ~intStat;
    }
    intStat = stUart->intStatus & (stUart->intMask /*| RXFIFONE | RXOVFERR*/);

  }

   if ( need_tty_wakeup_event && /*info->wakeup_tty && */(info->xmit_cnt < WAKEUP_CHARS))
  {
    AMO_EVAL( ser_stat.wakeup_tty++; )
    info->wakeup_tty = 0;
    AMO_REC("tty wakeup");
    dz_sched_event( info, DZ_EVENT_WRITE_WAKEUP );
  }

  if (need_tty_flip_buff_push && info->tty)
  {
	dz_sched_event( info, DZ_EVENT_FLIP_BUFF_PUSH );
  }
  

  return;
}

/*
 * -------------------------------------------------------------------
 * Here ends the DZ interrupt routines.
 * -------------------------------------------------------------------
 */

/*
 * This routine is used to handle the "bottom half" processing for the
 * serial driver, known also the "software interrupt" processing.
 * This processing is done at the kernel interrupt level, after the
 * rs_interrupt() has returned, BUT WITH INTERRUPTS TURNED ON.  This
 * is where time-consuming activities which can not be done in the
 * interrupt driver proper are done; the interrupt driver schedules
 * them using rs_sched_event(), and they get done here.
 */
static void 
do_serial_bh (void)
{
	AMO_REC("do_serial_bh");
	run_task_queue (&tq_serial);
}

static void 
do_softint (void *private_data)
{
	struct dz_serial *info = (struct dz_serial *)private_data;
	struct tty_struct *tty = info->tty;

	AMO_REC("do_softint");

	if (!tty) return;

	if (test_and_clear_bit (DZ_EVENT_WRITE_WAKEUP, &info->event)) {
		AMO_REC("wakeup");

		if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup)
		{
			PRINT_AMO("wakeup tty");
			(tty->ldisc.write_wakeup) (tty);
		}
		wake_up_interruptible (&tty->write_wait);
	}
	if (test_and_clear_bit (DZ_EVENT_FLIP_BUFF_PUSH, &info->event)) {
		if (tty) tty_flip_buffer_push(tty);
	}
}

/*
 * -------------------------------------------------------------------
 * This routine is called from the scheduler tqueue when the interrupt
 * routine has signalled that a hangup has occurred.  The path of
 * hangup processing is:
 *
 *      serial interrupt routine -> (scheduler tqueue) ->
 *      do_serial_hangup() -> tty->hangup() -> rs_hangup()
 * ------------------------------------------------------------------- 
 */
static void 
do_serial_hangup (void *private_data)
{
	AMO_REC("do_serial_hangup");
	struct dz_serial *info = (struct dz_serial *)private_data;
	struct tty_struct *tty = info->tty;;
	    
	if (!tty) return;

	tty_hangup (tty);
}

/*
 * -------------------------------------------------------------------
 * startup ()
 *
 * various initialization tasks
 * ------------------------------------------------------------------- 
 */
static int 
startup (struct dz_serial *info)
{
    // Port is already started...
    return 0;
}

/* 
 * -------------------------------------------------------------------
 * shutdown ()
 *
 * This routine will shutdown a serial port; interrupts are disabled, and
 * DTR is dropped if the hangup on close termio flag is on.
 * -------------------------------------------------------------------
 */
static void 
shutdown (struct dz_serial * info)
{
  unsigned long flags;
  unsigned short tmp;

  AMO_REC("shutdown");

  if (!info->is_initialized) return;

  save_flags (flags);
  cli ();

  dz_stop (info->tty);



  info->cflags &= ~DZ_CREAD;          /* turn off receive enable flag */
 // dz_out (info, DZ_LPR, info->cflags);

/*
  if (info->xmit_buf) {              
    free_page ((unsigned long)info->xmit_buf);
    info->xmit_buf = 0;
  }
*/
/*
  if (!info->tty || (info->tty->termios->c_cflag & HUPCL)) {
    tmp = dz_in (info, DZ_TCR);
    if (tmp & DZ_MODEM_DTR) {
      tmp &= ~DZ_MODEM_DTR;
      dz_out (info, DZ_TCR, tmp);
    }
  }
*/
  if (info->tty) set_bit (TTY_IO_ERROR, &info->tty->flags);

  info->is_initialized = 0;
  restore_flags (flags);
}

/* 
 * -------------------------------------------------------------------
 * change_speed ()
 *
 * set the baud rate.
 * ------------------------------------------------------------------- 
 */
#define BD_BCM63xx_TIMER_CLOCK_INPUT    (FPERIPH)

static void 
change_speed (struct dz_serial * info)
{
	unsigned long flags;
	unsigned cflag;
	int baud;

	if (!info->tty || !info->tty->termios) return;

	save_flags (flags);
	cli ();

	info->cflags = info->line;
	baud = tty_get_baud_rate (info->tty);


	//AMO {start - possibilitiété de modifier la vitesse}
	ULONG clockFreqHz;
	UINT32 tmpVal;
	    /* Dissable channel's receiver and transmitter.                */
	stUart->control &= ~(BRGEN|TXEN|RXEN);
        clockFreqHz = BD_BCM63xx_TIMER_CLOCK_INPUT;
	tmpVal = (clockFreqHz / baud) / 16;
	if( tmpVal & 0x01 )
	      tmpVal /= 2;  //Rounding up, so sub is already accounted for
	else
	     tmpVal = (tmpVal / 2) - 1; // Rounding down so we must sub 1
	stUart->baudword = tmpVal;
        /* Finally, re-enable the transmitter and receiver.            */
	stUart->control |= (BRGEN|TXEN|RXEN);
	PRINT_AMO("AMO: change speed to %d -> req=%d\n",baud, tmpVal);
	//AMO {end}

	restore_flags (flags);
return;


	cflag = info->tty->termios->c_cflag;

	switch (cflag & CSIZE) {
		case CS5: info->cflags |= DZ_CS5; break;
		case CS6: info->cflags |= DZ_CS6; break;
		case CS7: info->cflags |= DZ_CS7; break;
		case CS8: 
		default:  info->cflags |= DZ_CS8;
	}

	if (cflag & CSTOPB) info->cflags |= DZ_CSTOPB;
	if (cflag & PARENB) info->cflags |= DZ_PARENB;
	if (cflag & PARODD) info->cflags |= DZ_PARODD;

	switch (baud) {
		case 50  :  info->cflags |= DZ_B50;   break;
		case 75  :  info->cflags |= DZ_B75;   break;
		case 110 :  info->cflags |= DZ_B110;  break;
		case 134 :  info->cflags |= DZ_B134;  break; 
		case 150 :  info->cflags |= DZ_B150;  break;
		case 300 :  info->cflags |= DZ_B300;  break; 
		case 600 :  info->cflags |= DZ_B600;  break;
		case 1200:  info->cflags |= DZ_B1200; break; 
		case 1800:  info->cflags |= DZ_B1800; break;
		case 2000:  info->cflags |= DZ_B2000; break;
		case 2400:  info->cflags |= DZ_B2400; break;
		case 3600:  info->cflags |= DZ_B3600; break; 
		case 4800:  info->cflags |= DZ_B4800; break;
		case 7200:  info->cflags |= DZ_B7200; break; 
		case 9600: 
		default  :  info->cflags |= DZ_B9600; 
	}

	info->cflags |= DZ_RXENAB;
	dz_out (info, DZ_LPR, info->cflags);


	/* setup accept flag */
	info->read_status_mask = RXOVFERR;
	if (I_INPCK(info->tty))
		info->read_status_mask |= (RXFRAMERR | RXPARERR);

	/* characters to ignore */
	info->ignore_status_mask = 0;
	if (I_IGNPAR(info->tty))
    	info->ignore_status_mask |= (RXFRAMERR | RXPARERR);

}

/* 
 * -------------------------------------------------------------------
 * dz_flush_char ()
 *
 * Flush the buffer.
 * ------------------------------------------------------------------- 
 */
static void 
bcm63xx_cons_flush_chars (struct tty_struct *tty)
{
	struct dz_serial *info = (struct dz_serial *)tty->driver_data;
	unsigned long flags;

	AMO_REC("cons_flush_chars");

	if (info->xmit_cnt <= 0 || tty->stopped || tty->hw_stopped /*|| !info->xmit_buf*/)
	    return;

	save_flags (flags);
	cli ();  

	bcm_start (info->tty);

	restore_flags (flags);
}


/* 
 * -------------------------------------------------------------------
 * bcm63xx_cons_write ()
 *
 * main output routine.
 * ------------------------------------------------------------------- 
 */
static int 
bcm63xx_cons_write (struct tty_struct *tty, int from_user, const unsigned char *buf, int count)
{
	struct dz_serial *info = (struct dz_serial *)tty->driver_data;
	unsigned long flags;
	int c, ret = 0, prev_count;
	AMO_EVAL(int nb_loop=0;)

	//printk("bcm63xx_cons_write\n");
	//for (c = 0; c < count; c++) _putc(buf[c]);
	//return count;

  AMO_REC("cons write %d", count );

  if (!tty )  return ret;
  /*if (!info->xmit_buf) return ret;*/
  if (!tmp_buf) tmp_buf = tmp_buffer;

    prev_count = 0x10000;

  if (info != lines[ DZ_CURRENT_LINE ])
  {
    //on ecrit sur le tty qui n'est pas actif -> presque rien a faire
    PRINT_AMO("cons write on no active line");
    dz_sched_event( info, DZ_EVENT_WRITE_WAKEUP );
    return count;
  }

  PRINT_AMO("cons write %d - xmit_cnt:%d",count, info->xmit_cnt);
  
  if (from_user)
  {
    //printk("Ucount=%d", count );
    down (&tmp_buf_sem);
    while (1)
    {
      
      AMO_EVAL( nb_loop++; )
      c = MIN(count, MIN(DZ_XMIT_SIZE - info->xmit_cnt - 1, DZ_XMIT_SIZE - info->xmit_head));
      if (c <= 0) break;

      c -= copy_from_user (tmp_buf, buf, c);
      if (!c) {
        if (!ret) ret = -EFAULT;
        break;
      }

      save_flags (flags);
      cli ();

      prev_count = MIN( prev_count, info->xmit_cnt );
      c = MIN(c, MIN(DZ_XMIT_SIZE - info->xmit_cnt - 1, DZ_XMIT_SIZE - info->xmit_head));
      memcpy(info->xmit_buf + info->xmit_head, tmp_buf, c);
      info->xmit_head = ((info->xmit_head + c) & (DZ_XMIT_SIZE-1));
      info->xmit_cnt += c;

      restore_flags(flags);

      buf += c;
      count -= c;
      ret += c;
    }
    up (&tmp_buf_sem);
  }
  else
  {
    //printk("K");
    while (count)
    {
      save_flags (flags);
      cli ();
      
      prev_count = MIN( prev_count, info->xmit_cnt );
      c = MIN(count, MIN(DZ_XMIT_SIZE - info->xmit_cnt - 1, DZ_XMIT_SIZE - info->xmit_head));
      if (c <= 0) {
        restore_flags (flags);
        break;
      }
      else
      {
        memcpy (info->xmit_buf + info->xmit_head, buf, c);
        info->xmit_head = ((info->xmit_head + c) & (DZ_XMIT_SIZE-1));
        info->xmit_cnt += c;

        restore_flags (flags);
	AMO_REC("loop %d",c);
        buf += c;
        count -= c;
        ret += c;
      }
    }
  }

  //need to feed the fifo ?

  AMO_REC("prev_count:%d",prev_count);
  if ((ret > 0 ) && (prev_count == 0))
  {
    save_flags (flags);
    cli ();
    AMO_REC("write->transmit_char");
    transmit_chars( info );
    restore_flags(flags);
  }
  if (info->xmit_cnt) {
    if (!tty->stopped) {
      if (!tty->hw_stopped) {
        bcm_start (info->tty);
		    }
    }
  }
  if ((ret == 0) && (count))
    info->wakeup_tty = 1;
  AMO_REC("write ret %d", ret);
  return ret;
}

/*
 * -------------------------------------------------------------------
 * bcm63xx_cons_write_room ()
 *
 * compute the amount of space available for writing.
 * ------------------------------------------------------------------- 
 */
static int
bcm63xx_cons_write_room (struct tty_struct *tty)
{
  struct dz_serial *info = (struct dz_serial *)tty->driver_data;
  int ret;
  
  ret = DZ_XMIT_SIZE - info->xmit_cnt - 1;
  if (ret < 0) ret = 0;

  return ret;

}

/* 
 * -------------------------------------------------------------------
 * dz_chars_in_buffer ()
 *
 * compute the amount of char left to be transmitted
 * ------------------------------------------------------------------- 
 */
static int dz_chars_in_buffer (struct tty_struct *tty)
{
  struct dz_serial *info = (struct dz_serial *)tty->driver_data;
  
  return info->xmit_cnt;
}

/* 
 * -------------------------------------------------------------------
 * dz_flush_buffer ()
 *
 * Empty the output buffer
 * ------------------------------------------------------------------- 
 */
static void dz_flush_buffer (struct tty_struct *tty)
{
  struct dz_serial *info = (struct dz_serial *)tty->driver_data;


  AMO_REC("flush buffer");
  cli ();
  info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
  sti ();

  wake_up_interruptible (&tty->write_wait);

  if ((tty->flags & (1 << TTY_DO_WRITE_WAKEUP)) && tty->ldisc.write_wakeup)
    (tty->ldisc.write_wakeup)(tty);
}

/*
 * ------------------------------------------------------------
 * dz_throttle () and dz_unthrottle ()
 * 
 * This routine is called by the upper-layer tty layer to signal that
 * incoming characters should be throttled (or not).
 * ------------------------------------------------------------
 */
static void 
dz_throttle (struct tty_struct *tty)
{
	struct dz_serial *info = (struct dz_serial *)tty->driver_data;  

  if (info->use_ctsrts)
    UART->prog_out = 0;
	else if (I_IXOFF(tty))
	    info->x_char = STOP_CHAR(tty);
}

static void 
dz_unthrottle (struct tty_struct *tty)
{
	struct dz_serial *info = (struct dz_serial *)tty->driver_data;  

  if (info->use_ctsrts)
    UART->prog_out = 1 << 1;
	if (I_IXOFF(tty)) {
	    if (info->x_char)
    		info->x_char = 0;
		else
			info->x_char = START_CHAR(tty);
	}
}

static void 
dz_send_xchar (struct tty_struct *tty, char ch)
{
	struct dz_serial *info = (struct dz_serial *)tty->driver_data;

	AMO_REC("send xchar");

	info->x_char = ch;

	if (ch) bcm_start (info->tty);
}

/*
 * ------------------------------------------------------------
 * rs_ioctl () and friends
 * ------------------------------------------------------------
 */
static int 
get_serial_info (struct dz_serial * info, struct serial_struct * retinfo)
{
  struct serial_struct tmp;
  
  if (!retinfo)
    return -EFAULT;

  memset (&tmp, 0, sizeof(tmp));

  tmp.type 			= info->type;
  tmp.line 			= info->line;
  tmp.port 			= info->port;
  tmp.irq 			= info->irq;
  tmp.flags 		= info->flags;
  tmp.baud_base 	= (int)info->baud_base;
  tmp.close_delay 	= info->close_delay;
  tmp.closing_wait 	= info->closing_wait;
  
  return copy_to_user (retinfo, &tmp, sizeof(*retinfo));
}

static int 
set_serial_info (struct dz_serial *info, struct serial_struct *new_info)
{
  struct serial_struct new_serial;
  struct dz_serial old_info;
  int retval = 0;

  if (!new_info)
    return -EFAULT;

  copy_from_user (&new_serial, new_info, sizeof(new_serial));
  old_info = *info;

  if (!suser())
    return -EPERM;

  if (info->count > 1)
    return -EBUSY;

  /*
   * OK, past this point, all the error checking has been done.
   * At this point, we start making changes.....
   */
  
  info->baud_base = (Uart *)new_serial.baud_base;
  info->type = new_serial.type;
  info->close_delay = new_serial.close_delay;
  info->closing_wait = new_serial.closing_wait;

  retval = startup (info);
  return retval;
}

/*
 * get_lsr_info - get line status register info
 *
 * Purpose: Let user call ioctl() to get info when the UART physically
 *          is emptied.  On bus types like RS485, the transmitter must
 *          release the bus after transmitting. This must be done when
 *          the transmit shift register is empty, not be done when the
 *          transmit holding register is empty.  This functionality
 *          allows an RS485 driver to be written in user space. 
 */
static int get_lsr_info (struct dz_serial *info, unsigned int *value)
{
  unsigned short status = dz_in (info, DZ_LPR);

  return put_user (status, value);
}

/*
 * This routine sends a break character out the serial port.
 */
static void 
send_break (struct dz_serial *info, int duration)
{
	unsigned long flags;
	unsigned short tmp, mask;

	AMO_REC("send break");

	if (!info->port)
		return;

	mask = 1 << info->line;
	tmp = dz_in (info, DZ_TCR);
	tmp |= mask;

	current->state = TASK_INTERRUPTIBLE;

	save_flags (flags);
	cli();

	dz_out (info, DZ_TCR, tmp);

	schedule_timeout(duration);

	tmp &= ~mask;
	dz_out (info, DZ_TCR, tmp);

	restore_flags (flags);
}

static int 
bcm_ioctl (struct tty_struct * tty, struct file * file, unsigned int cmd, unsigned long arg)
{
	int error;
	struct dz_serial * info = (struct dz_serial *)tty->driver_data;
	int retval;

  if ((cmd != TIOCGSERIAL) && (cmd != TIOCSSERIAL) &&
      (cmd != TIOCSERCONFIG) && (cmd != TIOCSERGWILD)  &&
      (cmd != TIOCSERSWILD) && (cmd != TIOCSERGSTRUCT)) {
    if (tty->flags & (1 << TTY_IO_ERROR))
      return -EIO;
  }

  switch (cmd) {
  case TCSBRK:    /* SVID version: non-zero arg --> no break */
    retval = tty_check_change (tty);
    if (retval)
      return retval;
    tty_wait_until_sent (tty, 0);
    if (!arg)
      send_break (info, HZ/4); /* 1/4 second */
    return 0;

  case TCSBRKP:   /* support for POSIX tcsendbreak() */
    retval = tty_check_change (tty);
    if (retval)
      return retval;
    tty_wait_until_sent (tty, 0);
    send_break (info, arg ? arg*(HZ/10) : HZ/4);
    return 0;

  case TIOCGSOFTCAR:
    error = verify_area (VERIFY_WRITE, (void *)arg, sizeof(long));
    if (error)
      return error;
    put_user (C_CLOCAL(tty) ? 1 : 0, (unsigned long *)arg);
    return 0;

  case TIOCSSOFTCAR:
    error = get_user (arg, (unsigned long *)arg);
    if (error)
      return error;
    tty->termios->c_cflag = ((tty->termios->c_cflag & ~CLOCAL) | (arg ? CLOCAL : 0));
    return 0;

  case TIOCGSERIAL:
    error = verify_area (VERIFY_WRITE, (void *)arg, sizeof(struct serial_struct));
    if (error)
      return error;
    return get_serial_info (info, (struct serial_struct *)arg);

  case TIOCSSERIAL:
    return set_serial_info (info, (struct serial_struct *) arg);

  case TIOCSERGETLSR: /* Get line status register */
    error = verify_area (VERIFY_WRITE, (void *)arg, sizeof(unsigned int));
    if (error)
      return error;
    else
      return get_lsr_info (info, (unsigned int *)arg);

  case TIOCSERGSTRUCT:
    error = verify_area (VERIFY_WRITE, (void *)arg, sizeof(struct dz_serial));
    if (error)
      return error;
    copy_to_user((struct dz_serial *)arg, info, sizeof(struct dz_serial));
    return 0;
    
  default:
    return -ENOIOCTLCMD;
  }

  return 0;
}

#include <linux/proc_fs.h>

#ifdef AMO
static int proc_cons(char *page, char **start, off_t offset, int count, int *eof, void *priv)
{
struct dz_serial * info = lines[ DZ_CURRENT_LINE ];
  char *ptr = page;
  int len=0;
  ptr += sprintf( ptr, "UART->control:%x   UART->config:%x   UART->fifoctl:%x\n", UART->control,  UART->config, UART->fifoctl);
  ptr += sprintf( ptr, "UART->txf_levl:%x   UART->rxf_levl:%x   UART->fifocfg:%x\n", UART->txf_levl,  UART->rxf_levl, UART->fifocfg);
  ptr += sprintf( ptr, "UART->prog_out:%x   UART->DeltaIPEdgeNoSense:%x   UART->DeltaIPConfig_Mask:%x\n", UART->prog_out,  UART->DeltaIPEdgeNoSense, UART->DeltaIPConfig_Mask);
  ptr += sprintf( ptr, "UART->DeltaIP_SyncIP:%x    UART->intMask:%x    UART->intStatus:%x\n\n", UART->DeltaIP_SyncIP, UART->intMask, UART->intStatus );

  ptr += sprintf( ptr, "GPIO->TBusSel:%x   GPIODir:%x    GPIOio:%x   UartCtl:%x\n",GPIO->TBusSel, GPIO->GPIODir, GPIO->GPIOio, GPIO->UartCtl);

  ptr += sprintf( ptr, "tx_byte_count:%d   tx_int_count:%d    transmit_chars_call:%d\n", ser_stat.tx_byte_count, ser_stat.tx_int_count, ser_stat.transmit_chars_call);

  ptr += sprintf( ptr, "wakeup_tty:%d  state:%d\n", ser_stat.wakeup_tty , ser_stat.state);

  ptr += sprintf( ptr, "info->xmit_cnt:%d  info->xmit_head:%d   info->xmit_tail:%d      xmitsize:%d \n", info->xmit_cnt, info->xmit_head,  info->xmit_tail, DZ_XMIT_SIZE);

  int i,p=ser_stat.t_c;
  for (i=0; i < T_N; i++)
  {
  	p = (p-1 + T_N) % T_N;
	ptr += sprintf( ptr, "%s\n", ser_stat.t[p] );
	sprintf( ser_stat.t[p] , "-" );
  }



  len = ptr - page;


  if (len <= count + offset)
    *eof = 1;

  *start = page + offset;
  len -= offset;

  if (len > count)
    len = count;
  if (len < 0)
    len = 0;

  return len;
}
#endif



static void
dz_set_rtscts( struct dz_serial *info, int enabled)
{
  unsigned long flags;

  info->use_ctsrts = 0;
#if 0 
  if (enabled)
  {
    //enable flow control
    //GPIO13 = RTS_n -> out
    //GPIO9 = CTS -> input
    PRINT_AMO("enable RTSCTS\n");
    save_flags( flags);
    cli();
    UART->DeltaIPEdgeNoSense |= 1 << 1;  //detect both edge transition for CTS
    UART->DeltaIPConfig_Mask = 1 << 1;   //detection of CTS generate interrupt
    UART->intMask |= (1 << 0);
    restore_flags(flags);


    GPIO->GPIODir |= (1 << 13);  //configure GPIO13 as output
    GPIO->UartCtl |= (1 << 13) | (1 << 9);  //GPIO 13 & 9 are driven by uart
    UART->prog_out = (UART->prog_out & (1 << 1)) | (1 << 1);
  }
  else
  {
#endif	  
    //disable flow control
    PRINT_AMO("disbale RTSCTS\n");
    save_flags( flags);
    cli();
    UART->DeltaIPConfig_Mask &= ~(1 << 1);   //detection of CTS doesn't generate interrupt
    UART->intMask &= ~(1 << 0);  //no tinterruption for DeltaIP
    restore_flags(flags);
//    GPIO->UartCtl &= ~((1 << 13) | (1 << 9));
//  }
}



static void 
dz_set_termios (struct tty_struct *tty,
			    struct termios *old_termios)
{
  struct dz_serial *info = (struct dz_serial *)tty->driver_data;

  if (tty->termios->c_cflag == old_termios->c_cflag)
    return;

  change_speed (info);

  //AMO {begin - support du controle de flux cts rts}
  dz_set_rtscts(info, tty->termios->c_cflag & CRTSCTS);
/*
  if ((old_termios->c_cflag & CRTSCTS) &&
      !(tty->termios->c_cflag & CRTSCTS)) {
    tty->hw_stopped = 0;
    bcm_start (tty);
  }
*/
}

/*
 * ------------------------------------------------------------
 * bcm63xx_cons_close()
 * 
 * This routine is called when the serial port gets closed.  First, we
 * wait for the last remaining data to be sent.  Then, we turn off
 * the transmit enable and receive enable flags.
 * ------------------------------------------------------------
 */
static void 
bcm63xx_cons_close (struct tty_struct *tty, struct file *filp)
{
  struct dz_serial * info = (struct dz_serial *)tty->driver_data;
  unsigned long flags;

  
  AMO_REC("close");

  if (!info) return;
        
  save_flags (flags); 
  cli();
        
  if (tty_hung_up_p (filp)) {
    restore_flags (flags);
    return;
  }

  if ((tty->count == 1) && (info->count != 1)) {
    /*
     * Uh, oh.  tty->count is 1, which means that the tty
     * structure will be freed.  Info->count should always
     * be one in these conditions.  If it's greater than
     * one, we've got real problems, since it means the
     * serial port won't be shutdown.
     */
    printk("bcm63xx_cons_close: bad serial port count; tty->count is 1, "
                       "info->count is %d\n", info->count);
                info->count = 1;
  }

  if (--info->count < 0) {
    printk("ds_close: bad serial port count for ttys%d: %d\n",
	   info->line, info->count);
    info->count = 0;
  }

  if (info->count) {
    restore_flags (flags);
    return;
  }
  info->flags |= DZ_CLOSING;
  /*
   * Save the termios structure, since this port may have
   * separate termios for callout and dialin.
   */
  if (info->flags & DZ_NORMAL_ACTIVE)
    info->normal_termios = *tty->termios;
  if (info->flags & DZ_CALLOUT_ACTIVE)
    info->callout_termios = *tty->termios;
  /*
   * Now we wait for the transmit buffer to clear; and we notify 
   * the line discipline to only process XON/XOFF characters.
   */
  tty->closing = 1;

  if (info->closing_wait != DZ_CLOSING_WAIT_NONE)
    tty_wait_until_sent (tty, info->closing_wait);

  /*
   * At this point we stop accepting input.  To do this, we
   * disable the receive line status interrupts.
   */

  shutdown (info);

  if (tty->driver.flush_buffer)
    tty->driver.flush_buffer (tty);
  if (tty->ldisc.flush_buffer)
    tty->ldisc.flush_buffer (tty);
  tty->closing = 0;
  info->event = 0;
  info->tty = 0;

  if (tty->ldisc.num != ldiscs[N_TTY].num) {
    if (tty->ldisc.close)
      (tty->ldisc.close)(tty);
    tty->ldisc = ldiscs[N_TTY];
    tty->termios->c_line = N_TTY;
    if (tty->ldisc.open)
      (tty->ldisc.open)(tty);
  }
  if (info->blocked_open) {
    if (info->close_delay) {
      current->state = TASK_INTERRUPTIBLE;
      schedule_timeout(info->close_delay);
    }
    wake_up_interruptible (&info->open_wait);
  }

  info->flags &= ~(DZ_NORMAL_ACTIVE | DZ_CALLOUT_ACTIVE | DZ_CLOSING);
  wake_up_interruptible (&info->close_wait);

  restore_flags (flags);
}

/*
 * dz_hangup () --- called by tty_hangup() when a hangup is signaled.
 */
static void dz_hangup (struct tty_struct *tty)
{
  struct dz_serial *info = (struct dz_serial *)tty->driver_data;

  AMO_REC("hangup");
  
  dz_flush_buffer (tty);
  shutdown (info);
  info->event = 0;
  info->count = 0;
  info->flags &= ~(DZ_NORMAL_ACTIVE | DZ_CALLOUT_ACTIVE);
  info->tty = 0;
  wake_up_interruptible (&info->open_wait);
}

/*
 * ------------------------------------------------------------
 * rs_open() and friends
 * ------------------------------------------------------------
 */
/* LO static */ int 
block_til_ready (struct tty_struct *tty, struct file *filp, struct dz_serial *info)
{
  //DECLARE_WAITQUEUE(wait, current);
  AMO_REC("block_til_ready");
  return 0;
}       

/*
 * This routine is called whenever a serial port is opened.  It
 * enables interrupts for a serial port. It also performs the 
 * serial-specific initialization for the tty structure.
 */
/* LO static */ int 
bcm63xx_cons_open (struct tty_struct * tty, struct file * filp)
{
  struct dz_serial *info;
  int retval, line;
  PRINT_AMO("opened now, generate " __DATE__ "  " __TIME__ "\n");

  AMO_REC("open");

  line = MINOR(tty->device) - tty->driver.minor_start;
  // Make sure we're only opening on of the ports we support
  if ((line < 0) || (line >= BCM_NUM_UARTS))
  	return -ENODEV;
  info = lines[line];

  info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;

  if (info == lines[ DZ_CURRENT_LINE ])
  {
    //setup of current line
    //setup fifos
    stUart->fifocfg = (DZ_TX_FIFO_THRESHOLD << 4) | (DZ_RX_FIFO_THRESHOLD << 0);
    stUart->fifoctl  = 8;  //if chars in rx fifo and threshold is not reached, claim an interrupt after a timer of 8
  
  
    // Clear any pending interrupts
    stUart->intMask  = 0;

    dz_set_rtscts( info,  0 );

    // Enable Interuptions
    stUart->intMask  |= RXINT ; //interruptions pour tous les characters
    BcmHalInterruptEnable (INTERRUPT_ID_UART);
  }
  

  info->count++;
  
  tty->driver_data = info;
  info->tty = tty;
  
  // Start up serial port
  retval = startup (info);
  if (retval)
    return retval;
  
  retval = block_til_ready (tty, filp, info);
  if (retval)
    return retval;
  
  if ((info->count == 1) && (info->flags & DZ_SPLIT_TERMIOS)) {
    if (tty->driver.subtype == SERIAL_TYPE_NORMAL)
      *tty->termios = info->normal_termios;
    else 
    		*tty->termios = info->callout_termios;
    if (info == lines[ DZ_CURRENT_LINE ])
      change_speed (info);
  }
  
  info->session = current->session;
  info->pgrp = current->pgrp;
  
  return 0;
}

/* --------------------------------------------------------------------------
    Name: bcm63xx_init
 Purpose: Initialize our BCM63xx serial driver
-------------------------------------------------------------------------- */
int __init bcm63xx_init(void)
{
	int i, flags;
	struct dz_serial * info;

	/* Setup base handler, and timer table. */
	init_bh (SERIAL_BH, do_serial_bh);

	// Print the driver version information
	printk(VER_STR);
	printk("date: " __DATE__ " time: " __TIME__ " - FPERIPH:%d\n", FPERIPH);

	memset(&serial_driver, 0, sizeof(struct tty_driver));
	serial_driver.magic 			= TTY_DRIVER_MAGIC;
	serial_driver.name 				= "ttyS";
	serial_driver.major 			= TTY_MAJOR;
	serial_driver.minor_start 		= 64;
	serial_driver.num 				= BCM_NUM_UARTS;
	serial_driver.type 				= TTY_DRIVER_TYPE_SERIAL;
	serial_driver.subtype 			= SERIAL_TYPE_NORMAL;
	serial_driver.init_termios 		= tty_std_termios;

	serial_driver.init_termios.c_cflag = B9600 | CS8 | CREAD | HUPCL | CLOCAL;
	serial_driver.flags 			= TTY_DRIVER_REAL_RAW;
	serial_driver.refcount 			= &serial_refcount;
	serial_driver.table 			= serial_table;
	serial_driver.termios 			= serial_termios;
	serial_driver.termios_locked 	= serial_termios_locked;

	serial_driver.open 				= bcm63xx_cons_open;
	serial_driver.close 			= bcm63xx_cons_close;
	serial_driver.write 			= bcm63xx_cons_write;
	serial_driver.flush_chars 		= bcm63xx_cons_flush_chars;
	serial_driver.write_room 		= bcm63xx_cons_write_room;
	serial_driver.chars_in_buffer 	= dz_chars_in_buffer;
	serial_driver.flush_buffer 		= dz_flush_buffer;
	serial_driver.ioctl 			= bcm_ioctl;
	serial_driver.throttle 			= dz_throttle;
	serial_driver.unthrottle 		= dz_unthrottle;
	serial_driver.send_xchar 		= dz_send_xchar;
	serial_driver.set_termios 		= dz_set_termios;
	serial_driver.stop  	 		= dz_stop;
	serial_driver.start 	 		= bcm_start;
	serial_driver.hangup	 		= dz_hangup;

	/*
	* The callout device is just like normal device except for
	* major number and the subtype code.
	*/
	callout_driver 					= serial_driver;
	callout_driver.name    			= "cua";
	callout_driver.major   			= TTYAUX_MAJOR;
	callout_driver.subtype 			= SERIAL_TYPE_CALLOUT;

	if (tty_register_driver (&serial_driver))
	    panic("Couldn't register serial driver\n");
	if (tty_register_driver (&callout_driver))
    	panic("Couldn't register callout driver\n");
	save_flags(flags); cli();
 
	for (i = 0; i < BCM_NUM_UARTS; i++) {
		info = &multi[i]; 
		lines[i] = info;

//		info->magic 				= SERIAL_MAGIC;
		info->port 					= UART_BASE + (i * 0x20);
		info->irq					= (2 - i) + 8;
	    info->line 					= i;
	    info->tty 					= 0;
	    info->close_delay 			= 50;
	    info->closing_wait 			= 3000;
	    info->x_char 				= 0;
	    info->event 				= 0;
	    info->count 				= 0;
	    info->blocked_open 			= 0;
	    info->tqueue.routine 		= do_softint;
	    info->tqueue.data 			= info;
	    info->tqueue_hangup.routine = do_serial_hangup;
	    info->tqueue_hangup.data 	= info;
	    info->callout_termios 		= callout_driver.init_termios;
	    info->normal_termios 		= serial_driver.init_termios;

      info->xmit_cnt = info->xmit_head = info->xmit_tail = 0;
      info->xmit_buf = (unsigned char *) dz_common_tx_buff; //tous utilise le meme buffer

	    init_waitqueue_head(&info->open_wait); 
	    init_waitqueue_head(&info->close_wait); 

	    /* If we are pointing to address zero then punt - not correctly
	       set up in setup.c to handle this. */
	    if (! info->port)
    		return 0;

        BcmHalMapInterrupt((FN_ISR)bcm_interrupt, 0, INTERRUPT_ID_UART);
	}

	/* order matters here... the trick is that flags
       is updated... in request_irq - to immediatedly obliterate
       it is unwise. */
	restore_flags(flags);

#ifdef AMO
  create_proc_read_entry("cons", 0, 0, proc_cons, NULL);
#endif
  create_proc_read_entry("sdram_cas",0,0,proc_read_sdram_cas, NULL);
  struct proc_dir_entry * res ;
  res = create_proc_entry("tty_selector",0,0);
  res->write_proc= tty_selector_proc;
//  res = create_proc_entry("change_fbus_speed",0,0);
//  res->write_proc= change_fbus_speed_proc;
	return 0;
}

#if defined(CONFIG_SERIAL_CONSOLE)
/* --------------------------------------------------------------------------
    Name: bcm_console_print
 Purpose: bcm_console_print is registered for printk.
		  The console_lock must be held when we get here.
-------------------------------------------------------------------------- */
/* LO static */void 
bcm_console_print (struct console * cons, const char * str, unsigned int count)
{
	unsigned int i;

	//_puts(str);
	AMO_REC("console_print");

	for(i=0; i<count; i++, str++) 
	{
		_putc(*str);
		if (*str == 10) {
			_putc(13);
		}
	}
}
#include <linux/delay.h>
/* LO static */int 
bcm_console_wait_key(struct console *co)
{
    return 0;
}

/* LO static */ kdev_t 
bcm_console_device(struct console * c)
{
	kdev_t device;
	device = MKDEV(TTY_MAJOR, 64 + c->index);
	return device;
}

/* LO static */int __init 
bcm_console_setup(struct console * co, char * options)
{
    printk(("bcm_console_setup\n"));
    return 0;
}

static struct console bcm_sercons = {
	name:        "ttyS",
	write:       bcm_console_print,
	device:      bcm_console_device,
	wait_key:    bcm_console_wait_key,
	setup:       bcm_console_setup,
        flags:       CON_PRINTBUFFER, // CON_CONSDEV, CONSOLE_LINE,
	index:       -1, 			 
};

void __init bcm63xx_console_init(void)
{
	register_console(&bcm_sercons);
}
#endif

module_init(bcm63xx_init);

