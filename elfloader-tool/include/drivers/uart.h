/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#pragma once

#include <drivers/common.h>

#define dev_get_uart(dev) ((struct elfloader_uart_ops *)(dev->drv->ops))

struct elfloader_uart_ops {
    int (*putc)(struct elfloader_device *dev, unsigned int c);
};

volatile void *uart_get_mmio(void);
void uart_set_out(struct elfloader_device *out);
