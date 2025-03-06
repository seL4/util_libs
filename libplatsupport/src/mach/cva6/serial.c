/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/serial.h>
#include <platsupport/plat/serial.h>

#include "../../chardev.h"

int uart_init(const struct dev_defn *defn,
              const ps_io_ops_t *ops,
              ps_chardevice_t *dev)
{
    return 0;
}
