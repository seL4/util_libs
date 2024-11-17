/*
 * Copyright 2021, Data61, CSIRO (ABN 41 687 119 230)
 * Copyright 2021, Breakaway Consulting Pty. Ltd.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/*
 * A simple implementation of the `serial` interface for the
 * i.MX8X low-power UART.
 *
 * The driver is non-interrupt driven, with polling to ensure
 * FIFO is available.
 *
 * This implementation makes the assumption that bootloader
 * software has correctly configured the UART.
 *
 * Technical Reference:
 *   i.MX 8DualX/8DualXPlus/8QuadXPlus Applications Processor Reference Manual
 *   Revision 0 (IMX8DQXPRM.pdf)
 *   Chapter 16.13 (page 7908)
 *
 * Same LPUART is used by i.MX93.
 */
#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

#define STAT 0x14
#define TRANSMIT_RECEIVE 0x1c

#define STAT_TDRE   (1 << 23) /* Transmit Data Register Empty Flag */
#define STAT_RDRF   (1 << 21) /* Receive Data Register Full Flag   */

#define UART_REG(d, x) ((volatile uint32_t *)((d->vaddr) + (x)))

int uart_getchar(ps_chardevice_t *d)
{
    /* Wait until received. */
    while (!(*UART_REG(d, STAT) & STAT_RDRF)) { }
    return *UART_REG(d, TRANSMIT_RECEIVE);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    /* Wait to be able to transmit. */
    while (!(*UART_REG(d, STAT) & STAT_TDRE)) { }
    *UART_REG(d, TRANSMIT_RECEIVE) = c;
    return 0;
}


static void uart_handle_irq(ps_chardevice_t *d UNUSED)
{
    /* Not needed, UART drive is not interrupt driven. */
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    /* Attempt to map the UART registers into the virtual address space */
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        /* if this is not possible return error immediately */
        return -1;
    }

    /* Initialize the ps_chardevice structure appropriately */
    memset(dev, 0, sizeof * dev);
    dev->id = defn->id;
    dev->vaddr = (void *)vaddr;
    dev->read = &uart_read;
    dev->write = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs = NULL;
    dev->ioops = *ops;
    dev->flags = SERIAL_AUTO_CR;

    /*
     * Note: The UART has been configured and intialized by U-boot;
     * no configuration required.
     */
    return 0;
}
