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
#include <mode/assembler.h>

.text

.extern _boot_pgd_up
.extern _boot_pgd_down
.extern arm_vector_table

BEGIN_FUNC(invalidate_dcache)
    dcache  isw
    ret
END_FUNC(invalidate_dcache)

BEGIN_FUNC(invalidate_icache)
    ic      iallu
    dsb     nsh
    isb
    ret
END_FUNC(invalidate_icache)

BEGIN_FUNC(flush_dcache)
    dcache  cisw
    ret
END_FUNC(flush_dcache)

BEGIN_FUNC(arm_enable_mmu)
    /* We call nested functions, follow the ABI. */
    stp     x29, x30, [sp, #-16]!
    mov     x29, sp

    bl      flush_dcache

    /* Ensure I-cache, D-cache and mmu are disabled for EL1/Stage1 */
    disable_mmu sctlr_el1 , x8

    /*
     * Invalidate the local I-cache so that any instructions fetched
     * speculatively are discarded.
     */
    bl      invalidate_icache

    /*
     *   DEVICE_nGnRnE      000     00000000
     *   DEVICE_nGnRE       001     00000100
     *   DEVICE_GRE         010     00001100
     *   NORMAL_NC          011     01000100
     *   NORMAL             100     11111111
     */
    ldr     x5, =MAIR(0x00, MT_DEVICE_nGnRnE) | \
                 MAIR(0x04, MT_DEVICE_nGnRE) | \
                 MAIR(0x0c, MT_DEVICE_GRE) | \
                 MAIR(0x44, MT_NORMAL_NC) | \
                 MAIR(0xff, MT_NORMAL)
    msr     mair_el1, x5

    ldr     x10, =TCR_TxSZ(48) | TCR_IRGN_WBWA | TCR_ORGN_WBWA | TCR_TG0_4K | TCR_TG1_4K | TCR_ASID16
    mrs     x9, ID_AA64MMFR0_EL1
    bfi     x10, x9, #32, #3
    msr     tcr_el1, x10

    /* Setup page tables */
    adrp    x8, _boot_pgd_down
    msr     ttbr0_el1, x8
    adrp    x8, _boot_pgd_up
    msr     ttbr1_el1, x8
    isb

    /* invalidate all TLB entries for EL1 */
    tlbi    vmalle1is
    dsb     ish
    isb

    enable_mmu sctlr_el1 , x8

    adrp    x8, arm_vector_table
    msr     vbar_el1, x8

    ldp     x29, x30, [sp], #16
    ret
END_FUNC(arm_enable_mmu)
