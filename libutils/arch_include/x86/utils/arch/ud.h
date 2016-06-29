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

#define UD_INSTRUCTION_SIZE 2

static inline void ALWAYS_INLINE utils_undefined_instruction(void) {
    asm volatile ("ud2");
}
