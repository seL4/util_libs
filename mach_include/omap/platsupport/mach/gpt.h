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
typedef enum {
    GPT_FIRST = 0,
    GPT1 = 0,
    GPT2 = 1,
    GPT3 = 2,
    GPT4 = 3,
    GPT5 = 4,
    GPT6 = 5,
    GPT7 = 6,
    GPT8 = 7,
    GPT9 = 8,
    GPT10 = 9,
    GPT11 = 10, 
    GPT_LAST = GPT11
} gpt_id_t;

typedef struct {
    /* vaddr gpt is mapped to */
    void *vaddr;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} gpt_config_t;

pstimer_t *gpt_get_timer(gpt_config_t *config);

#endif /* __PLAT_SUPPORT_GPT_H */
