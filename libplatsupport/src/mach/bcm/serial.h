/*
 * Copyright 2022, Technology Innovation Institute
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include "../../chardev.h"

int uart_init(const struct dev_defn* defn, const ps_io_ops_t* ops, struct ps_chardevice* dev);

int bcm_uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev);

int bcm_uart_getchar(ps_chardevice_t *dev);

int bcm_uart_putchar(ps_chardevice_t *dev, int c);

int pl011_uart_init(const struct dev_defn *defn, const ps_io_ops_t *ops, ps_chardevice_t *dev);

int pl011_uart_getchar(ps_chardevice_t *dev);

int pl011_uart_putchar(ps_chardevice_t *dev, int c);