/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "serial.h"
#include <string.h>


#define UART_BYTE_MASK  0xff

#define LCR_DLAB        BIT(7)
#define LCR_SET_BREAK   BIT(6)
#define LCR_SET_PARITY  BIT(5)
#define LCR_EVEN        BIT(4)  /* even parity? */
#define LCR_PAR         BIT(3)  /* send parity? */
#define LCR_STOP        BIT(2)  /* transmit 1 (0) or 2 (1) stop bits */
#define LCR_WD_SIZE_5   0
#define LCR_WD_SIZE_6   1
#define LCR_WD_SIZE_7   2
#define LCR_WD_SIZE_8   3

#define LSR_THRE_EMPTY  BIT(5)
#define LSR_RDR_READY   1

#define IER_RHR_ENABLE  1

struct tk1_uart_regs {
    uint32_t    thr_dlab;   /* 0x0: tx holding register                     */
    uint32_t    ier_dlab;   /* 0x4: IER and DLH registers                   */
    uint32_t    iir_fcr;    /* 0x8: FIFO control; interrupt identification  */
    uint32_t    lcr;        /* 0xc: line control                            */ 
    uint32_t    mcr;        /* 0x10: modem control                          */
    uint32_t    lsr;        /* 0x14: line status                            */
    uint32_t    msr;        /* 0x18: modem status                           */
    uint32_t    spr;        /* 0x1c: scratchpad                             */
    uint32_t    csr;        /* 0x20: IrDA pulse coding                      */
    uint32_t    rx_fifo_cfg;/* 0x24:                                        */
    uint32_t    mie;        /* 0x28: modem interrupt enable                 */
    uint32_t    asr;        /* 0x3c: auto sense baud                        */
};
typedef volatile struct tk1_uart_regs tk1_uart_regs_t;

static inline tk1_uart_regs_t*
tk1_uart_get_priv(ps_chardevice_t *d)
{
    return (tk1_uart_regs_t*)d->vaddr;
}


static int uart_getchar(ps_chardevice_t *d)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    uint32_t reg = 0;
    int c = -1;

    if (regs->lsr & LSR_RDR_READY) {
        reg = regs->thr_dlab;
        c = reg & UART_BYTE_MASK;
    }
    return c;
}

static int uart_putchar(ps_chardevice_t* d, int c)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    if (regs->lsr & LSR_THRE_EMPTY) {
        if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
            uart_putchar(d, '\r');
        }
        regs->thr_dlab = c;
        return c;
    } else {
        return -1;
    }
}

static void
uart_handle_irq(ps_chardevice_t* d UNUSED)
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
uart_configure(ps_chardevice_t* d, long bps, int char_size, enum serial_parity parity, int stop_bits)
{
    tk1_uart_regs_t* regs = tk1_uart_get_priv(d);
    /* line control register */
    uint32_t lcr = 0;

    switch (char_size) {
        case 5:
            lcr |= LCR_WD_SIZE_5; 
            break;
        case 6:
            lcr |= LCR_WD_SIZE_6;
            break;
        case 7:
            lcr |= LCR_WD_SIZE_7;
            break;
        case 8:
            lcr |= LCR_WD_SIZE_8;
            break;
        default:
            return -1;
    }

    switch (parity) {
        case PARITY_NONE:
            break;
        case PARITY_EVEN:
            lcr |= LCR_SET_PARITY;
            lcr |= LCR_EVEN;
            break;
        case PARITY_ODD:
            lcr |= LCR_SET_PARITY;
            break;
        default:
            return -1;
    }

    /* one stop bit */

    regs->lcr = lcr;

    return 0;
}

int uart_init(const struct dev_defn* defn,
              const ps_io_ops_t* ops,
              ps_chardevice_t* dev)
{
    static void *vaddr = 0;
    void *uart_vaddr = 0;
    tk1_uart_regs_t* regs;

    /* Attempt to map the virtual address, assure this works */
    if (vaddr == 0) {
        vaddr = chardev_map(defn, ops);

        if (vaddr == NULL) {
            return -1;
        }
    }
    
    /* add offsets properly */
    switch (defn->id) {
        case TK1_UARTA:
            uart_vaddr = vaddr;
            break;
        case TK1_UARTB:
            uart_vaddr = vaddr + UARTB_OFFSET;
            break;
        case TK1_UARTC:
            uart_vaddr = vaddr + UARTC_OFFSET;
            break;
        case TK1_UARTD:
            uart_vaddr = vaddr + UARTD_OFFSET;
            break;
        default:
            return -1;
    }

    memset(dev, 0, sizeof(*dev));

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*)uart_vaddr;
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    regs = tk1_uart_get_priv(dev);

    /* Line configuration */
    uart_configure(dev, 115200, 8, PARITY_NONE, 1);

    /* enable received data interrupt */
    regs->ier_dlab = IER_RHR_ENABLE;

    return 0;
}

