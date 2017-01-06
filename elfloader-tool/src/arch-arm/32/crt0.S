/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <autoconf.h>
#include <assembler.h>

.extern main
.extern non_boot_main

.section ".text.start"

BEGIN_FUNC(_start)
    ldr sp, =[core_stack_alloc + 0xffc]
    b   main
END_FUNC(_start)

BEGIN_FUNC(non_boot_core)
    ldr     r0, =[smp_aps_index]
1:  ldrex   r1, [r0]
    add     r1, r1, #1
    strex   r2, r1, [r0]
    teq     r2, #0
    bne     1b

    mov     r0, #0x1000
    mul     r1, r0
    ldr     r3, =[core_stack_alloc - 0x4]
    add     r3, r1
    mov     sp, r3
    b       non_boot_main
END_FUNC(non_boot_core)

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
