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

#define   HVBAR(reg)    p15, 4, reg, c12, c0, 0
#define     HCR(reg)    p15, 4, reg, c1 , c1, 0
#define  HSCTLR(reg)    p15, 4, reg, c1 , c0, 0
#define  HACTLR(reg)    p15, 4, reg, c1 , c0, 1
#define    HDCR(reg)    p15, 4, reg, c1 , c1, 1
#define   HCPTR(reg)    p15, 4, reg, c1 , c1, 2
#define    HSTR(reg)    p15, 4, reg, c1 , c1, 3
#define    HACR(reg)    p15, 4, reg, c1 , c1, 7
#define    HTCR(reg)    p15, 4, reg, c2 , c0, 2
#define  HADFSR(reg)    p15, 4, reg, c5 , c1, 0
#define  HAIFSR(reg)    p15, 4, reg, c5 , c1, 1
#define     HSR(reg)    p15, 4, reg, c5 , c2, 0
#define   HDFAR(reg)    p15, 4, reg, c6 , c0, 0
#define   HIFAR(reg)    p15, 4, reg, c6 , c0, 2
#define   HPFAR(reg)    p15, 4, reg, c6 , c0, 4
#define  HMAIR0(reg)    p15, 4, reg, c10, c2, 0
#define  HMAIR1(reg)    p15, 4, reg, c10, c2, 1
#define HAMAIR0(reg)    p15, 4, reg, c10, c3, 0
#define HAMAIR1(reg)    p15, 4, reg, c10, c3, 1
#define  HTPIDR(reg)    p15, 4, reg, c13, c0, 2
#define   HTTBR(rh,rl)  p15, 4, rl, rh, c2


#define  DTLBIALL(reg) p15, 0, reg, c8, c6, 0
#define   TLBIALL(reg) p15, 0, reg, c8, c7, 0
#define DTLBIASID(reg) p15, 0, reg, c8, c6, 2
#define  TLBIASID(reg) p15, 0, reg, c8, c7, 2
.text

/*
 * Enable the ARM MMU.
 *
 * It is expected that the code of this function will be mapped 1:1
 * virtual/physical in the pagetable we activate.
 */
.global arm_enable_hyp_mmu
.extern _lpae_boot_pgd
.extern flush_dcache
.extern invalidate_dcache
.extern invalidate_icache

arm_enable_hyp_mmu:
    stmfd   sp!, {lr}
    /* Clean D-Cache if enabled */
    mrc     HSCTLR(r1)
    and     r1, r1, #(1 << 2)
    cmp     r1, #0
    bleq flush_dcache
    /* Ensure I-cache, D-cache and mmu are disabled. */
    mrc     HSCTLR(r1)
    bic     r1, r1, #(1 << 12) /* Disable I-cache */
    bic     r1, r1, #(1 << 2)  /* Disable D-Cache */
    bic     r1, r1, #(1 << 0)  /* Disable MMU */
    mcr     HSCTLR(r1)
    /* invalidate caches. */
    bl       invalidate_dcache
    bl       invalidate_icache
    /* Setup MAIR - Strongly ordered non-cachable for all index */
    mov     r1, #0
    mcr     HMAIR0(r1)
    mcr     HMAIR1(r1)
    /* Set up the page table */
    ldr     r0, =_lpae_boot_pgd
    mov     r1, #0
    mov     r2, #(1<<31)
    mcrr    HTTBR(r1,r0)
    mcr     HTCR(r2)
    /* Invalidate TLB */
    mcr     DTLBIALL(r1)
    /* Enable MMU, D-cache, and I-cache. */
    mrc     HSCTLR(r0)
    orr     r0, r0, #(1 << 2)  // enable dcache
    orr     r0, r0, #(1 << 12) // enable icache
    orr     r0, r0, #(1 << 0)  // MMU enable 
    mcr     HSCTLR(r0)

#ifdef CONFIG_SMP_ARM_MPCORE
    /* Enable SMP */
    mrc     p15, 0, r0, c1, c0, 1
    orr     r0, r0, #(1 << 6) // enable SMP bit
    mcr     p15, 0, r0, c1, c0, 1
#endif

    ldmfd   sp!, {pc}

