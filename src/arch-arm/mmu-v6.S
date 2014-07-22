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

#ifdef ARMV6

.text

/*
 * Enable the ARM MMU
 *
 * It is expected that the code of this function will be mapped 1:1
 * virtual/physical in the pagetable we activate.
 */
.global arm_enable_mmu
.extern _boot_pd
arm_enable_mmu:
    /* load boot page directory pointer */
    ldr     r2, =_boot_pd

    /* ensure i-cache is disabled */
    mrc     p15, 0, r0, c1, c0, 0
    bic     r1, r0, #(1 << 12)
    mcr     p15, 0, r1, c1, c0, 0

    /* clean entire d-cache. */
    mov     r1, #0
    nop
    mcr     p15, 0, r1, c7, c14, 0
    nop
    nop

    /* drain write-buffer */
    mcr     p15, 0, r1, c7, c10, 0

    /* clean i-cache */
    mcr     p15, 0, r1, c7, c5, 0

    /* setup client to only have access to domain 0, and setup the DACR */
    mov     r1, #1
    mcr     p15, 0, r1, c3, c0, 0

#ifdef ARMV5
    /* set page table */
    mcr p15, 0, r2, c2, c0, 0

    mov r1, #0
    mcr p15, 0, r1, c8, c7, 0
    mcr p15, 0, r1, c7, c5, 4
    mcr p15, 0, r1, c7, c5, 6

    /* enable mmu and d/i cache */
    mov r0, #0
    mrc p15, 0, r0, c1, c0, 0
    orr r0, r0, #1
    orr r0, r0, #(1 << 2)
    orr r0, r0, #(1 << 12)
    mcr p15, 0, r0, c1, c0, 0
    nop
    nop
#else /* ARMv6 */
    /* enable caching of pagetables, setting TTBR0 */
    orr     r1, r2, #0x19
    mcr     p15, 0, r1, c2, c0, 0

    /* setup misc MMU */
    mov     r1, #0
    mcr     p15, 0, r1, c13, c0, 1 // set ASID to 0
    mcr     p15, 0, r1, c8, c7, 0 // invalidate TLB entries
    mcr     p15, 0, r1, c2, c0, 2 // set TTBCR to 0
    mcr     p15, 0, r1, c7, c5, 4 // flush prefetch buffer
    mcr     p15, 0, r1, c7, c5, 6 // flush branch target cache

    /* enable MMU, d-cache, and i-cache */
    orr     r0, r0, #5 // set MMU and dcache enable bits
    orr     r0, r0, #(1 << 12) // set icache enable bit
    mcr     p15, 0, r0, c1, c0, 0
#endif

    bx      lr

.global arm_enable_hyp_mmu
arm_enable_hyp_mmu:
    bl abort

#endif /* ARMV6 */
