/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <stdlib.h>
#include <platsupport/serial.h>
#include <string.h>

#include "../../chardev.h"

#define IMXUART_DLL             0x000
#define IMXUART_RHR             0x000 /* UXRD */
#define IMXUART_THR             0x040 /* UTXD */
#define IMXUART_UCR1            0x080 /* Control reg */

#define IMXUART_LSR             0x094 /* USR1 -- status reg */
#define IMXUART_LSR_RXFIFIOE    (1<<9)
#define IMXUART_LSR_RXOE        (1<<1)
#define IMXUART_LSR_RXPE        (1<<2)
#define IMXUART_LSR_RXFE        (1<<3)
#define IMXUART_LSR_RXBI        (1<<4)
#define IMXUART_LSR_TXFIFOE     (1<<13)
#define IMXUART_LSR_TXSRE       (1<<6)
#define IMXUART_LSR_RXFIFOSTS   (1<<7)

#define UART_RHR_READY_MASK    (BIT(15))
#define UART_BYTE_MASK           0xFF

#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

int uart_getchar(ps_chardevice_t* d)
{
    int character = -1;
    uint32_t data = 0;

    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_RXFIFIOE) {
        data = *REG_PTR(d->vaddr, IMXUART_RHR);
        if (data & UART_RHR_READY_MASK) {
            character = data & UART_BYTE_MASK;
        }
    }

    return character;
}

static int
internal_uart_tx_busy(void* vaddr)
{
    return !(*REG_PTR(vaddr, IMXUART_LSR) & IMXUART_LSR_TXFIFOE);
}

static int
internal_uart_tx(void* vaddr, int c)
{
    *REG_PTR(vaddr, IMXUART_THR) = c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    void* vaddr = d->vaddr;
    if (internal_uart_tx_busy(vaddr)) {
        return -1;
    }

    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        internal_uart_tx(vaddr, '\r');
        if (internal_uart_tx_busy(vaddr)) {
            return -1;
        }
    }

    internal_uart_tx(vaddr, c);
    return c;
}

static void uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* TODO */
}

int
uart_init(const struct dev_defn* defn,
          const ps_io_ops_t* ops,
          ps_chardevice_t* dev)
{

    void* vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (vaddr == NULL) {
        return -1;
    }
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    /*
     * Enable interrupts for receiver
     */
    *REG_PTR(dev->vaddr, IMXUART_UCR1) |= IMXUART_LSR_RXFIFIOE;
    return 0;
}
