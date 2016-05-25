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

.extern main

.text
.global _start
_start:
#ifdef CONFIG_SMP_ARM_MPCORE
    bl      _asm_get_mpidr
    ldr     r2, =booting_cpu_id
    ldrex   r1, [r2] 
    cmp     r1, #0xff
    bne     non_boot_core
    strex   r3, r0, [r2]
    cmp     r3, #0
    bne     _start
#endif

    ldr sp, =_bootstack_top
    b   main

_asm_get_mpidr:
    mrc     p15, 0, r0, c0, c0, 5
    and     r0, r0, #0xf
    bx      lr

.global cpu_idle
cpu_idle:
    mcr     p15, 0, r0, c7, c10, 4
    mcr     p15, 0, r0, c7, c0, 4
    bx      lr

.extern non_boot_main

.global non_boot_core
non_boot_core:
    bl _asm_get_mpidr
    cmp r0, #1
    ldreq     sp, =_bootstack_top_core1
    cmp r0, #2
    ldreq     sp, =_bootstack_top_core2
    cmp r0, #3
    /* If we got more than 4 cores we are hozed, just don't bring them up */
    bhi non_boot_core
    ldreq     sp, =_bootstack_top_core3
    b       non_boot_main

.align 8
.space 1024
_bootstack_top_core1:
.align 8
.space 1024
_bootstack_top_core2:
.align 8
.space 1024
_bootstack_top_core3:

.global booting_cpu_id
booting_cpu_id:
.word   0xff

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
