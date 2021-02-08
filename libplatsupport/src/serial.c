/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "chardev.h"

ssize_t uart_write(
    ps_chardevice_t *d,
    const void *vdata,
    size_t count,
    chardev_callback_t rcb UNUSED,
    void *token UNUSED)
{
    const unsigned char *data = (const unsigned char *)vdata;
    for (int i = 0; i < count; i++) {
        if (uart_putchar(d, data[i]) < 0) {
            return i;
        }
    }
    return count;
}

ssize_t uart_read(
    ps_chardevice_t *d,
    void *vdata,
    size_t count,
    chardev_callback_t rcb UNUSED,
    void *token UNUSED)
{
    char *data = (char *)vdata;
    for (int i = 0; i < count; i++) {
        int ret = uart_getchar(d);
        if (EOF == ret) {
            return i;
        }
        data[i] = ret;
    }
    return count;
}
