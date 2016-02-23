/*
 * @TAG(OTHER_GPL)
 */

/* Some code in here derived from Linux */

/*
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 */

#include <autoconf.h>

.extern non_boot_core

.global imx_non_boot
imx_non_boot:
    /* Our caches are in an unknown state. We need to perform an INVALIDATE
       to ensure we do not ship random data to memory in future cache cleans */
    bl invalidate_l1
    b non_boot_core

invalidate_l1:
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0   @ invalidate I cache
    mcr p15, 2, r0, c0, c0, 0
    mrc p15, 1, r0, c0, c0, 0

    ldr r1, =0x7fff
    and r2, r1, r0, lsr #13

    ldr r1, =0x3ff

    and r3, r1, r0, lsr #3  @ NumWays - 1
    add r2, r2, #1      @ NumSets

    and r0, r0, #0x7
    add r0, r0, #4  @ SetShift

    clz r1, r3      @ WayShift
    add r4, r3, #1  @ NumWays
1:  sub r2, r2, #1  @ NumSets--
    mov r3, r4      @ Temp = NumWays
2:  subs    r3, r3, #1  @ Temp--
    mov r5, r3, lsl r1
    mov r6, r2, lsl r0
    orr r5, r5, r6  @ Reg = (Temp<<WayShift)|(NumSets<<SetShift)
    mcr p15, 0, r5, c7, c6, 2
    bgt 2b
    cmp r2, #0
    bgt 1b
    dsb
    isb
    mov pc, lr
