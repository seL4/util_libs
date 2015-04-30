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
#include <platsupport/plat/serial.h>

#include "serial.h"
#include <string.h>

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

#define UART_RHR_READY_MASK    (1 << 15)
#define UART_BYTE_MASK           0xFF

#define REG_PTR(base, offset)  ((volatile uint32_t *)((char*)(base) + (offset)))

static int uart_getchar(ps_chardevice_t* d)
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

static int uart_putchar(ps_chardevice_t* d, int c)
{
    if (*REG_PTR(d->vaddr, IMXUART_LSR) & IMXUART_LSR_TXFIFOE) {
        *REG_PTR(d->vaddr, IMXUART_THR) = c;
        if (c == '\n') {
            uart_putchar(d, '\r');
        }
        return c;
    } else {
        return -1;
    }
}


static void uart_handle_irq(ps_chardevice_t* d UNUSED)
{
    /* TODO */
}


static ssize_t
uart_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    const char* data = (const char*)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (uart_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t
uart_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    char* data;
    int ret;
    int i;
    data = (char*)vdata;
    for (i = 0; i < count; i++) {
        ret = uart_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
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

    /*
     * Enable interrupts for receiver
     */
    *REG_PTR(dev->vaddr, IMXUART_UCR1) |= IMXUART_LSR_RXFIFIOE;
    return 0;
}
