/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */

#include <autoconf.h>
#include <assembler.h>
#include <armv/assembler.h>

.extern main
.extern non_boot_main

.section ".text.start"

BEGIN_FUNC(_start)

    /* Disable Async aborts that might be pending from bootloader */
    cpsid   ifa

#if CONFIG_MAX_NUM_NODES > 1
    /* Enable SMP */
    mrc     ACTLR(r0)
    orr     r0, r0, #(1 << 6)       /* enable SMP bit   */
#ifdef CONFIG_ARM_CORTEX_A9
    orr     r0, r0, #1              /* enable FW bit    */
#endif
    mcr     ACTLR(r0)
#endif /* CONFIG_MAX_NUM_NODES > 1 */

    ldr     sp, =core_stack_alloc + 0xff0
    b       main
END_FUNC(_start)

#if CONFIG_MAX_NUM_NODES > 1
BEGIN_FUNC(non_boot_core)
    /* Disable Async aborts that might be pending from bootloader */
    cpsid   ifa

    /* Enable SMP */
    mrc     ACTLR(r0)
    orr     r0, r0, #(1 << 6)       /* enable SMP bit   */
#ifdef CONFIG_ARM_CORTEX_A9
    orr     r0, r0, #1              /* enable FW bit    */
#endif
    mcr     ACTLR(r0)

    ldr     r0, =smp_aps_index
1:  ldrex   r1, [r0]
    add     r1, r1, #1
    strex   r2, r1, [r0]
    teq     r2, #0
    bne     1b

    mov     r0, #0x1000
    mul     r1, r0
    ldr     r3, =core_stack_alloc - 0x10
    add     r3, r1
    mov     sp, r3
    b       non_boot_main
END_FUNC(non_boot_core)
#endif /* CONFIG_MAX_NUM_NODES */

/*
 * Symbols required for libgcc.
 */
.global raise
.global __aeabi_unwind_cpp_pr0
.global __aeabi_unwind_cpp_pr1
.global __aeabi_unwind_cpp_pr2
raise:
__aeabi_unwind_cpp_pr0:
__aeabi_unwind_cpp_pr1:
__aeabi_unwind_cpp_pr2:
    b       raise
