/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "serial.h"

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

/* Modem Status Interrupt (MSI) bit 3,
 * Line Status Interrupt (LSI) bit 2,
 * and receive data (RXD) bit 0 need
 * to be enabled in order to receive
 * interrupts.
 */
#define MU_IER_ENA_RX_IRQ  (BIT(3) | BIT(2) | BIT(0))

/* This bit is set if the transmit FIFO can accept at least one byte.*/
#define MU_LSR_TXEMPTY   BIT(5)

/* This bit is set if the transmit FIFO is empty and the
 * transmitter is idle. (Finished shifting out the last bit). */
#define MU_LSR_TXIDLE    BIT(6)
#define MU_LSR_RXOVERRUN BIT(1)
#define MU_LSR_DATAREADY BIT(0)

#define MU_CNTL_RXE      BIT(0)
#define MU_CNTL_TXE      BIT(1)

#define MU_LCR_DLAB      BIT(7)
#define MU_LCR_BREAK     BIT(6)
#define MU_LCR_DATASIZE  BIT(0)


static inline mini_uart_regs_t *bcm_uart_get_priv(ps_chardevice_t *dev)
{
    return (mini_uart_regs_t *)(dev->vaddr);
}

static inline void bcm_uart_enable_rx_irq(ps_chardevice_t *dev)
{
    mini_uart_regs_t *r = bcm_uart_get_priv(dev);
    r->mu_ier |= MU_IER_ENA_RX_IRQ;
}

static inline void bcm_uart_disable_rx_irq(ps_chardevice_t *dev)
{
    mini_uart_regs_t *r = bcm_uart_get_priv(dev);
    r->mu_ier &= ~MU_IER_ENA_RX_IRQ;
}

static inline void bcm_uart_enable(ps_chardevice_t *dev)
{
    mini_uart_regs_t *r = bcm_uart_get_priv(dev);
    r->mu_cntl |= (MU_CNTL_RXE | MU_CNTL_TXE);
}

static inline void bcm_uart_disable(ps_chardevice_t *dev)
{
    mini_uart_regs_t *r = bcm_uart_get_priv(dev);
    r->mu_cntl &= ~(MU_CNTL_RXE | MU_CNTL_TXE);
}

static void bcm_uart_handle_irq(ps_chardevice_t *dev)
{
    bcm_uart_enable_rx_irq(dev);
}

static void bcm_uart_configure(ps_chardevice_t *dev)
{
    // Disable RX interrupts
    bcm_uart_disable_rx_irq(dev);

    // Disable RX and TX
    bcm_uart_disable(dev);

    // TODO: line configuration?

    // Enable RX and TX
    bcm_uart_enable(dev);

    // Enable receive interrupts
    bcm_uart_enable_rx_irq(dev);
}

int bcm_uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev)
{

    /* Attempt to map the virtual address, assure this works */
    void *vaddr = chardev_map(defn, ops);
    memset(dev, 0, sizeof(*dev));
    if (!vaddr) {
        return -1;
    }

    /* When mapping the virtual address space, the base addresses need to be
     * 4k byte aligned. Since the real base addresses of the UART peripherals
     * are not 4k byte aligned, we have to add the required offset.
     */
    uint32_t addr_offset = 0;

    switch (defn->id) {
#if defined(CONFIG_PLAT_BCM2711) || defined(CONFIG_PLAT_BCM2837)
    case 1:
        addr_offset = UART1_OFFSET;
        break;
#elif defined (CONFIG_PLAT_BCM2712)
        /* BCM UART is not supported on BCM2712 as it is on the PCIe bus rather
         * than the SoC itself. */
#else
#error "bcm_uart_init: unknown platform"
#endif
    default:
        ZF_LOGE("Mini-UART with ID %d does not exist!", defn->id);
        return -1;
    }

    /* Set up all the device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr + addr_offset; // use real base address
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &bcm_uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    bcm_uart_configure(dev);

    return 0;
}

int bcm_uart_getchar(ps_chardevice_t *d)
{
    mini_uart_regs_t *r = bcm_uart_get_priv(d);
    int ch = EOF;

    if (r->mu_lsr & MU_LSR_DATAREADY) {
        ch = (int)(r->mu_io & MASK(8));
    }

    /*
    while (!(r->mu_lsr & MU_LSR_DATAREADY));
    ch = (int)(r->mu_io & MASK(8));
    */
    return ch;
}

int bcm_uart_putchar(ps_chardevice_t *d, int c)
{
    if ((d->flags & SERIAL_AUTO_CR) && ((char)c == '\n')) {
        bcm_uart_putchar(d, '\r');
    }

    mini_uart_regs_t *r = bcm_uart_get_priv(d);

    // Wait until transmit FIFO has space
    while (!(r->mu_lsr & MU_LSR_TXEMPTY));

    r->mu_io = (c & MASK(8));

    return 0;
}
