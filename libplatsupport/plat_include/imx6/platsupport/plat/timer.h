/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
#pragma once

/* Default ipg_clk configuration:
 *
 * FIN (24 MHZ)
 *  |_ *22 PLL2 (528)
 *           |_ /4 AHB_CLK (132 MHZ)
 *                     |_ /2 IPG_CLK (66 MHZ)
 *                              |_ EPIT
 */
#define PLL2_FREQ  (528u)
#define AHB_FREQ   (PLL2_FREQ / 4u)
#define IPG_FREQ   (AHB_FREQ  / 2u)
#define GPT_FREQ   IPG_FREQ

#include <platsupport/mach/gpt.h>
#include <platsupport/mach/epit.h>

typedef struct {
    gpt_t timestamp;
    epit_t timeout;
} imx_timers_t;

static inline uint64_t imx_get_time(imx_timers_t *timers)
{
    return gpt_get_time(&timers->timestamp);
}

static inline void imx_start_timestamp(imx_timers_t *timers)
{
    gpt_start(&timers->timestamp);
}

static inline void imx_stop_timestamp(imx_timers_t *timers)
{
    gpt_stop(&timers->timestamp);
}

static inline int imx_init_timestamp(imx_timers_t *timers, ps_io_ops_t io_ops, ltimer_callback_fn_t user_callback,
                                     void *user_callback_token)
{
    gpt_config_t config = {
        .io_ops = io_ops,
        .user_callback = user_callback,
        .user_callback_token = user_callback_token,
        .device_path = GPT_PATH,
        .prescaler = GPT_PRESCALER
    };
    return gpt_init(&timers->timestamp, config);
}

static inline int imx_destroy_timestamp(imx_timers_t *timers)
{
    return gpt_destroy(&timers->timestamp);
}

static inline int imx_set_timeout(imx_timers_t *timers, uint64_t ns, bool periodic)
{
    return epit_set_timeout(&timers->timeout, ns, periodic);
}

static inline void imx_stop_timeout(imx_timers_t *timers)
{
    epit_stop(&timers->timeout);
}

static inline int imx_init_timeout(imx_timers_t *timers, ps_io_ops_t io_ops, ltimer_callback_fn_t user_callback,
                                   void *user_callback_token)
{
    epit_config_t config = {
        .io_ops = io_ops,
        .user_callback = user_callback,
        .user_callback_token = user_callback_token,
        .device_path = EPIT_PATH,
        .prescaler = 0
    };
    return epit_init(&timers->timeout, config);
}

static inline int imx_destroy_timeout(imx_timers_t *timers)
{
    return epit_destroy(&timers->timeout);
}
