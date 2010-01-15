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

#ifndef __6345_INTR_H
#define __6345_INTR_H

#ifdef __cplusplus
    extern "C" {
#endif

/*=====================================================================*/
/* BCM6345 External Interrupt Level Assignments                       */
/*=====================================================================*/
#define INTERRUPT_ID_EXTERNAL_0         3
#define INTERRUPT_ID_EXTERNAL_1         4
#define INTERRUPT_ID_EXTERNAL_2         5
#define INTERRUPT_ID_EXTERNAL_3         6

/*=====================================================================*/
/* BCM6345 Timer Interrupt Level Assignments                          */
/*=====================================================================*/
#define MIPS_TIMER_INT                  7

/*=====================================================================*/
/* Peripheral ISR Table Offset                                              */
/*=====================================================================*/
#define INTERNAL_ISR_TABLE_OFFSET       8
#define DMA_ISR_TABLE_OFFSET            (INTERNAL_ISR_TABLE_OFFSET + 13)

/*=====================================================================*/
/* Logical Peripheral Interrupt IDs                                    */
/*=====================================================================*/

/* Internal peripheral interrupt IDs */
#define INTERRUPT_ID_TIMER              (INTERNAL_ISR_TABLE_OFFSET +  0)
#define INTERRUPT_ID_UART               (INTERNAL_ISR_TABLE_OFFSET +  2)
#define INTERRUPT_ID_ADSL               (INTERNAL_ISR_TABLE_OFFSET +  3)
#define INTERRUPT_ID_ATM                (INTERNAL_ISR_TABLE_OFFSET +  4)
#define INTERRUPT_ID_USB                (INTERNAL_ISR_TABLE_OFFSET +  5)
#define INTERRUPT_ID_EMAC               (INTERNAL_ISR_TABLE_OFFSET +  8)
#define INTERRUPT_ID_EPHY               (INTERNAL_ISR_TABLE_OFFSET +  12)

/* DMA channel interrupt IDs */        
#define INTERRUPT_ID_EMAC_RX_CHAN       (DMA_ISR_TABLE_OFFSET + EMAC_RX_CHAN)
#define INTERRUPT_ID_EMAC_TX_CHAN       (DMA_ISR_TABLE_OFFSET + EMAC_TX_CHAN)
#define INTERRUPT_ID_EBI_RX_CHAN        (DMA_ISR_TABLE_OFFSET + EBI_RX_CHAN)
#define INTERRUPT_ID_EBI_TX_CHAN        (DMA_ISR_TABLE_OFFSET + EBI_TX_CHAN)
#define INTERRUPT_ID_RESERVED_RX_CHAN   (DMA_ISR_TABLE_OFFSET + RESERVED_RX_CHAN)
#define INTERRUPT_ID_RESERVED_TX_CHAN   (DMA_ISR_TABLE_OFFSET + RESERVED_TX_CHAN)
#define INTERRUPT_ID_USB_BULK_RX_CHAN   (DMA_ISR_TABLE_OFFSET + USB_BULK_RX_CHAN)
#define INTERRUPT_ID_USB_BULK_TX_CHAN   (DMA_ISR_TABLE_OFFSET + USB_BULK_TX_CHAN)
#define INTERRUPT_ID_USB_CNTL_RX_CHAN   (DMA_ISR_TABLE_OFFSET + USB_CNTL_RX_CHAN)
#define INTERRUPT_ID_USB_CNTL_TX_CHAN   (DMA_ISR_TABLE_OFFSET + USB_CNTL_TX_CHAN)
#define INTERRUPT_ID_USB_ISO_RX_CHAN    (DMA_ISR_TABLE_OFFSET + USB_ISO_RX_CHAN)
#define INTERRUPT_ID_USB_ISO_TX_CHAN    (DMA_ISR_TABLE_OFFSET + USB_ISO_TX_CHAN)

/* defines */
struct pt_regs;
typedef unsigned int (*FN_ISR) (unsigned int);
typedef void (*FN_HANDLER) (int, void *, struct pt_regs *);

/* prototypes */
extern void enable_brcm_irq(unsigned int irq);
extern void disable_brcm_irq(unsigned int irq);
extern int request_external_irq(unsigned int irq,
    void (*handler)(int, void *, struct pt_regs *), unsigned long irqflags, 
    const char * devname, void *dev_id);
extern unsigned int BcmHalMapInterrupt(FN_ISR isr, unsigned int param,
    unsigned int interruptId);
extern void dump_intr_regs(void);

/* compatibility definitions */
#define BcmHalInterruptEnable(irq)      enable_brcm_irq( irq )
#define BcmHalInterruptDisable(irq)     disable_brcm_irq( irq )

#ifdef __cplusplus
    }
#endif                    

#endif  /* __BCM6345_H */

