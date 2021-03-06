/*
 * Copyright 2018, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#include <stdlib.h>
#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>
#include "../../chardev.h"
#include <string.h>


int uart_init(
    const struct dev_defn *defn UNUSED,
    const ps_io_ops_t *ops UNUSED,
    ps_chardevice_t *dev UNUSED)
{
    return 0;
}
