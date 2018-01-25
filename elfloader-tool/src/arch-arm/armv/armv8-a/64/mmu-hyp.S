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

#include <assembler.h>
#include <armv/assembler.h>

.text

.extern flush_dcache
.extern invalidate_dcache
.extern invalidate_icache

BEGIN_FUNC(leave_hyp)
    /* We call nested functions, follow the ABI. */
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    bl      flush_dcache

    /* Ensure I-cache, D-cache and mmu are disabled for EL2/Stage1 */
    disable_mmu sctlr_el2, x9

    /*
     * Invalidate the local I-cache so that any instructions fetched
     * speculatively are discarded.
     */
    bl      invalidate_icache

    /* Ensure I-cache, D-cache and mmu are disabled for EL1/Stage2 */
    mov     x9, #(1 << 31)
    msr     hcr_el2, x9

    /* Ensure traps to EL2 are disabled */
    mov     x9, #0x33ff
    msr     cptr_el2, x9
    msr     hstr_el2, xzr
    msr     vttbr_el2, xzr

    /* Ensure I-cache, D-cache and mmu are disabled for EL1/Stage1 */
    disable_mmu sctlr_el1 , x9

    mov     x9, #(PSR_F_BIT | PSR_I_BIT | PSR_A_BIT | PSR_D_BIT | PSR_MODE_EL1h)
    msr     spsr_el2, x9

    /* Let's the caller use our stack, in case it needs to pop something */
    ldp     x29, x30, [sp], #16
    mov     x10, sp
    msr     sp_el1, x10
    msr     elr_el2, x30
    eret
END_FUNC(leave_hyp)

BEGIN_FUNC(arm_enable_hyp_mmu)

END_FUNC(arm_enable_hyp_mmu)
