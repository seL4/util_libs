/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * Copyright 2017, DornerWorks
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_DORNERWORKS_BSD)
 */
#pragma once
/*
   The IPG Freq is set by the SCU.
   Here, the peripheral clock for the gpts was set to 80Mhz
   In the GPT function we use the peripheral clock
*/
#define IPG_FREQ (80u)
#define GPT_FREQ IPG_FREQ

#include <platsupport/mach/gpt.h>

#define TIMESTAMP_INTERRUPT GPT1_INTERRUPT
#define TIMEOUT_INTERRUPT GPT2_INTERRUPT

#define TIMESTAMP_DEVICE_PADDR     GPT1_DEVICE_PADDR
#define TIMEOUT_DEVICE_PADDR     GPT2_DEVICE_PADDR

typedef struct {
    gpt_t timestamp;
    gpt_t timeout;
} imx_timers_t;

static inline void handle_irq_timestamp(imx_timers_t *timers)
{
    gpt_handle_irq(&timers->timestamp);
}

static inline void handle_irq_timeout(imx_timers_t *timers)
{
    gpt_handle_irq(&timers->timeout);
}

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

static inline int imx_init_timer(gpt_t *gpt, void *vaddr)
{
    gpt_config_t config = {
        .vaddr = vaddr,
        .prescaler = GPT_PRESCALER
    };
    return gpt_init(gpt, config);
}

static inline int imx_init_timestamp(imx_timers_t *timers, void *vaddr)
{
    return imx_init_timer(&timers->timestamp, vaddr);
}

static inline int imx_init_timeout(imx_timers_t *timers, void *vaddr)
{
    return imx_init_timer(&timers->timeout, vaddr);
}
