/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

/* The compiler will assume, and maintain, the following alignment (in bytes)
 * of the stack pointer when a function is called. When writing assembly, or
 * code that operates on the stack pointer directly (e.g. starting a thread),
 * it's necessary to ensure this alignment when calling a function, so that
 * the compiler's assumptions are correct.
 */
#define STACK_CALL_ALIGNMENT  16
