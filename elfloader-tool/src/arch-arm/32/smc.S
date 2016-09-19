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

/* Processor mode encodings (for CPS etc.) */
#define PMODE_USER       0x10
#define PMODE_FIQ        0x11
#define PMODE_IRQ        0x12
#define PMODE_SUPERVISOR 0x13
#define PMODE_ABORT      0x17
#define PMODE_UNDEFINED  0x1b
#define PMODE_SYSTEM     0x1f
/* Processor exception mask bits */
#define PMASK_ASYNC_ABORT (1 << 8)
#define PMASK_IRQ         (1 << 7)
#define PMASK_FIRQ        (1 << 6)

#define CPSR_SUPERVISOR      ( PMASK_FIRQ         \
                             | PMASK_IRQ          \
                             | PMASK_ASYNC_ABORT  \
                             | PMODE_SUPERVISOR   )

.text


.global smc
smc:
    .arch_extension sec
    stmfd    sp!, {r3-r11, lr}
    dsb
    smc #0
    ldmfd    sp!, {r3-r11, pc}

#ifndef CONFIG_ARM_HYPERVISOR_SUPPORT
.global leave_hyp
leave_hyp:
    .arch_extension virt
    ldr r0, =CPSR_SUPERVISOR
    msr spsr_hyp, r0
    msr sp_svc, sp
    msr elr_hyp, lr
    eret

#endif
