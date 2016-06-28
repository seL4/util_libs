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
