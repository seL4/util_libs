/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __SRC_CHARDEV_H__
#define __SRC_CHARDEV_H__

#include <platsupport/chardev.h>
#include <utils/arith.h>

struct dev_defn {
    enum chardev_id id;
    uintptr_t paddr;
    int   size;
    const int* irqs;
    struct ps_chardevice* (*init_fn)(
        const struct dev_defn* defn,
        const ps_io_ops_t* ops,
        struct ps_chardevice* dev
    );
};


static inline void*
chardev_map(const struct dev_defn* d, const ps_io_ops_t* o)
{
    return ps_io_map((ps_io_mapper_t*)&o->io_mapper, (uintptr_t) d->paddr, d->size, 0, PS_MEM_NORMAL);
}

#endif /* __SRC_CHARDEV_H__ */
