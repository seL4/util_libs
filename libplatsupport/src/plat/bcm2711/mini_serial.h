/*
 * Copyright (C) 2021, Hensoldt Cyber GmbH
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../../chardev.h"
#include <autoconf.h>
#include <platsupport/gen_config.h>

int mini_uart_init(const struct dev_defn *defn,
                   const ps_io_ops_t *ops,
                   ps_chardevice_t *dev);

int mini_uart_getchar(ps_chardevice_t *d);

int mini_uart_putchar(ps_chardevice_t *d, int c);