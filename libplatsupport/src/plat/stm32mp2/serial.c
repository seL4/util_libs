/*
 * Copyright 2026, STMicroelectronics
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))

#define USART_ISR		0x1C
#define USART_RDR		0x24
#define USART_TDR		0x28
#define USART_ISR_RXNE		BIT(5)
#define USART_ISR_TXE		BIT(7)

int uart_getchar(ps_chardevice_t *d)
{
    int ch = EOF;

    while ((*REG_PTR(d->vaddr, USART_ISR) & USART_ISR_RXNE) == 0);

    ch = *REG_PTR(d->vaddr, USART_RDR) & 0x7f;

    return ch;
}

int uart_putchar(ps_chardevice_t* d, int c)
{
    while ((*REG_PTR(d->vaddr, USART_ISR) & USART_ISR_TXE) == 0);

    *REG_PTR(d->vaddr, USART_TDR) = c;
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    return c;
}

static void
uart_handle_irq(ps_chardevice_t* dev)
{

}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    char *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}

