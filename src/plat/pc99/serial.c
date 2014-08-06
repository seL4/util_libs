/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "serial.h"
#include <stdlib.h>
#include <sel4/sel4.h>

static int serial_getchar(struct ps_chardevice *device)
{
    uint32_t res;
    uint32_t io_port = (uint32_t) device->vaddr;

    /* Check if character is available. */
    int error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &res);
    assert(!error);
    if (!(res & SERIAL_LSR_DATA_READY)) {
        return -1;
    }

    /* retrieve character */
    error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, RBR), 1, &res);
    assert(!error);

    return (int) res;
}

static int serial_putchar(struct ps_chardevice* device, int c)
{
    uint32_t res;
    uint32_t io_port = (uint32_t) device->vaddr;

    /* Wait for serial to become ready. */
    do {
        res = 0;
        int error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &res);
        assert(!error);
    } while (!(res & SERIAL_LSR_TRANSMITTER_EMPTY));

    /* Write out the next character. */
    ps_io_port_out(&device->ioops.io_port_ops, CONSOLE(io_port, THR), 1, c);

    if(c == '\n') {
        serial_putchar(device, '\r');
    }

    return c;
}

static int serial_ioctl(struct ps_chardevice *device UNUSED, int param UNUSED, long arg UNUSED)
{
    /* TODO (not critical) */
    assert(!"Not implemented");
    return 0;
}

static void serial_handle_irq(struct ps_chardevice* device UNUSED, int irq UNUSED)
{
    /* No IRQ handling required here. */
}

struct ps_chardevice*
serial_init(const struct dev_defn* defn, const ps_io_ops_t* ops, struct ps_chardevice* dev)
{
    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*) defn->paddr; /* Save the IO port base number. */
    dev->getchar    = &serial_getchar;
    dev->putchar    = &serial_putchar;
    dev->ioctl      = &serial_ioctl;
    dev->handle_irq = &serial_handle_irq;
    dev->irqs       = defn->irqs;
    dev->rxirqcb    = NULL;
    dev->txirqcb    = NULL;
    dev->ioops      = *ops;
    dev->clk = NULL;

    /* Initialise the device. */
    uint32_t io_port = (uint32_t) dev->vaddr;

    /* clear DLAB - Divisor Latch Access Bit */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x00 & ~SERIAL_DLAB);
    /* disable generating interrupts */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, IER), 1, 0x00);

    /* set DLAB to*/
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x00 | SERIAL_DLAB);
    /* set low byte of divisor to 0x01 = 115200 baud */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, DLL), 1, 0x01);
    /* set high byte of divisor to 0x00 */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, DLH), 1, 0x00);

    /* line control register: set 8 bit, no parity, 1 stop bit; clear DLAB */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, LCR), 1, 0x03 & ~SERIAL_DLAB);
    /* modem control register: set DTR/RTS/OUT2 */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, MCR), 1, 0x0b);

    uint32_t temp;
    /* clear receiver port */
    ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, RBR), 1, &temp);
    /* clear line status port */
    ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &temp);
    /* clear modem status port */
    ps_io_port_in(&dev->ioops.io_port_ops, CONSOLE(io_port, MSR), 1, &temp);

    /* Enable the receiver interrupt. */
    ps_io_port_out(&dev->ioops.io_port_ops, CONSOLE(io_port, IER), 1, 0x01);

    return dev;
}