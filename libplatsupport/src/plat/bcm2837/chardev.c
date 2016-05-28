/*
 * Copyright 2016, Data61 CSIRO
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(D61_BSD)
 */

/**
 * Contains the definition for all character devices on this platform.
 * Currently this is just a simple patch.
 */

#include "../../chardev.h"
#include "../../common.h"
#include <utils/util.h>

#include "serial.h"

struct ps_chardevice*
ps_cdev_init(enum chardev_id id, const ps_io_ops_t* o, struct ps_chardevice* d) {
    return NULL;
}
