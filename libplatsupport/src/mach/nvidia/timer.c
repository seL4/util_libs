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

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <utils/util.h>
#include <inttypes.h>

#include <platsupport/timer.h>
#include <platsupport/mach/timer.h>

/* enable bit */
#define PVT_E_BIT           31

/* enable auto-reload for periodic mode */
#define PVT_PERIODIC_E_BIT  30

/* 0-28 bits are for value, n + 1 trigger mode */
#define PVT_VAL_MASK        0x1fffffff
#define PVT_VAL_BITS        29

/* write 1 to clear interruts */
#define PCR_INTR_CLR_BIT    30

/* counter value: decrements from PVT */
#define PCR_VAL_MASK        0x1fffffff
/* counter width is 29 bits */

/* The NV-TMR timers are based on a 1us upcounter. This 1us upcounter literally
 * counts up once every 1us, so the frequency here can only be 1us.
 *
 * You are required to initialize the upcounter with a divisor that will cause
 * it to count up at 1us, and no other upcount value is supported.
 *
 * We set the divisor below in nv_get_timer(). The divisor divides the
 * "clk_m" input clock (which operates at 12MHz) down to 1us.
 */
#define CLK_FREQ_MHZ        1
#define CLK_FREQ_HZ         CLK_FREQ_MHZ * US_IN_S

int nv_tmr_start(nv_tmr_t *tmr)
{
    tmr->tmr_map->pcr |= BIT(PCR_INTR_CLR_BIT);
    tmr->tmr_map->pvt |= BIT(PVT_E_BIT);
    return 0;
}

int nv_tmr_stop(nv_tmr_t *tmr)
{
    tmr->tmr_map->pvt &= ~(BIT(PVT_E_BIT));
    return 0;
}

#define INVALID_PVT_VAL 0x80000000

static uint32_t
get_ticks(uint64_t ns)
{
    uint64_t microsecond = ns / 1000ull;
    uint64_t ticks = microsecond * CLK_FREQ_MHZ;
    if (ticks >= BIT(PVT_VAL_BITS)) {
        ZF_LOGE("ns too high %"PRIu64"\n", ns);
        return INVALID_PVT_VAL;
    }
    return (uint32_t)ticks;
}

int nv_tmr_set_timeout(nv_tmr_t *tmr, bool periodic, uint64_t ns)
{
    uint32_t ticks = get_ticks(ns);
    if (ticks == INVALID_PVT_VAL) {
        ZF_LOGE("Invalid PVT val");
        return EINVAL;
    }
    /* ack any pending irqs */
    tmr->tmr_map->pcr |= BIT(PCR_INTR_CLR_BIT);
    tmr->tmr_map->pvt = BIT(PVT_E_BIT) | ticks;
    return 0;
}

void nv_tmr_handle_irq(nv_tmr_t *tmr)
{
    tmr->tmr_map->pcr |= BIT(PCR_INTR_CLR_BIT);
    tmr->tmr_map->pvt &= ~(BIT(PVT_E_BIT));
}

uint64_t nv_tmr_get_time(nv_tmr_t *tmr)
{
    return (uint64_t)tmr->tmrus_map->cntr_1us * NS_IN_US;
}

long nv_tmr_get_irq(nv_tmr_id_t n)
{
    switch (n) {
        case TMR0:
            return INT_NV_TMR0;
        case TMR1:
            return INT_NV_TMR1;
        case TMR2:
            return INT_NV_TMR2;
        case TMR3:
            return INT_NV_TMR3;
        case TMR4:
            return INT_NV_TMR4;
        case TMR5:
            return INT_NV_TMR5;
        case TMR6:
            return INT_NV_TMR6;
        case TMR7:
            return INT_NV_TMR7;
        case TMR8:
            return INT_NV_TMR8;
        case TMR9:
            return INT_NV_TMR9;
        default:
            ZF_LOGE("invalid timer id %d\n", n);
            return 0;
    }
}

#define TMRUS_USEC_CFG_DEFAULT   11

int nv_tmr_init(nv_tmr_t *tmr, nv_tmr_config_t config)
{
    if (config.id < TMR0 || config.id > TMR_LAST) {
        return EINVAL;
    }

    uintptr_t offset;
    switch (config.id)
    {
    case TMR0:
        offset = TMR0_OFFSET;
        break;
    case TMR1:
        offset = TMR1_OFFSET;
        break;
    case TMR2:
        offset = TMR2_OFFSET;
        break;
    case TMR3:
        offset = TMR3_OFFSET;
        break;
    case TMR4:
        offset = TMR4_OFFSET;
        break;
    case TMR5:
        offset = TMR5_OFFSET;
        break;
    case TMR6:
        offset = TMR6_OFFSET;
        break;
    case TMR7:
        offset = TMR7_OFFSET;
        break;
    case TMR8:
        offset = TMR8_OFFSET;
        break;
    case TMR9:
        offset = TMR9_OFFSET;
        break;
    };

    tmr->tmr_map = (void *) (config.vaddr + offset);
    tmr->tmrus_map = (void *) (config.vaddr + TMRUS_OFFSET);
    tmr->tmr_shared_map = (void *) config.vaddr;

    tmr->tmr_map->pvt = 0;
    tmr->tmr_map->pcr = BIT(PCR_INTR_CLR_BIT);

    /* Just unconditionally set the divisor as if "clk_m" is always 12MHz,
     * because it actually is always 12MHz.
     *
     * Nvidia manual, section 5.2.2, Table 14:
     * "clk_m: This clock (with DFT control) runs at 12 MHz, 13 MHz, 16.8 MHz,
     * 19.2 MHz, 26 MHz, 38.4 MHz, or 48 MHz. Only 12 MHz is currently
     * supported."
     */
    tmr->tmrus_map->usec_cfg = TMRUS_USEC_CFG_DEFAULT;

    return 0;
}
