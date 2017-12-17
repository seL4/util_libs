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
#include <platsupport/mach/gpt.h>
#include <platsupport/mach/epit.h>

#define IPG_FREQ (532/16) /*x10^5 */
#define GPT_FREQ IPG_FREQ

#ifdef CONFIG_KERNEL_RT
/* for RT, we use the EPIT as timestamp timer as the kernel is using the GPT */
#define TIMESTAMP_INTERRUPT EPIT1_INTERRUPT
#define TIMESTAMP_DEVICE_PADDR EPIT1_DEVICE_PADDR

typedef struct {
    epit_t timestamp;
    epit_t timeout;
    uint64_t high_bits;
} imx_timers_t;

static inline void handle_irq_timestamp(imx_timers_t *timers)
{
    timers->high_bits++;
    epit_handle_irq(&timers->timestamp);
}

static inline uint64_t imx_get_time(imx_timers_t *timers)
{
    uint64_t ticks = (timers->high_bits + !!epit_is_irq_raised(&timers->timestamp)) << 32llu;
    ticks += (UINT32_MAX - epit_read(&timers->timestamp));
    return epit_ticks_to_ns(&timers->timestamp, ticks);
}

static inline void imx_start_timestamp(imx_timers_t *timers)
{
    epit_set_timeout_ticks(&timers->timestamp, UINT32_MAX, true);
}

static inline void imx_stop_timestamp(imx_timers_t *timers)
{
    epit_stop(&timers->timestamp);
}

static inline int imx_init_timestamp(imx_timers_t *timers, void *vaddr)
{
    epit_config_t config = {
        .vaddr = vaddr,
        .irq = TIMESTAMP_INTERRUPT,
        .prescaler = 0
    };
    return epit_init(&timers->timestamp, config);
}

#else
/* for baseline, the timestamp timer is the GPT as the kernel is using EPIT1 */
#define TIMESTAMP_INTERRUPT GPT1_INTERRUPT
#define TIMESTAMP_DEVICE_PADDR GPT1_DEVICE_PADDR

typedef struct {
    gpt_t timestamp;
    epit_t timeout;
} imx_timers_t;

static inline void handle_irq_timestamp(imx_timers_t *timers)
{
    gpt_handle_irq(&timers->timestamp);
}

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

static inline int imx_init_timestamp(imx_timers_t *timers, void *vaddr)
{
    gpt_config_t config = {
        .vaddr = vaddr,
        .prescaler = GPT_PRESCALER
    };
    return gpt_init(&timers->timestamp, config);
}
#endif

/* for both kernel versions, we use EPIT2 as the timeout timer */
#define TIMEOUT_INTERRUPT EPIT2_INTERRUPT
#define TIMEOUT_DEVICE_PADDR EPIT2_DEVICE_PADDR

static inline void handle_irq_timeout(imx_timers_t *timers)
{
	epit_handle_irq(&timers->timeout);
}

static inline int imx_set_timeout(imx_timers_t *timers, uint64_t ns, bool periodic)
{
    return epit_set_timeout(&timers->timeout, ns, periodic);
}

static inline void imx_stop_timeout(imx_timers_t *timers)
{
    epit_stop(&timers->timeout);
}

static inline int imx_init_timeout(imx_timers_t *timers, void *vaddr)
{
    epit_config_t config = {
        .vaddr = vaddr,
        .irq = TIMEOUT_INTERRUPT,
        .prescaler = 0
    };
    return epit_init(&timers->timeout, config);
}
