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

#include <autoconf.h>

#include <assembler.h>
#include <armv/assembler.h>

.extern non_boot_core

#if CONFIG_MAX_NUM_NODES > 1
BEGIN_FUNC(tk1_non_boot)
    /* Invalidate caches before proceeding... */
    mov     r0, #0
    mcr     IIALL(r0)
    dcache  isw

    b       non_boot_core
END_FUNC(tk1_non_boot)
#endif /* CONFIG_MAX_NUM_NODES */
