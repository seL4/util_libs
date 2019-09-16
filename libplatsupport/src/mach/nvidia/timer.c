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
#include <platsupport/plat/timer.h>

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

void nv_tmr_destroy(nv_tmr_t *tmr)
{
    if (tmr->reg_base) {
        ps_pmem_unmap(&tmr->ops, tmr->timer_pmem, (void *) tmr->reg_base);
    }

    if (tmr->irq_id > PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&tmr->ops.irq_ops, tmr->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister an IRQ");
    }
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
    tmr->tmr_map->pvt = BIT(PVT_E_BIT) | ticks
        | ((periodic) ? BIT(PVT_PERIODIC_E_BIT) : 0);
    return 0;
}

void nv_tmr_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    nv_tmr_t *tmr = data;
    tmr->tmr_map->pcr |= BIT(PCR_INTR_CLR_BIT);
    if (!(tmr->tmr_map->pvt & BIT(PVT_PERIODIC_E_BIT))) {
        tmr->tmr_map->pvt &= ~(BIT(PVT_E_BIT));
    }
    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (tmr->user_callback) {
        /* Call the ltimer's user callback */
        tmr->user_callback(tmr->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }
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

static int allocate_register_callback(pmem_region_t pmem, unsigned curr_num, size_t num_regs, void *token)
{
    assert(token != NULL);
    nv_tmr_t *tmr = token;
    /* There's only one register region to map, map it in */
    assert(num_regs == 1 && curr_num == 0);
    tmr->reg_base = (uintptr_t) ps_pmem_map(&tmr->ops, pmem, false, PS_MEM_NORMAL);
    if (tmr->reg_base == 0) {
        return EIO;
    }
    tmr->timer_pmem = pmem;
    return 0;
}

static int allocate_irq_callback(ps_irq_t irq, unsigned curr_num, size_t num_irqs, void *token)
{
    assert(token != NULL);
    nv_tmr_t *tmr = token;
    /* Skip all interrupts except the first */
    if (curr_num != 0) {
        return 0;
    }

    tmr->irq_id = ps_irq_register(&tmr->ops.irq_ops, irq, nv_tmr_handle_irq, tmr);
    if (tmr->irq_id < 0) {
        return EIO;
    }

    return 0;
}

int nv_tmr_init(nv_tmr_t *tmr, ps_io_ops_t ops, char *device_path, ltimer_callback_fn_t user_callback, void *user_callback_token)
{
    if (!tmr || !device_path) {
        return EINVAL;
    }

    /* setup the private structure */
    tmr->ops = ops;
    tmr->user_callback = user_callback;
    tmr->user_callback_token = user_callback_token;
    tmr->irq_id = PS_INVALID_IRQ_ID;

    /* read the timer's path in the DTB */
    ps_fdt_cookie_t *cookie = NULL;
    int error = ps_fdt_read_path(&ops.io_fdt, &ops.malloc_ops, device_path, &cookie);
    if (error) {
        nv_tmr_destroy(tmr);
        return ENODEV;
    }

    /* walk the registers and allocate them */
    error = ps_fdt_walk_registers(&ops.io_fdt, cookie, allocate_register_callback, tmr);
    if (error) {
        nv_tmr_destroy(tmr);
        return ENODEV;
    }

    /* walk the interrupts and allocate the first */
    error = ps_fdt_walk_irqs(&ops.io_fdt, cookie, allocate_irq_callback, tmr);
    if (error) {
        nv_tmr_destroy(tmr);
        return ENODEV;
    }

    error = ps_fdt_cleanup_cookie(&ops.malloc_ops, cookie);
    if (error) {
        nv_tmr_destroy(tmr);
        return ENODEV;
    }

    tmr->tmr_map = (void *)(tmr->reg_base + NV_TMR_ID_OFFSET);
    tmr->tmrus_map = (void *)(tmr->reg_base + TMRUS_OFFSET);
    tmr->tmr_shared_map = (void *) tmr->reg_base + TMR_SHARED_OFFSET;

    tmr->tmr_map->pvt = 0;
    tmr->tmr_map->pcr = BIT(PCR_INTR_CLR_BIT);

/* The following #ifdef is for some platform specific init for tk1, tx1, tx2
 * currently this custom init is only one line each hence using the ifdef.
 * If the platform specific code becomes any larger, then it should be considered
 * moving into a per platform nv_timer_plat_init function
 */
#ifdef CONFIG_PLAT_TX2
    /* Route the interrupt to the correct shared interrupt number. */
    tmr->tmr_shared_map->TKEIE[NV_TMR_ID] = BIT(NV_TMR_ID);
#else
    /* Just unconditionally set the divisor as if "clk_m" is always 12MHz,
     * because it actually is always 12MHz on TK1 or TX1.
     *
     * Nvidia manual, section 5.2.2, Table 14:
     * "clk_m: This clock (with DFT control) runs at 12 MHz, 13 MHz, 16.8 MHz,
     * 19.2 MHz, 26 MHz, 38.4 MHz, or 48 MHz. Only 12 MHz is currently
     * supported."
     */
    tmr->tmrus_map->usec_cfg = TMRUS_USEC_CFG_DEFAULT;
#endif

    return 0;
}
