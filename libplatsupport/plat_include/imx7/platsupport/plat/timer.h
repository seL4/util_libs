/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#define GPT_FREQ   (12u)
/* need to double check the value when the
 * clock control module is implemented.
 */
#define IPG_FREQ   (24u)

#include <platsupport/mach/gpt.h>

#define TIMESTAMP_INTERRUPT GPT1_INTERRUPT
#define TIMEOUT_INTERRUPT GPT2_INTERRUPT

#define TIMESTAMP_DEVICE_PADDR     GPT1_DEVICE_PADDR
#define TIMEOUT_DEVICE_PADDR     GPT2_DEVICE_PADDR

typedef struct {
    gpt_t timestamp;
    gpt_t timeout;
} imx_timers_t;

static inline uint64_t imx_get_time(imx_timers_t *timers)
{
    return gpt_get_time(&timers->timestamp);
}

static inline int imx_set_timeout(imx_timers_t *timers, uint64_t ns, bool periodic)
{
    return gpt_set_timeout(&timers->timeout, ns, periodic);
}

static inline void imx_start_timestamp(imx_timers_t *timers)
{
    gpt_start(&timers->timestamp);
}

static inline void imx_stop_timestamp(imx_timers_t *timers)
{
    gpt_stop(&timers->timestamp);
}

static inline void imx_stop_timeout(imx_timers_t *timers)
{
    gpt_stop(&timers->timeout);
}

static inline int imx_init_timer(gpt_t *gpt, ps_io_ops_t *io_ops, ltimer_callback_fn_t user_callback,
                                 void *user_callback_token, char *device_path)
{
    gpt_config_t config = {
        .io_ops = io_ops,
        .user_callback = user_callback,
        .user_callback_token = user_callback_token,
        .device_path = device_path,
        .prescaler = GPT_PRESCALER
    };
    return gpt_init(gpt, config);
}

static inline int imx_init_timestamp(imx_timers_t *timers, ps_io_ops_t *io_ops, ltimer_callback_fn_t user_callback,
                                     void *user_callback_token)
{
    return imx_init_timer(&timers->timestamp, io_ops, user_callback, user_callback_token, GPT1_PATH);
}

static inline int imx_destroy_timestamp(imx_timers_t *timers)
{
    return gpt_destroy(&timers->timestamp);
}

static inline int imx_init_timeout(imx_timers_t *timers, ps_io_ops_t *io_ops, ltimer_callback_fn_t user_callback,
                                   void *user_callback_token)
{
    return imx_init_timer(&timers->timeout, io_ops, user_callback, user_callback_token, GPT2_PATH);
}

static inline int imx_destroy_timeout(imx_timers_t *timers)
{
    return gpt_destroy(&timers->timestamp);
}
