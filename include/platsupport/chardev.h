/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_CHARDEV_H__
#define __PLATSUPPORT_CHARDEV_H__

#include <platsupport/io.h>
#include <platsupport/plat/chardev.h>

struct ps_chardevice;
struct ps_clk;

typedef void (*rxirqcb_t)(const struct ps_chardevice* device, int val);
typedef int  (*txirqcb_t)(const struct ps_chardevice* device);


struct ps_chardevice {
    /* identifier for the device */
    enum chardev_id id;
    void* vaddr;
    /* Character operations for this device */
    int     (*getchar)(struct ps_chardevice* device);
    int     (*putchar)(struct ps_chardevice* device, int c);
    /* TODO be removed */
    int       (*ioctl)(struct ps_chardevice* device, int param, long arg);
    void (*handle_irq)(struct ps_chardevice* device, int irq);
    /* array of irqs associated with this device */
    const int *irqs;
    /* IRQ handlers for this device */
    rxirqcb_t rxirqcb;
    txirqcb_t txirqcb;
    /* Input clock for this device */
    struct ps_clk* clk;
    /* OS specific memory operations */
    ps_io_ops_t ioops;
};
typedef struct ps_chardevice ps_chardevice_t;

/*
 * Initialiase a device
 * @param  id: the id of the character device
 * @param ops: a structure containing OS specific operations for memory access
 * @param dev: a character device structure to populate
 * @return   : NULL on error, otherwise returns the device structure pointer
 */
struct ps_chardevice* ps_cdev_init(enum chardev_id id,
        const ps_io_ops_t* ops,
        struct ps_chardevice* dev);

/*
 * Create a pseudo device: initialise with nop function pointers
 * @param o: a structure containing OS specific operations for memory access
 * @param d: a character device structure to populate
 * @return   : NULL on error, otherwise returns the device structure pointer
 */
struct ps_chardevice* ps_cdev_new(const ps_io_ops_t* o,
        struct ps_chardevice* d);

/***************************************
 *** inline character device helpers ***
 ***************************************/
static inline void ps_cdev_putchar(struct ps_chardevice* d, int c)
{
    int ret;
    do {
        ret = d->putchar(d, c);
    } while (ret != c);
}

static inline int ps_cdev_getchar(struct ps_chardevice* d)
{
    return d->getchar(d);
}

static inline int ps_cdev_ioctl(struct ps_chardevice* d, int param, long arg)
{
    return d->ioctl(d, param, arg);
}

static inline void ps_cdev_handle_irq(struct ps_chardevice* d, int irq)
{
    d->handle_irq(d, irq);
}

static inline int ps_cdev_produces_irq(const struct ps_chardevice* d, int irq)
{
    int i;
    for (i = 0; d->irqs[i] != -1; i++) {
        if (d->irqs[i] == irq) {
            return 1;
        }
    }
    return 0;
}

#endif /* __PLATSUPPORT_CHARDEV_H__ */
