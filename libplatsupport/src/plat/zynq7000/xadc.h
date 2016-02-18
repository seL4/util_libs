/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLATSUPPORT_PLAT_ZYNQ7000_XADC_H
#define __PLATSUPPORT_PLAT_ZYNQ7000_XADC_H

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

#endif
