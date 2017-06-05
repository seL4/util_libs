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

.text

BEGIN_FUNC(smc)
    .arch_extension sec
    stmfd    sp!, {r3-r11, lr}
    dsb
    smc #0
    ldmfd    sp!, {r3-r11, pc}
END_FUNC(smc)
