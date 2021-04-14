/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define RAW_WRITE8(v,a)     *((uint8_t*)(a)) = v
#define RAW_WRITE16(v,a)    *((uint16_t*)(a)) = v
#define RAW_WRITE32(v,a)    *((uint32_t*)(a)) = v

#define RAW_READ8(a)        *((uint8_t*)(a))
#define RAW_READ16(a)       *((uint16_t*)(a))
#define RAW_READ32(a)       *((uint32_t*)(a))
