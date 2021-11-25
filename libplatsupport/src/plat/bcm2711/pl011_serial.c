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
#include "pl011_serial.h"

#define DEV_OFFSET(devid) UART##devid##_OFFSET

typedef volatile struct uart_regs_s {
    uint32_t dr;            // 0x00: data register
    uint32_t rsrecr;        // 0x04: receive status/error clear register
    uint64_t unused0[2];    // 0x08
    uint32_t fr;            // 0x18: flag register
    uint32_t unused1;       // 0x1c
    uint32_t ilpr;          // 0x20: not in use
    uint32_t ibrd;          // 0x24: integer baud rate divisor
    uint32_t fbrd;          // 0x28: fractional baud rate divisor
    uint32_t lcrh;          // 0x2c: line control register
    uint32_t cr;            // 0x30: control register
    uint32_t ifls;          // 0x34: interrupt FIFO level select register
    uint32_t imsc;          // 0x38: interrupt mask set clear register
    uint32_t ris;           // 0x3c: raw interrupt status register
    uint32_t mis;           // 0x40: masked interrupt status register
    uint32_t icr;           // 0x44: interrupt clear register
    uint32_t dmacr;         // 0x48: DMA control register
}
uart_regs_t;

/* LCRH register */
#define LCRH_SPS        (1 << 7) // stick parity select
#define LCRH_WLEN_8BIT  (3 << 5) // word length
#define LCRH_WLEN_7BIT  (2 << 5) // word length
#define LCRH_WLEN_6BIT  (1 << 5) // word length
#define LCRH_WLEN_5BIT  (0 << 5) // word length
#define LCRH_FEN        (1 << 4) // enable fifos
#define LCRH_STP2       (1 << 3) // two stop bits select
#define LCRH_EPS        (1 << 2) // Even parity select
#define LCRH_PEN        (1 << 1) // Parity enable
#define LCRH_BRK        (1 << 0) // send break

/* CR register */
#define CR_RXE          (1 << 9) // receive enable
#define CR_TXE          (1 << 8) // transmit enable
#define CR_UARTEN       (1 << 0) // UART enable

/* FR register */
#define FR_TXFE         (1 << 7) // Transmit FIFO empty
#define FR_RXFF         (1 << 6) // Receive FIFO full
#define FR_TXFF         (1 << 5) // Transmit FIFO full
#define FR_RXFE         (1 << 4) // Receive FIFO empty
#define FR_BUSY         (1 << 3) // UART busy

static void pl011_uart_handle_irq(ps_chardevice_t *dev)
{
    // clear interrupts
    ((uart_regs_t *)dev->vaddr)->icr = 0x7f0;
}

static int pl011_uart_cr_configure(ps_chardevice_t *dev)
{
    uint32_t val = ((uart_regs_t *)dev->vaddr)->cr;

    val |= CR_TXE; // transmit enable
    val |= CR_RXE; // receive enable

    ((uart_regs_t *)dev->vaddr)->cr = val;

    return 0;
}

static int pl011_uart_lcrh_configure(ps_chardevice_t *dev)
{
    uint32_t val = ((uart_regs_t *)dev->vaddr)->lcrh;

    val |= LCRH_WLEN_8BIT; // character size is 8bit
    val &= ~LCRH_STP2;     // only one stop bit
    val &= ~LCRH_PEN;      // no parity

    ((uart_regs_t *)dev->vaddr)->lcrh = val;

    return 0;
}

/*
 * Set UART baud rate divisor value
 *
 * More information: https://developer.arm.com/documentation/ddi0183/g/programmers-model/register-descriptions/fractional-baud-rate-register--uartfbrd
 */
static int pl011_uart_baudrate_div_configure(ps_chardevice_t *dev)
{
    // Base UART clock according to https://github.com/raspberrypi/firmware/issues/951
    const uint32_t freq_uart_clk = 48000000;
    const uint32_t baud_rate = 115200;

    double baud_div = (double) freq_uart_clk / (double)(16.0 * baud_rate);
    double frac_div = baud_div - (uint32_t) baud_div;

    // Set IBRD register
    uint32_t val = ((uart_regs_t *)dev->vaddr)->ibrd;
    val |= (uint32_t) baud_div;
    ((uart_regs_t *)dev->vaddr)->ibrd = val;

    // Set FBRD register
    val = ((uart_regs_t *)dev->vaddr)->fbrd;
    val |= (uint32_t)(frac_div * 64.0 + 0.5);
    ((uart_regs_t *)dev->vaddr)->fbrd = val;

    return 0;
}

