/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "../../chardev.h"

#define UART_THR 0x00 /* UART Transmit Holding Register */
#define UART_IER 0x04 /* UART Interrupt Enable Register */
#define UART_IER_ERDAI BIT(0) /* Enable Received Data Available Interrupt */
#define UART_LSR 0x14 /* UART Line Status Register */
#define UART_LSR_THRE 0x20 /* Transmit Holding Register Empty */

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

int uart_getchar(ps_chardevice_t *d)
{
    while ((*REG_PTR(d->vaddr, UART_LSR) & BIT(0)));
    return *REG_PTR(d->vaddr, UART_THR);
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    while ((*REG_PTR(d->vaddr, UART_LSR) & UART_LSR_THRE) == 0);

    /* Add character to the buffer. */
    *REG_PTR(d->vaddr, UART_THR) = c;

    return c;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    /*
     * This is currently only called when received data is available on the device.
     * The interrupt will be acked to the device on the next read to the device.
     * There's nothing else we need to do here.
     */
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        ZF_LOGE("Unable to map chardev");
        return -1;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    *REG_PTR(dev->vaddr, 0x8) = 1;
    *REG_PTR(dev->vaddr, UART_IER) = UART_IER_ERDAI;

    return 0;
}
