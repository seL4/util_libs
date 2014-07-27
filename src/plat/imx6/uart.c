/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "uart.h"
#include <stdlib.h>
#include <platsupport/plat/uart.h>

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

    /* Initialise the receiver interrupt. */
    *REG_PTR(dev->vaddr, UCR1) &= ~(1 << UART_CR1_RRDYEN);  /* Disable recv interrupt. */
    *REG_PTR(dev->vaddr, UFCR) &= ~UART_FCR_RXTL_MASK; /* Clear the rx trigger level value. */
    *REG_PTR(dev->vaddr, UFCR) |= 0x1; /* Set the rx tigger level to 1. */
    *REG_PTR(dev->vaddr, UCR1) |= (1 << UART_CR1_RRDYEN); /* Enable recv interrupt. */

    return dev;
}

