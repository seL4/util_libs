/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/attribute.h>

#define UD_INSTRUCTION_SIZE 2

static inline void ALWAYS_INLINE utils_undefined_instruction(void) {
    asm volatile ("ud2");
}
