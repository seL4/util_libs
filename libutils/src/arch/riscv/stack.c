/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include <autoconf.h>
#include <utils/stack.h>

void *
utils_run_on_stack(void *stack_top, void * (*func)(void *arg), void *arg)
{
    void *ret;
    asm volatile (
        "mv s0, sp\n\t"                /* Save sp - s0 is callee saved so we don't need to save it. */
        "mv sp, %[new_stack]\n\t"      /* Switch to new stack. */
        "mv a0, %[arg]\n\t"            /* Setup argument to func. */
        "jalr %[func]\n\t"
        "mv sp, s0\n\t"
        "mv %[ret], a0\n\t"
        : [ret] "=r" (ret)
        : [new_stack] "r" (stack_top),
        [func] "r" (func),
        [arg] "r" (arg)
        /* caller saved registers */
        : "t0", "t1", "t2", "t3", "t4", "t5", "t6", "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "ra");
    return ret;
}
