/*
 * Copyright 2016, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#pragma once

#include <utils/attribute.h>

static inline void ALWAYS_INLINE utils_undefined_instruction(void) {
    /* See §A8.8.247 of the ARM ARM. */
    asm volatile ("udf #0");
}
