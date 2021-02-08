/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/attribute.h>

/** Execute an undefined machine instruction.
 *
 * This function is designed to be used as a last resort for error handling, when the usual options
 * are not possible (e.g. serial output is unavailable). It is guaranteed to generate an exception
 * during instruction decoding, that can be propagated to a monitor (a fault handler or the kernel).
 * You can think of it as forcing a `SIGILL`.
 *
 * It is forced inline, so you can call it even when you don't have a stack. Though morally it
 * doesn't return, it is not marked as such so the compiler will not remove following code. It is
 * possible that the caller's monitor may want to handle the fault by bumping the instruction
 * pointer to ignore the error and continue.
 *
 * At first glance, it might seem as if `__builtin_trap` already serves this purpose. However, its
 * semantics are vague enough to make it near-useless in a portable embedded context. E.g. on x86
 * it emits an instruction that generates an exception, but on ARM it calls `abort`.
 */
static inline void utils_undefined_instruction(void) ALWAYS_INLINE;

#include <utils/arch/ud.h>
