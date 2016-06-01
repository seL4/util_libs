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

.text

/*
 * Enable the ARM MMU.
 *
 * It is expected that the code of this function will be mapped 1:1
 * virtual/physical in the pagetable we activate.
 */
.global arm_enable_mmu
.global flush_dcache
.global invalidate_dcache
.global invalidate_icache
.extern _boot_pd


arm_enable_mmu:

    stmfd   sp!, {lr}

    /* Clean D-Cache if enabled */
    mrc     p15, 0, r1, c1, c0, 0
    and     r1, r1, #(1 << 2)
    cmp     r1, #0
    beq     1f
    bl flush_dcache

1:
    /* Ensure I-cache, D-cache and mmu are disabled. */

    mrc     p15, 0, r1, c1, c0, 0
    bic     r1, r1, #(1 << 12)      /* Disable I-cache */
    bic     r1, r1, #(1 << 2)       /* Disable D-Cache */
    bic     r1, r1, #(1 << 0)       /* Disable MMU     */
    mcr     p15, 0, r1, c1, c0, 0

    /* invalidate caches. */
    bl       invalidate_dcache
    bl       invalidate_icache

    /* Set up TTBR0, enable caching of pagetables. */
    ldr     r0, =_boot_pd
    orr     r1, r0, #0x19
    mcr     p15, 0, r1, c2, c0, 0
    /* Invalidate entire TLB (TLBIALL) */
    mcr     p15, 0, r1, c8, c7, 0

    /*
     * Setup client to only have access to domain 0, and setup
     * the DACR.
     */
    mov     r1, #1
    mcr     p15, 0, r1, c3, c0, 0

    /* Setup misc MMU. */
    mov     r1, #0
    mcr     p15, 0, r1, c13, c0, 1  /* set ASID to 0    */
    mcr     p15, 0, r1, c2, c0, 2   /* set TTBCR to 0   */
    mcr     p15, 0, r1, c7, c5, 6   /* flush branch target cache */
#ifndef ARMV8_A
    /* The coprocessor register deprecated in ARMV8 A */
    mcr     p15, 0, r1, c7, c5, 4   /* flush prefetch buffer */
#else
    isb
#endif

    /* Enable MMU, D-cache, and I-cache. */
    mrc     p15, 0, r0, c1, c0, 0
    orr     r0, r0, #5              /* set MMU and dcache enable bits */
    orr     r0, r0, #(1 << 12)      /* set icache enable bit */
    mcr     p15, 0, r0, c1, c0, 0

#ifdef CONFIG_SMP_ARM_MPCORE
    /* Enable SMP */
    mrc     p15, 0, r0, c1, c0, 1
    orr     r0, r0, #(1 << 6)       /* enable SMP bit   */
    mcr     p15, 0, r0, c1, c0, 1
#endif

    ldmfd   sp!, {pc}



invalidate_dcache:
    stmfd sp!, {r4-r8,lr}
    /* invalidate entire D-cache. */
    /* On ARMv7, we need to do this by set/way. */
    /* This code does it generically for any cache hierarchy. */

    /* Read Cache Level ID */
    mrc     p15, 1, r0, c0, c0, 1
    /* Read Level of Coherency */
    /* Level of Coherency is where we need to flush up to. */
    and     r2, r0, #(7 << 24)
    lsr     r2, r2, #24

    /* for (level = 0; level < LoC; level++) { */
    mov     r1, #0
0:
    cmp     r1, r2
    beq     4f

    /*
     * Grab bits for this level of cache.
     * 0 = no cache, 1 = I-cache, 2 = D-cache
     * 3 = separate I+D, 4 = unified
     *
     * Anything 2 or higher means we need to flush.
     */
    and     r3, r0, #7
    cmp     r3, #1
    bls     3f

    /* Select the cache level to determine size. */
    lsl     r3, r1, #1
    mcr     p15, 2, r3, c0, c0, 0
    isb

    /* Read cache size. */
    mrc     p15, 1, r3, c0, c0, 0

    /* Extract line size (log2(cache size) - 4) bytes. */
    and     r4, r3, #7
    add     r4, r4, #4

    /* Extract associativity - 1. */
    lsr     r5, r3, #3
    mov     r8, #0x400
    sub     r8, r8, #1
    and     r5, r5, r8

    /* Determine how many bits up the way number needs to be shifted. */
    clz     r6, r5

    /* Extract number of sets - 1. */
    lsr     r7, r3, #13
    mov     r8, #0x8000
    sub     r8, r8, #1
    and     r7, r7, r8

    /* Flush em! */
1:
    mov     r3, r5 /* Way number */
2:
    mov     r8, r1, lsl #1
    orr     r8, r8, r7, lsl r4
    orr     r8, r8, r3, lsl r6
    mcr     p15, 0, r8, c7, c6, 2
    subs    r3, r3, #1
    bge     2b
    subs    r7, r7, #1
    bge     1b

3:
    add     r1, r1, #1
    lsr     r0, r0, #3
    b       0b

4:
    /* Select cache level 0. */
    mov     r0, #0
    mcr     p15, 2, r0, c0, c0, 0
    dsb
    isb

    ldmfd   sp!, {r4-r8,pc}

invalidate_icache:
    /* invalidate I-cache. */
    mcr     p15, 0, r1, c7, c5, 0
    bx      lr

flush_dcache:
    stmfd sp!, {r4-r8,lr}
    /* Clean and invalidate entire D-cache. */
    /* On ARMv7, we need to do this by set/way. */
    /* This code does it generically for any cache hierarchy. */

    /* Read Cache Level ID */
    mrc     p15, 1, r0, c0, c0, 1
    /* Read Level of Coherency */
    /* Level of Coherency is where we need to flush up to. */
    and     r2, r0, #(7 << 24)
    lsr     r2, r2, #24

    /* for (level = 0; level < LoC; level++) { */
    mov     r1, #0
0:
    cmp     r1, r2
    beq     4f

    /*
     * Grab bits for this level of cache.
     * 0 = no cache, 1 = I-cache, 2 = D-cache
     * 3 = separate I+D, 4 = unified
     *
     * Anything 2 or higher means we need to flush.
     */
    and     r3, r0, #7
    cmp     r3, #1
    bls     3f

    /* Select the cache level to determine size. */
    lsl     r3, r1, #1
    mcr     p15, 2, r3, c0, c0, 0
    isb

    /* Read cache size. */
    mrc     p15, 1, r3, c0, c0, 0

    /* Extract line size (log2(cache size) - 4) bytes. */
    and     r4, r3, #7
    add     r4, r4, #4

    /* Extract associativity - 1. */
    lsr     r5, r3, #3
    mov     r8, #0x400
    sub     r8, r8, #1
    and     r5, r5, r8

    /* Determine how many bits up the way number needs to be shifted. */
    clz     r6, r5

    /* Extract number of sets - 1. */
    lsr     r7, r3, #13
    mov     r8, #0x8000
    sub     r8, r8, #1
    and     r7, r7, r8

    /* Flush em! */
1:
    mov     r3, r5 /* Way number */
2:
    mov     r8, r1, lsl #1
    orr     r8, r8, r7, lsl r4
    orr     r8, r8, r3, lsl r6
    mcr     p15, 0, r8, c7, c14, 2
    subs    r3, r3, #1
    bge     2b
    subs    r7, r7, #1
    bge     1b

3:
    add     r1, r1, #1
    lsr     r0, r0, #3
    b       0b

4:
    /* Select cache level 0. */
    mov     r0, #0
    mcr     p15, 2, r0, c0, c0, 0
    dsb
    isb

    ldmfd   sp!, {r4-r8,pc}
