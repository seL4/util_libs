/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 *# @TAG(DATA61_BSD)
 #*/

#pragma once

/* The compiler will assume, and maintain, the following alignment (in bytes)
 * of the stack pointer when a function is called. When writing assembly, or
 * code that operates on the stack pointer directly (e.g. starting a thread),
 * it's necessary to ensure this alignment when calling a function, so that
 * the compiler's assumptions are correct.
 */
#define STACK_CALL_ALIGNMENT  16
