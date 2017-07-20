/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

#pragma once

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

#include <platsupport/plat/gpt_constants.h>

typedef struct {
    /* which gpt */
    gpt_id_t id;
    /* vaddr gpt frame is mapped to */
    void *vaddr;
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} gpt_config_t;

typedef struct gpt {
    volatile struct gpt_map *gpt_map;
    uint64_t counter_start;
    uint32_t irq;
    gpt_id_t id;
    uint32_t prescaler;
    uint32_t high_bits;
} gpt_t;

static UNUSED timer_properties_t abs_gpt_properties = {
    .absolute_timeouts = true,
    .relative_timeouts = true,
    .periodic_timeouts = false,
    .upcounter = true,
    .bit_width = 32,
    .irqs = 1
};

static UNUSED timer_properties_t rel_gpt_properties = {
    .absolute_timeouts = false,
    .relative_timeouts = true,
    .periodic_timeouts = true,
    .upcounter = true,
    .bit_width = 32,
    .irqs = 1
};

/**
 * Functions to get a GPT timer which is programmed to overflow
 * at 0xFFFFFFFF and fire an irq and reload to 0. This can be
 * used to track absolute time, and also set absolute timeouts.
 *
 */
int abs_gpt_init(gpt_t *gpt, gpt_config_t config);
uint64_t abs_gpt_get_time(gpt_t *gpt);
int abs_gpt_set_timeout(gpt_t *gpt, uint64_t abs_ns);

/**
 * Functions to get a GPT timer that can do periodic or oneshot
 * relative timeouts.
 */
int rel_gpt_init(gpt_t *gpt, gpt_config_t config);
int rel_gpt_set_timeout(gpt_t *gpt, uint64_t ns, bool periodic);

/* General GPT management functions */
int gpt_stop(gpt_t *gpt);
int gpt_start(gpt_t *gpt);
void gpt_handle_irq(gpt_t *gpt);
