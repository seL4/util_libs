/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include <string.h>
#include <stdio.h>
#include "../../chardev.h"

#define AXI_UARTLITE_CR_RST_TX_FIFO     BIT(0)
#define AXI_UARTLITE_CR_RST_RX_FIFO     BIT(1)
#define AXI_UARTLITE_CR_ENABLE_INTR     BIT(4)

#define AXI_UARTLITE_SR_RX_FIFO_VALID   BIT(0)
#define AXI_UARTLITE_SR_RX_FIFO_FULL    BIT(1)
#define AXI_UARTLITE_SR_TX_FIFO_EMPTY   BIT(2)
#define AXI_UARTLITE_SR_TX_FIFO_FULL    BIT(3)
#define AXI_UARTLITE_SR_INTR_ENABLED    BIT(4)
#define AXI_UARTLITE_SR_OVERRUN_ERROR   BIT(5)
#define AXI_UARTLITE_SR_FRAME_ERROR     BIT(6)
#define AXI_UARTLITE_SR_PARITY_ERROR    BIT(7)

struct zynq_axi_uartlite_regs {
    uint32_t rx_fifo;   /* 0x0 Receive FIFO */
    uint32_t tx_fifo;   /* 0x4 Transmit FIFO */
    uint32_t sr;        /* 0x8 Status Register */
    uint32_t cr;        /* 0xC Control Register */
};
typedef volatile struct zynq_axi_uartlite_regs zynq_axi_uartlite_regs_t;

static inline zynq_axi_uartlite_regs_t *zynq_axi_uartlite_get_priv(ps_chardevice_t *d)
{
    return (zynq_axi_uartlite_regs_t *)d->vaddr;
}

static int axi_uartlite_getchar(ps_chardevice_t *d)
{
    zynq_axi_uartlite_regs_t *regs =
        zynq_axi_uartlite_get_priv(d);

    int c = -1;

    /* check if there is at least one byte in the fifo */
    if (regs->sr & AXI_UARTLITE_SR_RX_FIFO_VALID) {
        c = regs->rx_fifo;
    }

    return c;
}

static int axi_uartlite_putchar(ps_chardevice_t *d, int c)
{

    static int needs_newline = 0;

    zynq_axi_uartlite_regs_t *regs =
        zynq_axi_uartlite_get_priv(d);

    /* check if fifo is full */
    if (regs->sr & AXI_UARTLITE_SR_TX_FIFO_FULL) {
        return -1;
    } else {
        if (needs_newline) {
            /* if the last putchar was a '\n' and the fifo filled after
             * only the '\r' was sent, send the remaining '\n' here */
            regs->tx_fifo = '\n';
            needs_newline = 0;
            if (regs->sr & AXI_UARTLITE_SR_TX_FIFO_FULL) {
                return -1;
            }
        }
        if (c == '\n') {
            regs->tx_fifo = '\r';
            /* the fifo may have filled after sending the '\r' */
            if (regs->sr & AXI_UARTLITE_SR_TX_FIFO_FULL) {
                needs_newline = 1;
                /* even if the '\n' didn't get sent on this call, still
                 * return '\n', as it will still eventually be sent */
            } else {
                regs->tx_fifo = '\n';
            }
        } else {
            regs->tx_fifo = c;
        }
        return c;
    }
}

static ssize_t axi_uartlite_write(ps_chardevice_t *d, const void *vdata,
                                  size_t count, chardev_callback_t rcb UNUSED,
                                  void *token UNUSED)
{
    const char *data = (const char *)vdata;
    for (int i = 0; i < count; i++) {
        if (axi_uartlite_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t axi_uartlite_read(ps_chardevice_t *d, void *vdata,
                                 size_t count, chardev_callback_t rcb UNUSED,
                                 void *token UNUSED)
{
    char *data = (char *)vdata;
    for (int i = 0; i < count; i++) {
        int ch = axi_uartlite_getchar(d);
        if (ch != EOF) {
            *data++ = ch;
        } else {
            return i;
        }
    }
    return count;
}

int axi_uartlite_init(void *vaddr, ps_chardevice_t *dev)
{

    memset(dev, 0, sizeof(*dev));

    dev->vaddr = vaddr;
    dev->read  = &axi_uartlite_read;
    dev->write = &axi_uartlite_write;

    zynq_axi_uartlite_regs_t *regs = zynq_axi_uartlite_get_priv(dev);

    // clear the fifos
    regs->cr |= (AXI_UARTLITE_CR_RST_TX_FIFO | AXI_UARTLITE_CR_RST_RX_FIFO);

    // disable interrupts
    regs->cr &= ~AXI_UARTLITE_CR_ENABLE_INTR;

    return 0;
}

int axi_uartlite_init_defn(const struct dev_defn *defn,
                           const ps_io_ops_t *ops,
                           ps_chardevice_t *dev)
{

    void *vaddr = chardev_map(defn, ops);
    if (vaddr == NULL) {
        return -1;
    }

    axi_uartlite_init(vaddr, dev);

    dev->id    = defn->id;
    dev->irqs  = defn->irqs;
    dev->ioops = *ops;

    return 0;
}
