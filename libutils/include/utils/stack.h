/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <utils/arch/stack.h>
#include <utils/arith.h>

#define STACK_CALL_ALIGNMENT_BITS LOG_BASE_2(STACK_CALL_ALIGNMENT)

/**
 * Switch to a new stack and start running func on it.
 * If func returns, you will be back on the old stack.
 *
 * @param stack_top top of previously allocated stack.
 * @param func to jump to with the new stack.
 * @param arg to pass to new function.
 * @ret   the return value of func.
 */
void *utils_run_on_stack(void *stack_top, void *(*func)(void *arg), void *arg);
