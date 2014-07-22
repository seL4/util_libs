/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */

#ifndef __PLAT_SUPPORT_GPT_H
#define __PLAT_SUPPORT_GPT_H

#include <platsupport/timer.h>
#include <platsupport/plat/gpt_constants.h>

#include <stdint.h>

typedef struct {
    /* vaddr gpt is mapped to */
    void *vaddr;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} gpt_config_t;

pstimer_t *gpt_get_timer(gpt_config_t *config);

#endif /* __PLAT_SUPPORT_GPT_H */
