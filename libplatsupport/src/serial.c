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
