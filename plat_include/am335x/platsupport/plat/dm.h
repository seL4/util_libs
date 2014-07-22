/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_EPIT_H
#define __PLAT_SUPPORT_EPIT_H

#include <platsupport/timer.h>

#define DMTIMER2_PADDR 0x48040000
#define DMTIMER2_INTERRUPT 68

/**
 * Get the dm timer interface.
 *
 * @param vaddr that DMTIMER2_PADDR is mapped in to.
 * @return NULL on error.
 */
pstimer_t *dm_get_timer(void *vaddr);

#endif /* __PLAT_SUPPORT_EPIT_H */
