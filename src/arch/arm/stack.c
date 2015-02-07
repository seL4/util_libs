/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <utils/stack.h>

void *
utils_run_on_stack(void *stack_top, void *(*func)(void *arg), void *arg)
{
    void *ret;
    asm volatile (
        "mov r4, sp\n\t"                /* Save sp - r4 is callee saved so we don't need to save it. */
        "mov sp, %[new_stack]\n\t"      /* Switch to new stack. */
        "mov r0, %[arg]\n\t"            /* Setup argument to func. */
        "blx %[func]\n\t"
        "mov sp, r4\n\t"
        "mov %[ret], r0\n\t"
        : [ret] "=r" (ret)
        : [new_stack] "r" (stack_top),
          [func] "r" (func),
          [arg] "r" (arg)
        /* r0 - r3 are caller saved, so they will be clobbered by blx.
         * blx clobbers lr
         * we clobber r4 to save the return value in it */
        : "r0", "r1", "r2", "r3", "r4", "lr");

    return ret;
}
