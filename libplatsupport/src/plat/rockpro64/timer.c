/*
 * Copyright 2019, Data61
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
#include <utils/util.h>
#include <platsupport/timer.h>
#include <platsupport/plat/timer.h>
#include <utils/frequency.h>

#include "../../ltimer.h"

#define USER_MODE BIT(1)
#define UNMASKED_INT BIT(2)
#define TCLR_STARTTIMER BIT(0)
#define TISR_IRQ_CLEAR BIT(0)

//debug method
static void print_regs(rk_t *rk)
{
    printf("load_count0          >> 0x%08x\n", rk->hw->load_count0);
    printf("load_count1          >> 0x%08x\n", rk->hw->load_count1);
    printf("current_cnt_lowbits  >> 0x%08x\n", rk->hw->current_value0);
    printf("current_cnt_highbits >> 0x%08x\n", rk->hw->current_value1);
    printf("load_count2          >> 0x%08x\n", rk->hw->load_count2);
    printf("load_count3          >> 0x%08x\n", rk->hw->load_count3);
    printf("interrupt_status     >> 0x%08x\n", rk->hw->interrupt_status);
    printf("control_register     >> 0x%08x\n", rk->hw->control_register);
}

int rk_stop(rk_t *rk)
{
    if (rk == NULL) {
        return EINVAL;
    }

    rk->hw->control_register = 0;
    return 0;
}

uint64_t rk_get_time(rk_t *rk)
{
    if (rk == NULL) {
        return EINVAL;
    }
    uint32_t val1 = rk->hw->current_value1;
    uint32_t val2 = rk->hw->current_value0;
    if (val1 != rk->hw->current_value1) {
        val1 = rk->hw->current_value1;
        val2 = rk->hw->current_value0;
    }

    uint64_t time = 0;
    time = val1;
    time <<= 32;
    time |= val2;
    return ((uint64_t)((time) * NS_IN_S) / 24000000ull);
}

int rk_start_timestamp_timer(rk_t *rk)
{
    assert(rk != NULL);
    assert(rk->user_cb_event == LTIMER_OVERFLOW_EVENT);

    rk->hw->control_register = 0;

    //set timer to count up monotonically
    rk->hw->load_count0  = 0xffffffff;
    rk->hw->load_count1  = 0xffffffff;

    rk->hw->control_register |= UNMASKED_INT | TCLR_STARTTIMER;
    return 0;
}

int rk_set_timeout(rk_t *rk, uint64_t ns, bool periodic)
{
    if (rk == NULL) {
        return EINVAL;
    }
    /* disable timer */
    rk->hw->control_register = 0;

    /* timer mode */
    uint32_t tclrFlags = periodic ? 0 : USER_MODE;

    /* load timer count */
    uint64_t ticks = freq_ns_and_hz_to_cycles(ns, 24000000ull);
    rk->hw->load_count0  = (uint32_t)(ticks & 0xffffffff);
    rk->hw->load_count1  = (ticks >> 32);

    /* enable timer with configs */
    rk->hw->control_register |= TCLR_STARTTIMER | UNMASKED_INT | tclrFlags;
    return 0;
}

static void rk_handle_irq(void *data, ps_irq_acknowledge_fn_t acknowledge_fn, void *ack_data)
{
    assert(data != NULL);
    rk_t *rk = data;

    /* ack any pending irqs */
    rk->hw->interrupt_status = 1;

    ZF_LOGF_IF(acknowledge_fn(ack_data), "Failed to acknowledge the timer's interrupts");
    if (rk->user_cb_fn) {
        rk->user_cb_fn(rk->user_cb_token, rk->user_cb_event);
    }
}

bool rk_pending_match(rk_t *rk)
{
    return rk->hw->interrupt_status & TISR_IRQ_CLEAR;
}

void rk_destroy(rk_t *rk)
{
    int error;
    if (rk->irq_id != PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&rk->ops.irq_ops, rk->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    if (rk->hw != NULL) {
        rk_stop(rk);
    }
    if (rk->rk_map_base != NULL) {
        /* use base because rk_map is adjusted based on whether secondary */
        ps_pmem_unmap(&rk->ops, rk->pmem, (void *) rk->rk_map_base);
    }
}

int rk_init(rk_t *rk, ps_io_ops_t ops, rk_config_t config)
{
    int error;

    if (rk == NULL) {
        ZF_LOGE("rk cannot be null");
        return EINVAL;
    }

    rk->ops = ops;
    rk->user_cb_fn = config.user_cb_fn;
    rk->user_cb_token = config.user_cb_token;
    rk->user_cb_event = config.user_cb_event;

    error = helper_fdt_alloc_simple(
                &ops, config.fdt_path,
                RK_REG_CHOICE, RK_IRQ_CHOICE,
                &rk->rk_map_base, &rk->pmem, &rk->irq_id,
                rk_handle_irq, rk
            );
    if (error) {
        ZF_LOGE("Simple fdt alloc helper failed");
        return error;
    }

    rk->hw = rk->rk_map_base;

    return 0;
}

/* initialise rk using the base address of rkp, so that we do not attempt to map
 * that base address again but instead re-use it. */
int rk_init_secondary(rk_t *rk, rk_t *rkp, ps_io_ops_t ops, rk_config_t config)
{
    int error;

    if (rk == NULL || rkp == NULL) {
        ZF_LOGE("rk or rkp cannot be null");
        return EINVAL;
    }

    rk->ops = ops;
    rk->user_cb_fn = config.user_cb_fn;
    rk->user_cb_token = config.user_cb_token;
    rk->user_cb_event = config.user_cb_event;

    /* so that destroy does not try to unmap twice */
    rk->rk_map_base = NULL;
    /* just like dmt, rockpro64 has another timer in the same page at 0x20 offset */
    rk->hw = (void *)((uintptr_t) rkp->rk_map_base) + 0x20;
    /* similarly, the IRQ for this secondary timer is offset by 1 */
    rk->irq_id = rkp->irq_id + 1;

    return 0;
}
