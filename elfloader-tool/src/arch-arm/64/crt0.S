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

#include <assembler.h>

.extern main

.section ".text.start"

BEGIN_FUNC(_start)
    adrp    x19, core_stack_alloc
    add     x19, x19, #0xff0
    mov     sp, x19
    b       main
END_FUNC(_start)
