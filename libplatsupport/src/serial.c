/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <string.h>
#include <stdlib.h>
#include <platsupport/serial.h>
#include "chardev.h"

ssize_t
uart_write(ps_chardevice_t* d, const void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    const unsigned char* data = (const unsigned char*)vdata;
    int i;
    for (i = 0; i < count; i++) {
        if (uart_putchar(d, data[i]) < 0) {
            return i;
        }
    }
    return count;
}

ssize_t
uart_read(ps_chardevice_t* d, void* vdata, size_t count, chardev_callback_t rcb UNUSED, void* token UNUSED)
{
    char* data;
    int ret;
    int i;
    data = (char*) vdata;
    for (i = 0; i < count; i++) {
        ret = uart_getchar(d);
        if (ret != EOF) {
            data[i] = ret;
        } else {
            return i;
        }
    }
    return count;
}
