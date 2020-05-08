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

#include <platsupport/ltimer.h>

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
    /* prescaler to scale time by. 0 = divide by 1. 1 = divide by 2. ...*/
    uint32_t prescaler;
} gpt_config_t;

typedef struct gpt {
    /* set up during gpt_create() */
    ps_io_ops_t                     ops;
    ltimer_callback_fn_t            user_callback;
    void                            *user_callback_token;

    /* set up during gpt_create() callbacks */
    pmem_region_t                   timer_pmem;
    volatile struct gpt_map         *gpt_map; /* NULL implies invalid pmem */
    irq_id_t                        irq_id;

    /* set up during rel / abs init */
    uint32_t                        prescaler;
    uint32_t                        high_bits;
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
 * Sets up the basic memory mapping and interrupt handling and sets
 * common registers.
 *
 * Device needs further configuration depending on use-case (i.e.
 * absolute mode may need registers set.)
 * Therefore, use the relevant device setup functions afterwards.
 *
 * @returns 0 if successful. Error code if not.
 */
int gpt_create(gpt_t *gpt, ps_io_ops_t ops, char *fdt_path, ltimer_callback_fn_t user_cb_fn, void *user_cb_token);

/**
 * Functions to get a GPT timer which is programmed to overflow
 * at 0xFFFFFFFF and fire an irq and reload to 0. This can be
 * used to track absolute time.
 *
 */
int abs_gpt_init(gpt_t *gpt, gpt_config_t config);
uint64_t abs_gpt_get_time(gpt_t *gpt);

/**
 * Functions to get a GPT timer that can do periodic or oneshot
 * relative timeouts.
 */
int rel_gpt_init(gpt_t *gpt, gpt_config_t config);
int rel_gpt_set_timeout(gpt_t *gpt, uint64_t ns, bool periodic);

/* General GPT management functions */
void gpt_stop(gpt_t *gpt);
void gpt_start(gpt_t *gpt);
void gpt_destroy(gpt_t *gpt);
/* get the max value ns this gpt can be programmed with */
uint64_t gpt_get_max(void);
