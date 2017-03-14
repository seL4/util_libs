/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#include <autoconf.h>
#include <utils/stack.h>
#include <utils/arith.h>
#include <utils/util.h>

void *
utils_run_on_stack(void *stack_top, void * (*func)(void *arg), void *arg)
{
    if (stack_top == NULL) {
        ZF_LOGE("Invalid stack address \n");
        return NULL;
    } else if (!IS_ALIGNED((uintptr_t) stack_top, 4)) {
      /* Stack has to be aligned to 16-bytes */
        ZF_LOGE("Invalid stack alignment. Stack has to be 16-bytes aligned \n");
        return NULL;
    }

/*
 * Note that x86-64 ABI requires that at least SSE and
 * SSE2 are supported, and thus the API uses xmm registers
 * for passing floating point parameters. The GCC compiler
 * may use the movaps instruction to store the context
 * xmm regsiters on stack, and these store operations
 * require that the stack addresses are 16-byte
 * aligned, so we push oen more dummy number to the stack
 * for alignment purpose.
 */
#ifdef CONFIG_ARCH_X86_64
    void *ret;
    asm volatile (
        "movq   %%rsp, %%rcx\n\t"
        "movq   %[new_stack], %%rsp\n\t"
        "push   %%rcx\n\t"
        "pushq  $0\n\t"             /* dummy number for stack alignment */
        "movq   %[arg], %%rdi\n\t"
        "call   *%[func]\n\t"
        "add    $0x8, %%rsp\n\t"
        "pop    %%rsp\n\t"
        : [ret] "=a" (ret)
        : [new_stack] "r" (stack_top),
        [func] "r" (func),
        [arg] "r" (arg)
        : "rcx", "rbx", "rdi"
    );
#else
    void *ret;
    asm volatile (
        "mov %%esp, %%ecx\n\t"          /* Save sp. */
        "mov %[new_stack], %%esp\n\t"   /* Switch to new stack. */
        "pushl %%ecx\n\t"               /* Push old sp onto new stack. */
        "subl $8, %%esp\n\t"            /* dummy padding for 16-byte stack alignment */
        "pushl %[arg]\n\t"              /* Setup argument to func. */
        "call *%[func]\n\t"
        "add $12, %%esp\n\t"
        "popl %%esp\n\t"                /* Switch back! */
        : [ret] "=a" (ret)
        : [new_stack] "r" (stack_top),
        [func] "r" (func),
        [arg] "r" (arg)
        : "ecx", "ebx");
#endif /* CONFIG_ARCH_X86_64 */
    return ret;
}
