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

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "../../chardev.h"

#define UART_WFIFO  0x0
#define UART_RFIFO  0x4
#define UART_STATUS 0xC

#define UART_TX_FULL        BIT(21)
#define UART_RX_EMPTY       BIT(20)

#define REG_PTR(base, off)     ((volatile uint32_t *)((base) + (off)))


int uart_getchar(ps_chardevice_t *d)
{
    while ((*REG_PTR(d->vaddr, UART_STATUS) & UART_RX_EMPTY));
    return *REG_PTR(d->vaddr, UART_RFIFO);
}

static void internal_uart_putchar(void* vaddr, int c)
{
    while ((*REG_PTR(vaddr, UART_STATUS) & UART_TX_FULL) != 0) {
        continue;
    }
    *REG_PTR(vaddr, UART_WFIFO) = c & 0x7f;
}

int uart_putchar(ps_chardevice_t *d, int c)
{
    void* vaddr = d->vaddr;
    /* SERIAL_AUTO_CR: Send '\r' (CR) before every '\n' (LF). */
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        internal_uart_putchar(vaddr, '\r');
    }
    internal_uart_putchar(vaddr, c);

    return c;
}

static void uart_handle_irq(ps_chardevice_t *dev)
{
    /* nothing to do, interrupts are not used */
}

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    char *page_vaddr = chardev_map(defn, ops);
    if (page_vaddr == NULL) {
        return -1;
    }

    void *uart_vaddr;
    switch (defn->id) {
    case UART0:
        uart_vaddr = page_vaddr + UART0_OFFSET;
        break;
    case UART1:
        uart_vaddr = page_vaddr + UART1_OFFSET;
        break;
    case UART2:
        uart_vaddr = page_vaddr + UART2_OFFSET;
        break;
    case UART0_AO:
        uart_vaddr = page_vaddr + UART0_AO_OFFSET;
        break;
    case UART2_AO:
        uart_vaddr = page_vaddr + UART2_AO_OFFSET;
        break;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = uart_vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}
