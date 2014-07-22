/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef __UTILS_STACK_H
#define __UTILS_STACK_H

/**
 * Switch to a new stack and start running func on it.
 * If func returns, you will be back on the old stack.
 *
 * @param stack_top top of previously allocated stack.
 * @param func to jump to with the new stack.
 * @ret   the return value of func.
 */
int utils_run_on_stack(void *stack_top, int (*func)(void));

#endif /* __UTILS_STACK_H */
