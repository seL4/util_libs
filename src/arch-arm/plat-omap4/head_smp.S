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

#ifdef CONFIG_SMP_ARM_MPCORE
.extern non_boot_main
.extern non_boot_core

.global omap_non_boot
omap_non_boot:
    ldr     r12,=0x103
    dsb
    smc     #0
    mov     r0, r0, lsr #9
    mrc     p15, 0, r4, c0, c0, 5
    and     r4, r4, #0x0f
    cmp     r0, r4
    dsb
    bne     omap_non_boot
    b       non_boot_core 

.global omap_write_auxcoreboot0
omap_write_auxcoreboot0:
    stmfd   sp!, {r1-r12, lr}
    ldr     r12, =0x104
    dsb
    smc     #0
    ldmfd   sp!, {r1-r12, pc}

.global omap_write_auxcoreboot_addr
omap_write_auxcoreboot_addr:
    stmfd   sp!, {r2-r12, lr}
    ldr     r12, =0x105
    dsb
    smc     #0
    ldmfd   sp!, {r2-r12, pc}

#endif
