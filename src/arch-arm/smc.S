/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */


.text

.global smc
smc:
    stmfd    sp!, {r4-r11, lr}
    dsb
    .word 0x410024fc /* smc #0 */
    ldmfd    sp!, {r4-r11, pc}
    bx lr

