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

#include <utils/attribute.h>

#define UD_INSTRUCTION_SIZE 4

static inline void ALWAYS_INLINE utils_undefined_instruction(void) {
    /* See §A8.8.247 of the ARM ARM. */
    asm volatile ("udf #0");
}
