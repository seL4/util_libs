/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

#include "../../chardev.h"

int text_vga_init(const struct dev_defn* defn, const ps_io_ops_t* ops, ps_chardevice_t* dev);

