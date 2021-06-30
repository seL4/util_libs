/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "serial.h"
#include "mini_serial.h"

typedef volatile struct mini_uart_regs_s {
    uint32_t mu_io;         // 0x40: mini UART I/O Data
    uint32_t mu_ier;        // 0x44: mini UART interrupt enable
    uint32_t mu_iir;        // 0x48: mini UART interrupt identify
    uint32_t mu_lcr;        // 0x4c: mini UART line control
    uint32_t mu_mcr;        // 0x50: mini UART modem control
    uint32_t mu_lsr;        // 0x54: mini UART line status
    uint32_t mu_msr;        // 0x58: mini UART modem status
    uint32_t mu_scratch;    // 0x5c: mini UART scratch
    uint32_t mu_cntl;       // 0x60: mini UART extra control
    uint32_t mu_stat;       // 0x64: mini UART extra status
    uint32_t mu_baud;       // 0x68: mini UART baudrate
}
mini_uart_regs_t;

/* This bit is set if the transmit FIFO can accept at least one byte.*/
#define MU_LSR_TXEMPTY   BIT(5)
/* This bit is set if the transmit FIFO is empty and the
 * transmitter is idle. (Finished shifting out the last bit). */
#define MU_LSR_TXIDLE    BIT(6)
#define MU_LSR_RXOVERRUN BIT(1)
#define MU_LSR_DATAREADY BIT(0)

#define MU_LCR_DLAB      BIT(7)
#define MU_LCR_BREAK     BIT(6)
#define MU_LCR_DATASIZE  BIT(0)

static void mini_uart_handle_irq(ps_chardevice_t *dev)
{
}

int mini_uart_init(const struct dev_defn *defn,
                   const ps_io_ops_t *ops,
                   ps_chardevice_t *dev)
{
    /* Attempt to map the virtual address, assure this works */
    void *vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (vaddr == NULL) {
        return -1;
    }

    // When mapping the virtual address space, the base addresses need to be
    // 4k byte aligned. Since the real base addresses of the UART peripherals
    // are not 4k byte aligned, we have to add the required offset.
    uint32_t addr_offset = 0;
    switch (defn->id) {
    case 1:
        addr_offset = UART1_OFFSET;
        break;
    default:
        ZF_LOGE("Mini UART with ID %d does not exist!", defn->id);
        return -1;
        break;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr + addr_offset; // use real base address
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &mini_uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    return 0;
}

int mini_uart_getchar(ps_chardevice_t *d)
{
    while (!((mini_uart_regs_t *)d->vaddr)->mu_lsr & MU_LSR_DATAREADY);
    return ((mini_uart_regs_t *)d->vaddr)->mu_io;
}

int mini_uart_putchar(ps_chardevice_t *d, int c)
{
    while (!((mini_uart_regs_t *)d->vaddr)->mu_lsr & MU_LSR_TXIDLE);
    ((mini_uart_regs_t *)d->vaddr)->mu_io = (c & 0xff);

    return 0;
}
