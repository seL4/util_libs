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

#include "../assembler.h"
#include "assembler-v6.h"

.text

.extern _boot_pd

/*
 * Enable the ARM MMU
 *
 * It is expected that the code of this function will be mapped 1:1
 * virtual/physical in the pagetable we activate.
 */
BEGIN_FUNC(arm_enable_mmu)
    /* ensure i-cache is disabled */
    mrc     SCTLR(r0)
    bic     r1, r0, #(1 << 12)
    mcr     SCTLR(r1)

    /* clean entire d-cache. */
    mov     r1, #0
    nop
    mcr     DCIALL(r1)
    nop
    nop

    /* drain write-buffer */
    mcr     DCALL(r1)

    /* clean i-cache */
    mcr     IIALL(r1)

    /* Setup client to only have access to domain 0, and setup the DACR. */
    mov     r1, #1
    mcr     DACR(r1)

    /* Set up TTBR0, enable caching of pagetables. */
    ldr     r2, =_boot_pd
    orr     r1, r2, #0x19
    mcr     TTBR0(r1)

    /* setup misc MMU */
    mov     r1, #0
    mcr     CONTEXTIDR(r1)  /* set ASID to 0    */
    mcr     TLBIALL(r1)     /* invalidate TLB entries */
    mcr     TTBCR(r1)       /* set TTBCR to 0   */
    mcr     PIALL(r1)       /* flush prefetch buffer */
    mcr     BPIALL(r1)      /* flush branch target cache */

    /* Enable MMU, D-cache, and I-cache. */
    orr     r0, r0, #(1 << 12)      /* Enable I-cache */
    orr     r0, r0, #(1 << 2)       /* Enable D-cache */
    orr     r0, r0, #(1 << 0)       /* Enable MMU */
    mcr     SCTLR(r0)

    bx      lr
END_FUNC(arm_enable_mmu)

BEGIN_FUNC(arm_enable_hyp_mmu)
    bl abort
END_FUNC(arm_enable_hyp_mmu)
