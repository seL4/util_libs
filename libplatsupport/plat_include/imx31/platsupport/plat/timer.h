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

#include <autoconf.h>
#include <platsupport/gen_config.h>
#include <platsupport/mach/gpt.h>
#include <platsupport/mach/epit.h>

#define IPG_FREQ (500/8) /* 62.5MHz */
#define GPT_FREQ IPG_FREQ

#ifdef CONFIG_KERNEL_MCS
/* for RT, we use the EPIT as timestamp timer as the kernel is using the GPT */

typedef struct {
    epit_t timestamp;
    epit_t timeout;
} imx_timers_t;

static inline uint64_t imx_get_time(imx_timers_t *timers)
{
    return epit_get_time(&timers->timestamp);
}

static inline void imx_start_timestamp(imx_timers_t *timers)
{
    epit_set_timeout_ticks(&timers->timestamp, UINT32_MAX, true);
}

static inline void imx_stop_timestamp(imx_timers_t *timers)
{
    epit_stop(&timers->timestamp);
}

static inline int imx_init_timestamp(imx_timers_t *timers, ps_io_ops_t io_ops, ltimer_callback_fn_t user_callback,
                                     void *user_callback_token)
{
    epit_config_t config = {
        .io_ops = io_ops,
        .user_callback = user_callback,
        .user_callback_token = user_callback_token,
        .device_path = EPIT1_PATH,
        .is_timestamp = true,
        .prescaler = 0,
    };
    return epit_init(&timers->timestamp, config);
}

static inline int imx_destroy_timestamp(imx_timers_t *timers)
{
    return epit_destroy(&timers->timestamp);
}
#else
/* for baseline, the timestamp timer is the GPT as the kernel is using EPIT1 */

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
#endif

/* for both kernel versions, we use EPIT2 as the timeout timer */

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
        .device_path = EPIT2_PATH,
        .is_timestamp = false,
        .prescaler = 0
    };
    return epit_init(&timers->timeout, config);
}

static inline int imx_destroy_timeout(imx_timers_t *timers)
{
    return epit_destroy(&timers->timeout);
}
