/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>
#include <platsupport/plat/uart.h>
#include "uart.h"

#define UART_PADDR  UART1_PADDR
#define UART_REG(vaddr, x) ((volatile uint32_t*)(vaddr + (x)))
#define UART_SR1_TRDY  13
#define UART_SR1_RRDY  9
/* CR1 */
#define UART_CR1_RRDYEN 9
#define UART_CR1_UARTEN 0
/* CR2 */
#define UART_CR2_SRST   0
#define UART_CR2_RXEN   1
#define UART_CR2_TXEN   2
#define UART_CR2_ATEN   3
#define UART_CR2_RTSEN  4
#define UART_CR2_WS     5
#define UART_CR2_STPB   6
#define UART_CR2_PROE   7
#define UART_CR2_PREN   8
#define UART_CR2_RTEC   9
#define UART_CR2_ESCEN 11
#define UART_CR2_CTS   12
#define UART_CR2_CTSC  13
#define UART_CR2_IRTS  14
#define UART_CR2_ESCI  15
/* CR3 */
#define UART_CR3_RXDMUXDEL  2
/* FCR */
#define UART_FCR_RFDIV      7

#define UART_FCR_RXTL_MASK 0x1F
  
#define URXD  0x00 /* UART Receiver Register */
#define UTXD  0x40 /* UART Transmitter Register */
#define UCR1  0x80 /* UART Control Register 1 */
#define UCR2  0x84 /* UART Control Register 2 */
#define UCR3  0x88 /* UART Control Register 3 */
#define UCR4  0x8c /* UART Control Register 4 */
#define UFCR  0x90 /* UART FIFO Control Register */
#define USR1  0x94 /* UART Status Register 1 */
#define USR2  0x98 /* UART Status Register 2 */
#define UESC  0x9c /* UART Escape Character Register */
#define UTIM  0xa0 /* UART Escape Timer Register */
#define UBIR  0xa4 /* UART BRM Incremental Register */
#define UBMR  0xa8 /* UART BRM Modulator Register */
#define UBRC  0xac /* UART Baud Rate Counter Register */
#define ONEMS 0xb0 /* UART One Millisecond Register */
#define UTS   0xb4 /* UART Test Register */

#define UART_URXD_READY_MASK (1 << 15)
#define UART_BYTE_MASK       0xFF
#define UART_SR2_TXFIFO_EMPTY 14
#define UART_SR2_RXFIFO_RDR    0


#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

static int uart_getchar(struct ps_chardevice *d) {
    uint32_t reg = 0;
    int character = -1;
    
    if (*REG_PTR(d->vaddr, USR2) & BIT(UART_SR2_RXFIFO_RDR)) {
        reg = *REG_PTR(d->vaddr, URXD);
        
        if (reg & UART_URXD_READY_MASK) {
            character = reg & UART_BYTE_MASK;
        }
    }
    return character;
}

static int uart_putchar(struct ps_chardevice* d, int c) {
    if (*REG_PTR(d->vaddr, USR2) & BIT(UART_SR2_TXFIFO_EMPTY)) {
        if (c == '\n') {
            uart_putchar(d, '\r');
        }
        *REG_PTR(d->vaddr, UTXD) = c;
        return c;
    } else {
        return -1;
    }
}

static int uart_ioctl(struct ps_chardevice *d UNUSED, int param UNUSED, long arg UNUSED) {
    /* TODO (not critical) */
    return 0;
}

static void uart_handle_irq(struct ps_chardevice* d UNUSED, int irq UNUSED) {
    /* TODO */
}

#define REF_CLK 40089600
static long set_baud(struct ps_chardevice* dev, long bps){
    /*
     * BaudRate = RefFreq / (16 * (BMR + 1)/(BIR + 1) )
     * BMR and BIR are 16 bit
     */
    uint32_t bmr, bir;
    bir = 0xf;
    bmr = REF_CLK/bps - 1;
    *REG_PTR(dev->vaddr, UBIR) = bir;
    *REG_PTR(dev->vaddr, UBMR) = bmr;
    return 0;
}

static int
uart_configure(struct ps_chardevice* dev, int size, int parity, int stop) {
    /* Character size */
    uint32_t v;
    v = *REG_PTR(dev->vaddr, UCR2);
    if(size == 8){
        v |= BIT(UART_CR2_WS);
    }else if(size == 7){
        v &= ~BIT(UART_CR2_WS);
    }else{
        return -1;
    }
    /* Stop bits */
    if(stop == 2){
        v |= BIT(UART_CR2_STPB);
    }else if(stop == 1){
        v &= ~BIT(UART_CR2_STPB);
    }else{
        return -1;
    }
    /* Parity */
    if(parity == 0){
        v &= ~BIT(UART_CR2_PREN);
    }else if(parity == -1){
        /* ODD */
        v |= BIT(UART_CR2_PREN);
        v |= BIT(UART_CR2_PROE);
    }else if(parity == 1){
        /* Even */
        v |= BIT(UART_CR2_PREN);
        v &= ~BIT(UART_CR2_PROE);
    }else{
        return -1;
    }
    /* Apply the changes */
    *REG_PTR(dev->vaddr, UCR2) = v;
    return 0;
}

struct ps_chardevice* uart_init(const struct dev_defn* defn,
        const ps_io_ops_t* ops,
        struct ps_chardevice* dev) {

    /* Attempt to map the virtual address, assure this works */
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return NULL;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->getchar    = &uart_getchar;
    dev->putchar    = &uart_putchar;
    dev->ioctl      = &uart_ioctl;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->rxirqcb    = NULL;
    dev->txirqcb    = NULL;
    dev->ioops      = *ops;
    dev->clk = NULL;

    /* Software reset */
    *REG_PTR(dev->vaddr, UCR2) &= ~BIT(UART_CR2_SRST);
    while(!(*REG_PTR(dev->vaddr, UCR2) & BIT(UART_CR2_SRST)));

    /* Initialise the receiver interrupt. */
    *REG_PTR(dev->vaddr, UCR1) &= ~BIT(UART_CR1_RRDYEN);  /* Disable recv interrupt. */
    *REG_PTR(dev->vaddr, UFCR) &= ~UART_FCR_RXTL_MASK; /* Clear the rx trigger level value. */
    *REG_PTR(dev->vaddr, UFCR) |= 0x1; /* Set the rx tigger level to 1. */
    *REG_PTR(dev->vaddr, UCR1) |= BIT(UART_CR1_RRDYEN); /* Enable recv interrupt. */
    *REG_PTR(dev->vaddr, UCR1) |= BIT(UART_CR1_UARTEN); /* Enable The uart. */
    *REG_PTR(dev->vaddr, UCR2) |= BIT(UART_CR2_RXEN);  /* RX enable */
    *REG_PTR(dev->vaddr, UCR2) |= BIT(UART_CR2_TXEN);  /* TX enable */
    *REG_PTR(dev->vaddr, UCR2) |= BIT(UART_CR2_IRTS);  /* Ignore RTS */
    *REG_PTR(dev->vaddr, UCR3) |= BIT(UART_CR3_RXDMUXDEL); /* RX MUX */

    *REG_PTR(dev->vaddr, UFCR) &= ~(0x7 * BIT(UART_FCR_RFDIV));
    *REG_PTR(dev->vaddr, UFCR) |= 0x4 * BIT(UART_FCR_RFDIV);
    uart_configure(dev, 8, 0, 1);
    set_baud(dev, 115200);
    return dev;
}

