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

/* use the GPT as the timestamp timer */
#define TIMESTAMP_INTERRUPT GPT1_INTERRUPT
#define TIMESTAMP_DEVICE_PADDR GPT1_DEVICE_PADDR
/* use EPIT2 as the timeout timer */
#define TIMEOUT_INTERRUPT EPIT2_INTERRUPT
#define TIMEOUT_DEVICE_PADDR EPIT2_DEVICE_PADDR

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
