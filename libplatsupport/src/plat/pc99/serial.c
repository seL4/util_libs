/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdlib.h>
#include <string.h>
#include <utils/util.h>

#include "../../chardev.h"

/*
 * Port offsets
 * W    - write
 * R    - read
 * RW   - read and write
 * DLAB - Alternate register function bit
 */

#define SERIAL_THR  0 /* Transmitter Holding Buffer (W ) DLAB = 0 */
#define SERIAL_RBR  0 /* Receiver Buffer            (R ) DLAB = 0 */
#define SERIAL_DLL  0 /* Divisor Latch Low Byte     (RW) DLAB = 1 */
#define SERIAL_IER  1 /* Interrupt Enable Register  (RW) DLAB = 0 */
#define SERIAL_DLH  1 /* Divisor Latch High Byte    (RW) DLAB = 1 */
#define SERIAL_IIR  2 /* Interrupt Identification   (R ) */
#define SERIAL_FCR  2 /* FIFO Control Register      (W ) */
#define SERIAL_LCR  3 /* Line Control Register      (RW) */
#define SERIAL_MCR  4 /* Modem Control Register     (RW) */
#define SERIAL_LSR  5 /* Line Status Register       (R ) */
#define SERIAL_MSR  6 /* Modem Status Register      (R ) */
#define SERIAL_SR   7 /* Scratch Register           (RW) */
#define CONSOLE(port, label) ((port) + (SERIAL_##label))
#define SERIAL_DLAB BIT(7)
#define SERIAL_LSR_DATA_READY BIT(0)
#define SERIAL_LSR_TRANSMITTER_EMPTY BIT(5)

int uart_getchar(ps_chardevice_t *device)
{
    uint32_t res;
    uint32_t io_port = (uint32_t)(uintptr_t)device->vaddr;

    /* Check if character is available. */
    int error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &res);
    if (error != 0) {
        return -1;
    }
    if (!(res & SERIAL_LSR_DATA_READY)) {
        return -1;
    }

    /* retrieve character */
    error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, RBR), 1, &res);
    if (error != 0) {
        return -1;
    }

    return (int) res;
}

static int serial_ready(ps_chardevice_t *device)
{
    uint32_t io_port = (uint32_t)(uintptr_t)device->vaddr;
    uint32_t res;
    int error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &res);
    if (error != 0) {
        return 0;
    }
    return res & SERIAL_LSR_TRANSMITTER_EMPTY;
}

int uart_putchar(ps_chardevice_t *device, int c)
{
    uint32_t io_port = (uint32_t)(uintptr_t)device->vaddr;

    /* Check if serial is ready. */
    if (!serial_ready(device)) {
        return -1;
    }

    /* Write out the next character. */
    ps_io_port_out(&device->ioops.io_port_ops, CONSOLE(io_port, THR), 1, c);

    if (c == '\n') {
        /* If we output immediately then odds are the transmit buffer
         * will be full, so we have to wait */
        while (!serial_ready(device));
        uart_putchar(device, '\r');
    }

    return c;
}

static void uart_handle_irq(ps_chardevice_t *device UNUSED)
{
    /* No IRQ handling required here. */
}

int uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev)
{
    memset(dev, 0, sizeof(*dev));
    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void *) defn->paddr; /* Save the IO port base number. */
    dev->read       = &uart_read;
    dev->write      = &uart_write;
    dev->handle_irq = &uart_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

    /* Initialise the device. */
    uint32_t io_port = (uint32_t)(uintptr_t)dev->vaddr;

    /* clear DLAB - Divisor Latch Access Bit */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x00 & ~SERIAL_DLAB) != 0) {
        return -1;
    }

    /* disable generating interrupts */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, IER), 1, 0x00) != 0) {
        return -1;
    }

    /* set DLAB to*/
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x00 | SERIAL_DLAB) != 0) {
        return -1;
    }
    /* set low byte of divisor to 0x01 = 115200 baud */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, DLL), 1, 0x01) != 0) {
        return -1;
    }
    /* set high byte of divisor to 0x00 */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, DLH), 1, 0x00) != 0) {
        return -1;
    }

    /* line control register: set 8 bit, no parity, 1 stop bit; clear DLAB */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x03 & ~SERIAL_DLAB) != 0) {
        return -1;
    }
    /* modem control register: set DTR/RTS/OUT2 */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, MCR), 1, 0x0b) != 0) {
        return -1;
    }

    uint32_t temp;
    /* clear receiver port */
    if (ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, RBR), 1, &temp) != 0) {
        return -1;
    }
    /* clear line status port */
    if (ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &temp) != 0) {
        return -1;
    }
    /* clear modem status port */
    if (ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, MSR), 1, &temp) != 0) {
        return -1;
    }

    /* Enable the receiver interrupt. */
    if (ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, IER), 1, 0x01) != 0) {
        return -1;
    }

    return 0;
}
