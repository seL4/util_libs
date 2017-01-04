/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.
 * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(NICTA_GPL)
 */

#include <assembler.h>

.text

BEGIN_FUNC(smc)
    .arch_extension sec
    stmfd    sp!, {r3-r11, lr}
    dsb
    smc #0
    ldmfd    sp!, {r3-r11, pc}
END_FUNC(smc)
