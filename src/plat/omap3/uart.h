/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include "../../chardev.h"

struct ps_chardevice* uart_init(const struct dev_defn* defn,
        const ps_io_ops_t* ops,
        struct ps_chardevice* dev);
