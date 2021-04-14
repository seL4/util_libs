/*
 * Copyright 2017, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/spt.h>
#include <platsupport/plat/system_timer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#include "../../ltimer.h"

#define NV_TMR_ID TMR0

enum {
    SP804_TIMER = 0,
    SYSTEM_TIMER = 1,
    NUM_TIMERS = 2,
};

typedef struct {
    spt_t spt;
    system_timer_t system;
    void *timer_vaddrs[NUM_TIMERS];
    irq_id_t timer_irq_ids[NUM_TIMERS];
    timer_callback_data_t callback_datas[NUM_TIMERS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
    uint64_t period;
} spt_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = SP804_TIMER_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = SYSTEM_TIMER_PADDR,
        .length = PAGE_SIZE_4K
    }
};

size_t get_num_irqs(void *data)
{
    return NUM_TIMERS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    switch (n) {
    case SP804_TIMER:
        irq->irq.number = SP804_TIMER_IRQ;
        irq->type = PS_INTERRUPT;
        break;
    case SYSTEM_TIMER:
        irq->irq.number = SYSTEM_TIMER_MATCH_IRQ(SYSTEM_TIMER_MATCH);
        irq->type = PS_INTERRUPT;
        break;
    }
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return sizeof(pmems) / sizeof(pmems[0]);
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n < get_num_pmems(data));
    *paddr = pmems[n];
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;

    switch (irq->irq.number) {
    case SP804_TIMER_IRQ:
        spt_handle_irq(&spt_ltimer->spt);
        if (spt_ltimer->period > 0) {
            spt_set_timeout(&spt_ltimer->spt, spt_ltimer->period);
        }
        break;
    case SYSTEM_TIMER_MATCH_IRQ(SYSTEM_TIMER_MATCH):
        system_timer_handle_irq(&spt_ltimer->system);
        break;
    default:
        return EINVAL;
    }

    /* Both IRQs correspond to timeouts, the timestamp is not expected to roll over
     * in any reasonable timeframe */
    ltimer_event_t event = LTIMER_TIMEOUT_EVENT;
    if (spt_ltimer->user_callback) {
        spt_ltimer->user_callback(spt_ltimer->user_callback_token, event);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    spt_ltimer_t *spt_ltimer = data;
    *time = system_timer_get_time(&spt_ltimer->system);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_ltimer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        return system_timer_set_timeout(&spt_ltimer->system, ns);
    case TIMEOUT_PERIODIC:
        spt_ltimer->period = ns;
    /* fall through */
    case TIMEOUT_RELATIVE:
        return spt_set_timeout(&spt_ltimer->spt, ns);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_stop(&spt_ltimer->spt);
    spt_start(&spt_ltimer->spt);
    system_timer_reset(&spt_ltimer->system);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    spt_ltimer_t *spt_ltimer = data;
    for (int i = 0; i < NUM_TIMERS; i++) {
        if (i == SP804_TIMER) {
            spt_stop(&spt_ltimer->spt);
        }
        ps_pmem_unmap(&spt_ltimer->ops, pmems[i], spt_ltimer->timer_vaddrs[i]);

        if (spt_ltimer->callback_datas[i].irq) {
            ps_free(&spt_ltimer->ops.malloc_ops, sizeof(ps_irq_t), spt_ltimer->callback_datas[i].irq);
        }

        if (spt_ltimer->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            ps_irq_unregister(&spt_ltimer->ops.irq_ops, spt_ltimer->timer_irq_ids[i]);
        }
    }
    ps_free(&spt_ltimer->ops.malloc_ops, sizeof(spt_ltimer), spt_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{

    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(spt_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    spt_ltimer_t *spt_ltimer = ltimer->data;
    spt_ltimer->ops = ops;
    spt_ltimer->user_callback = callback;
    spt_ltimer->user_callback_token = callback_token;
    for (int i = 0; i < NUM_TIMERS; i++) {
        spt_ltimer->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    for (int i = 0; i < NUM_TIMERS; i++) {
        /* map the registers we need */
        spt_ltimer->timer_vaddrs[i] = ps_pmem_map(&ops, pmems[i], false, PS_MEM_NORMAL);
        if (spt_ltimer->timer_vaddrs[i] == NULL) {
            destroy(ltimer->data);
            return ENOMEM;
        }

        /* register the IRQs we need */
        error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t), (void **) &spt_ltimer->callback_datas[i].irq);
        if (error) {
            destroy(ltimer->data);
            return error;
        }
        spt_ltimer->callback_datas[i].ltimer = ltimer;
        spt_ltimer->callback_datas[i].irq_handler = handle_irq;
        error = get_nth_irq(ltimer->data, i, spt_ltimer->callback_datas[i].irq);
        if (error) {
            destroy(ltimer->data);
            return error;
        }
        spt_ltimer->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, *spt_ltimer->callback_datas[i].irq,
                                                       handle_irq_wrapper, &spt_ltimer->callback_datas[i]);
        if (spt_ltimer->timer_irq_ids[i] < 0) {
            destroy(ltimer->data);
            return EIO;
        }
    }

    /* setup spt */
    spt_config_t spt_config = {
        .vaddr = spt_ltimer->timer_vaddrs[SP804_TIMER],
    };

    spt_init(&spt_ltimer->spt, spt_config);
    spt_start(&spt_ltimer->spt);

    /* Setup system timer */
    system_timer_config_t system_config = {
        .vaddr = spt_ltimer->timer_vaddrs[SYSTEM_TIMER],
    };

    system_timer_init(&spt_ltimer->system, system_config);

    return 0;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = get_nth_pmem;
    return 0;
}
