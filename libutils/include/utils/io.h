/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
* @TAG(DATA61_BSD)
*/

#pragma once

#define RAW_WRITE8(v,a)	    *((uint8_t*)(a)) = v
#define RAW_WRITE16(v,a)	*((uint16_t*)(a)) = v
#define RAW_WRITE32(v,a)	*((uint32_t*)(a)) = v

#define RAW_READ8(a)		*((uint8_t*)(a))
#define RAW_READ16(a)		*((uint16_t*)(a))
#define RAW_READ32(a)		*((uint32_t*)(a))
