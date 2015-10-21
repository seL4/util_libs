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

#include <platsupport/plat/epit_constants.h>
#include <platsupport/timer.h>

#include <stdint.h>

typedef struct {
    /* vaddr epit is mapped to */
    void *vaddr;
    /* irq for this epit (should be EPIT_INTERRUPT1 or EPIT_INTERRUPT2 depending on the epit */
    uint32_t irq;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} epit_config_t;

pstimer_t *epit_get_timer(epit_config_t *config);

#endif /* __PLAT_SUPPORT_EPIT_H */
