/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <stdint.h>
#include <platsupport/io.h>

/* Initializes the XADC
 * Returns 0 on success, -1 on error
 */
int xadc_init(ps_io_ops_t* ops);

/* Returns the contents of an XADC register.
 * Only the least significant 6 bits of the
 * address are used (as they define the range
 * of valid addresses).
 */
uint32_t xadc_read_register(uint32_t address);

/* XADC Addresses */
#define XADC_ADDRESS_TEMPERATURE    0x00

