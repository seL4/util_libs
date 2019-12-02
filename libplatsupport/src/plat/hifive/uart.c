/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#include <autoconf.h>

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>

#include "../../chardev.h"

#define UART_TX_DATA_MASK  0xFF
#define UART_TX_DATA_FULL  BIT(31)

#define UART_RX_DATA_MASK   0xFF
#define UART_RX_DATA_EMPTY  BIT(31)

#define UART_TX_INT_EN     BIT(0)
#define UART_RX_INT_EN     BIT(1)

#define UART_TX_INT_PEND     BIT(0)
#define UART_RX_INT_PEND     BIT(1)
#define UART_BAUD_DIVISOR 4340
struct uart {
    uint32_t txdata;
    uint32_t rxdata;
    uint32_t txctrl;
    uint32_t rxctrl;
    uint32_t ie;
    uint32_t ip;
    uint32_t div;
};
typedef volatile struct uart uart_regs_t;

static inline uart_regs_t*
uart_get_priv(ps_chardevice_t *d)
{
    return (uart_regs_t*)d->vaddr;
}

int uart_getchar(ps_chardevice_t *d)
{
    uart_regs_t* regs = uart_get_priv(d);
    uint32_t reg = regs->rxdata;
    int c = -1;

    if (!(reg & UART_RX_DATA_EMPTY)) {
        c = reg & UART_RX_DATA_MASK;
    }
    return c;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    uart_regs_t* regs = uart_get_priv(d);
    if (!(regs->txdata & UART_TX_DATA_FULL)) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            regs->txdata = '\r' & UART_TX_DATA_MASK;
            while(regs->txdata & UART_TX_DATA_FULL) {}
        }
        regs->txdata = c & UART_TX_DATA_MASK;
        return c;
    } else {
        return -1;
    }
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    // IRQs are cleared when the TX/RX watermark conditions are no longer met
    // so there is nothing to do here.
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    uart_regs_t* regs;
    /* Attempt to map the virtual address, assure this works */
    void* vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    regs = uart_get_priv(dev);

    /*
     * Enable TX and RX and don't set any watermark levels.
     * 0 watermark on RX indicates an IRQ when more than 0 chars in RX buffer
     * O watermark on TX indicates no IRQ (less than 0 chars in TX buffer)
     */
    regs->txctrl = 0x00001;
    regs->rxctrl = 0x00001;
    /* Enable RX IRQs.  We don't enable TX IRQs as we don't expect any. */
    regs->ie = 0x2;

    if (regs->div != UART_BAUD_DIVISOR) {
        ZF_LOGW("Warning: We require a target baud of 115200 and assume an input clk freq 500MHz.");
        ZF_LOGW("Warning: However an incorrect divisor is set: %d, expected %d", regs->div, UART_BAUD_DIVISOR);
    }

    return 0;
}