/*
 * Configure UART registers
 *
 * This configuration process is based on the description in the BCM2711 TRM
 * section 11.5 Register View, more precisely on the information given for the
 * CR register. Furthermore, the LCRH and the baudrate divider registers (IBRD,
 * FBRD) are configured. For all of these registers the UART must be disabled.
 */
static int pl011_uart_configure(ps_chardevice_t *dev)
{
    /* ---------------------------------------------------------------------- */
    /* -------------------------    Disable UART     ------------------------ */
    /* ---------------------------------------------------------------------- */
    // disable UART
    uint32_t val = ((uart_regs_t *)dev->vaddr)->cr;
    val &= ~CR_UARTEN;
    ((uart_regs_t *)dev->vaddr)->cr = val;

    // wait till UART is not busy anymore
    while (((uart_regs_t *)dev->vaddr)->fr & FR_BUSY);

    // disable FIFO
    val = ((uart_regs_t *)dev->vaddr)->lcrh;
    val &= ~LCRH_FEN;
    ((uart_regs_t *)dev->vaddr)->lcrh = val;

    /* ---------------------------------------------------------------------- */
    /* --------------------    Configure UART registers   ------------------- */
    /* ---------------------------------------------------------------------- */
    // Set control register (CR)
    pl011_uart_cr_configure(dev);

    // Set line control register (LCRH)
    pl011_uart_lcrh_configure(dev);

    // Set baudrate divider
    pl011_uart_baudrate_div_configure(dev);

    /* ---------------------------------------------------------------------- */
    /* -------------------------    Enable UART     ------------------------- */
    /* ---------------------------------------------------------------------- */
    // enable FIFO
    val = ((uart_regs_t *)dev->vaddr)->lcrh;
    val |= LCRH_FEN;
    ((uart_regs_t *)dev->vaddr)->lcrh = val;

    // enable UART
    val = ((uart_regs_t *)dev->vaddr)->cr;
    val |= CR_UARTEN;
    ((uart_regs_t *)dev->vaddr)->cr = val;

    // Set Interrupt Mask Set/Clear Register (IMSC)
    ((uart_regs_t *)dev->vaddr)->imsc = 0x10; // activate receive interrupt

    return 0;
}

int pl011_uart_init(
    const struct dev_defn *defn,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev
)
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
    case 0:
        addr_offset = UART0_OFFSET;
        break;
    // case 1: Mini UART
    case 2:
        addr_offset = UART2_OFFSET;
        break;
    case 3:
        addr_offset = UART3_OFFSET;
        break;
    case 4:
        addr_offset = UART4_OFFSET;
        break;
    case 5:
        addr_offset = UART5_OFFSET;
        break;
    default:
        ZF_LOGE("PL011 UART with ID %d does not exist!", defn->id);
        return -1;
        break;
    }

    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *)vaddr + addr_offset; // use real base address
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &pl011_uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;
    dev->flags      = SERIAL_AUTO_CR;

    pl011_uart_configure(dev);

    return 0;
}

int pl011_uart_getchar(ps_chardevice_t *d)
{
    int ch = EOF;

    // only if receive fifo is not empty
    if ((((uart_regs_t *)d->vaddr)->fr & FR_RXFE) == 0) {
        ch = ((uart_regs_t *)d->vaddr)->dr & MASK(8);
    }
    return ch;
}

int pl011_uart_putchar(ps_chardevice_t *d, int c)
{
    // only if transmit fifo is not full
    while ((((uart_regs_t *)d->vaddr)->fr & FR_TXFF) != 0);

    ((uart_regs_t *)d->vaddr)->dr = c;
    if (c == '\n' && (d->flags & SERIAL_AUTO_CR)) {
        uart_putchar(d, '\r');
    }

    return c;
}
