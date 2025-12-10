/*
 * Copyright 2022, HENSOLDT Cyber GmbH
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 *
 *
 * QEMU arm-virt emulates a PL011 UART.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/driver/uart_pl011.h>
#include "../../chardev.h"

static pl011_regs_t *get_pl011_regs(ps_chardevice_t *dev)
{
    return (pl011_regs_t *)(dev->vaddr);
}

int uart_getchar(ps_chardevice_t *d)
{
    pl011_regs_t *regs = get_pl011_regs(d);
    return pl011_get_char_or_EOF(regs);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    pl011_regs_t *regs = get_pl011_regs(d);
    bool is_auto_cr = d->flags & SERIAL_AUTO_CR;
    pl011_put_char_blocking_auto_cr(regs, (uint8_t)c, is_auto_cr);
    return c;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    pl011_regs_t *regs = get_pl011_regs(dev);
    pl011_clear_interrupt(regs);
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));

    /* Map device. */
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr;
    dev->read       = &uart_read; /* calls uart_putchar() */
    dev->write      = &uart_write; /* calls uart_getchar() */
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    /* Enable RX and TX interrupt. */
    pl011_regs_t *regs = get_pl011_regs(dev);
    regs->imsc = PL011_IMSC_RXIM | PL011_IMSC_RTIM;

    return 0;
}
