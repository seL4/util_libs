/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

#include <platsupport/chardev.h>
#include <utils/arith.h>

struct dev_defn {
    enum chardev_id id;
    uintptr_t paddr;
    int size;
    const int *irqs;
    int (*init_fn)(const struct dev_defn *defn,
                   const ps_io_ops_t *ops,
                   struct ps_chardevice *dev);
};

static inline void *chardev_map(
    const struct dev_defn *dev,
    const ps_io_ops_t *ops)
{
    return ps_io_map(
               (ps_io_mapper_t *)&ops->io_mapper,
               (uintptr_t)dev->paddr,
               dev->size,
               0, // map uncached
               PS_MEM_NORMAL);
}

int uart_init(
    const struct dev_defn *defn,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev);

int uart_static_init(
    void *vaddr,
    const ps_io_ops_t *ops,
    ps_chardevice_t *dev);

ssize_t uart_write(
    ps_chardevice_t *dev,
    const void *vdata,
    size_t count,
    chardev_callback_t rcb UNUSED,
    void *token UNUSED);

ssize_t uart_read(
    ps_chardevice_t *dev,
    void *vdata,
    size_t count,
    chardev_callback_t rcb UNUSED,
    void *token UNUSED);

int uart_getchar(
    ps_chardevice_t *dev);

int uart_putchar(
    ps_chardevice_t *dev,
    int c);

