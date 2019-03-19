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
    /* If we're not booting under EFI, r0 will contain a pointer to the DTB */

    /* Disable Async aborts that might be pending from bootloader */
    cpsid   ifa

    /* r0 contains the FDT address from the bootloader */
#if CONFIG_MAX_NUM_NODES > 1
    /* Enable SMP */
    mrc     ACTLR(r1)
    orr     r1, r1, #(1 << 6)       /* enable SMP bit   */
#ifdef CONFIG_ARM_CORTEX_A9
    orr     r1, r1, #1              /* enable FW bit    */
#endif
    mcr     ACTLR(r1)
#endif /* CONFIG_MAX_NUM_NODES > 1 */

    adr     r1, LC0 // r1 = relocated address of literal pool
    /*
     * r4 = LC0 unrelocated address
     * r5 = stack pointer
     */
    ldmia   r1, {r4, r5}

    sub     r1, r1, r4 // r1 now contains relocation offset
    add     sp, r5, r1 // set up sp properly

    b       main
END_FUNC(_start)

.align 2
.type LC0, #object
LC0:
.word LC0                       // r4
.word core_stack_alloc + 0xff0 // r5
.size LC0, . - LC0

BEGIN_FUNC(arch_early_relocate)
    /* At this point, we're running on the EFI-provided stack, which is not a problem.
     * Before we can save the EFI arguments in efi_early_init, we need to relocate
     * the global offset table (GOT) entries.
     */

    push    {r0, r1}                    // save EFI arguments
    adr     r0, LC1                  // get actual address of LC1
    ldmia   r0, {r4, r6, r7, r8}

    /*
     * r4 = unrelocated LC1 address
     * r6 = GOT start (unrelocated)
     * r7 = GOT end (unrelocated)
     * r8 = vector literal pool (unrelocated)
     */

    sub     r0, r0, r4                  // get relocation offset
    add     r6, r6, r0                  // relocate GOT start
    add     r7, r7, r0                  // relocate GOT end
    add     r8, r8, r0                  // relocate vector literal pool

    /* relocate vector table literals */
1:  ldr r1, [r8, #0]                    // load table entry
    cmp r1, #0                          // zero => end of literal pool
    beq 2f
    add r1, r1, r0                      // relocate
    str r1, [r8], #4                    // put back
    b 1b
2:
    /* relocate GOT entries - fix up C global variables. */
1:  ldr r1, [r6, #0]                    // load table entry
    add r1, r1, r0                      // relocate
    str r1, [r6], #4                    // put back
    cmp r6, r7
    blo 1b

    pop {r0, r1}                        // restore EFI arguments
    mov pc, lr
END_FUNC(arch_early_relocate)
.align 2
.type LC1, #object
LC1:
.word LC1
.word _got_start
.word _got_end
.word arm_vector_literals
.size LC1, . - LC1

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

    /* Relocate literals */
    adr     r0, LC3
    ldmia   r0, {r1, r2, r3}

    sub     r0, r0, r1              // relocation offset
    add     r2, r2, r0              // relocate smp_aps_index
    add     r3, r3, r0              // relocate core_stack_alloc - 0x10

    /* Get stack index */
    mov     r0, r2
1:  ldrex   r1, [r0]
    add     r1, r1, #1
    strex   r2, r1, [r0]
    teq     r2, #0
    bne     1b

    /* Set up stack */
    mov     r0, #0x1000
    mul     r1, r0
    add     r3, r1
    mov     sp, r3
    b       non_boot_main
END_FUNC(non_boot_core)

.align 2
.type LC3, #object
LC3:
.word LC3                       // r1
.word smp_aps_index             // r2
.word core_stack_alloc - 0x10   // r3
.size LC3, . - LC3
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
