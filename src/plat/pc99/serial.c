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
#include <string.h>

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


static int serial_getchar(ps_chardevice_t *device)
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

static int serial_putchar(ps_chardevice_t* device, int c)
{
    uint32_t res;
    uint32_t io_port = (uint32_t) device->vaddr;

    /* Check if serial is ready. */
    res = 0;
    int error = ps_io_port_in(&device->ioops.io_port_ops, CONSOLE(io_port, LSR), 1, &res);
    assert(!error);
    if (!(res & SERIAL_LSR_TRANSMITTER_EMPTY)) {
        return -1;
    }

    /* Write out the next character. */
    ps_io_port_out(&device->ioops.io_port_ops, CONSOLE(io_port, THR), 1, c);

    if (c == '\n') {
        serial_putchar(device, '\r');
    }

    return c;
}

static ssize_t
serial_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    const unsigned char* data = (const unsigned char*)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (serial_putchar(d, *data++) < 0) {
            return i;
        }
    }
    return count;
}

static ssize_t
serial_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    char* data;
    int ret;
    int i;
    data = (char*)vdata;
    for (i = 0; i < count; i++) {
        ret = serial_getchar(d);
        if (ret != EOF) {
            *data++ = ret;
        } else {
            return i;
        }
    }
    return count;
}

static void serial_handle_irq(ps_chardevice_t* device UNUSED)
{
    /* No IRQ handling required here. */
}

int
serial_init(const struct dev_defn* defn, const ps_io_ops_t* ops, ps_chardevice_t* dev)
{
    memset(dev, 0, sizeof(*dev));
    /* Set up all the  device properties. */
    dev->id         = defn->id;
    dev->vaddr      = (void*) defn->paddr; /* Save the IO port base number. */
    dev->read       = &serial_read;
    dev->write      = &serial_write;
    dev->handle_irq = &serial_handle_irq;
    dev->irqs       = defn->irqs;
    dev->ioops      = *ops;

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

    return 0;
}
