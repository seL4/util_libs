/*
 * Copyright 2020, DornerWorks
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <utils/util.h>
#include <platsupport/timer.h>
#include <platsupport/plat/mstimer.h>
#include <utils/frequency.h>

#include "../../ltimer.h"

#define MAX_TIMEOUT_NS (BIT(31) * (NS_IN_S / POLARFIRE_PCLK))

static inline void mstimer_print_regs(mstimer_t *mstimer)
{
    printf("MSTIMER @ %p\n", mstimer->register_map);
    printf("________________________________________\n");
    printf("Value            : 0x%08x @ [%p]\n",
           mstimer->register_map->value,
           &mstimer->register_map->value);
    printf("Load Value       : 0x%08x @ [%p]\n",
           mstimer->register_map->load_value,
           &mstimer->register_map->load_value);
    printf("Bg Load Value    : 0x%08x @ [%p]\n",
           mstimer->register_map->bg_load_value,
           &mstimer->register_map->bg_load_value);
    printf("Control          : 0x%08x @ [%p]\n",
           mstimer->register_map->control,
           &mstimer->register_map->control);
    printf("Raw Int Status   : 0x%08x @ [%p]\n",
           mstimer->register_map->raw_interrupt_status,
           &mstimer->register_map->raw_interrupt_status);
    printf("Masked Int Status: 0x%08x @ [%p]\n",
           mstimer->register_map->masked_interrupt_status,
           &mstimer->register_map->masked_interrupt_status);
    printf("________________________________________\n");
}

int mstimer_start(mstimer_t *mstimer)
{

    if (mstimer == NULL) {
        return EINVAL;
    }

    mstimer->register_map->control |= MS_TIMER;

    return 0;
}

int mstimer_stop(mstimer_t *mstimer)
{
    if (mstimer == NULL) {
        return EINVAL;
    }
    mstimer->register_map->control &= ~MS_TIMER;

    return 0;
}

void mstimer_handle_irq(mstimer_t *mstimer, uint32_t irq)
{
    if (mstimer->register_map->raw_interrupt_status) {
        mstimer->time_h ++;
        /* Write 1 to clear */
        mstimer->register_map->raw_interrupt_status = 1;
    } else if (mstimer->register_map->masked_interrupt_status) {
        mstimer->time_h ++;
        /* Write 1 to clear */
        mstimer->register_map->raw_interrupt_status = 1;
    }
}

uint64_t mstimer_get_time(mstimer_t *mstimer)
{
    if (mstimer == NULL) {
        return EINVAL;
    }

    uint64_t timer_value = (uint64_t)(mstimer->register_map->value);
    uint64_t timer_h_ns = (uint64_t)mstimer->time_h * NS_IN_S;
    uint64_t timer_l_ns = (uint64_t)(UINT32_MAX - timer_value) * ((NS_IN_S) / POLARFIRE_PCLK);

    return timer_h_ns + timer_l_ns;
}
int mstimer_set_timeout(mstimer_t *mstimer, uint64_t ns, bool periodic)
{
    /* Clear the state of the timer */
    mstimer_stop(mstimer);
    mstimer->time_h = 0;
    if (periodic) {
        // Periodic
        mstimer->register_map->control &= ~MS_TIMER_MODE;
    } else {
        // One Shot
        mstimer->register_map->control |= MS_TIMER_MODE;
    }

    size_t num_ticks = ns / (NS_IN_S / POLARFIRE_PCLK);
    if (num_ticks > MAX_TIMEOUT_NS) {
        ZF_LOGE("Cannot program a timeout larger than the %ld ns", MAX_TIMEOUT_NS);
        return -EINVAL;
    }
    mstimer->register_map->raw_interrupt_status = 1;
    mstimer->register_map->load_value = num_ticks;
    mstimer_start(mstimer);

    return 0;
}

int mstimer_reset(mstimer_t *mstimer)
{
    if (mstimer == NULL) {
        return EINVAL;
    }
    mstimer->register_map->load_value = UINT32_MAX;
    // Interrupt enabled in periodic mode
    mstimer->register_map->control = MS_TIMER_INT;
    mstimer->register_map->raw_interrupt_status = 1;

    return 0;
}

int mstimer_init(mstimer_t *mstimer, ps_io_ops_t ops, mstimer_config_t mstimer_config)
{
    mstimer->register_map = (volatile struct mstimer_map *)(mstimer_config.base_vaddr +
                                                            mstimer_config.base_address_offset);

    mstimer->ops = ops;

    /* Configure initial state of timer */

    // Interrupt enabled in periodic mode
    mstimer->register_map->control = MS_TIMER_INT;
    mstimer->register_map->load_value = UINT32_MAX;
    mstimer->register_map->bg_load_value = UINT32_MAX;

    return 0;
}
