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

/* Implementation of a logical timer for beagle bone
 *
 * We use the 1 DMT for timeouts and 1 for a millisecond tick.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <utils/frequency.h>

#include <utils/util.h>

#include "../../ltimer.h"

#define TICK_DMT 0
#define TIMEOUT_DMT 1
#define NUM_DMTS 2

#define DMT_ID DMTIMER2

typedef struct {
    dmt_t dmts[NUM_DMTS];
    void *vaddrs[NUM_DMTS];
    irq_id_t timer_irq_ids[NUM_DMTS];
    timer_callback_data_t callback_datas[NUM_DMTS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
    uint32_t high_ticks;
} dmt_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return NUM_DMTS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));

    irq->type = PS_INTERRUPT;
    irq->irq.number = dmt_irq(n + DMT_ID);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return NUM_DMTS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    region->length = PAGE_SIZE_4K;
    region->base_addr = (uintptr_t) dmt_paddr(n + DMT_ID);
    return 0;
}

static int ltimer_handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;

    ltimer_event_t event;
    if (irq->irq.number == dmt_irq(DMT_ID + TICK_DMT)) {
        dmt_ltimer->high_ticks++;
        dmt_handle_irq(&dmt_ltimer->dmts[TICK_DMT]);
        event = LTIMER_OVERFLOW_EVENT;
    } else if (irq->irq.number == dmt_irq(DMT_ID + TIMEOUT_DMT)) {
        dmt_handle_irq(&dmt_ltimer->dmts[TIMEOUT_DMT]);
        event = LTIMER_TIMEOUT_EVENT;
    } else {
        ZF_LOGE("Unknown irq");
        return EINVAL;
    }

    if (dmt_ltimer->user_callback) {
        dmt_ltimer->user_callback(dmt_ltimer->user_callback_token, event);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    dmt_ltimer_t *dmt_ltimer = data;
    uint32_t high, low, high1;

    do {
        high  = dmt_ltimer->high_ticks;
        low   = dmt_get_time(&dmt_ltimer->dmts[TICK_DMT]);
        high1 = dmt_ltimer->high_ticks;
    } while (high != high1);

    uint64_t ticks = (((uint64_t)high << 32llu) | low);
    *time = freq_cycles_and_hz_to_ns(ticks, 24000000llu);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    *resolution = NS_IN_MS;
    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        uint64_t current_time = 0;
        int error = get_time(data, &current_time);
        assert(error == 0);
        assert(ns > current_time);
        ns -= current_time;
    }

    return dmt_set_timeout(&dmt_ltimer->dmts[TIMEOUT_DMT], ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    dmt_ltimer_t *dmt_ltimer = data;

    /* reset the timers */
    for (int i = 0; i < NUM_DMTS; i++) {
        dmt_stop(&dmt_ltimer->dmts[i]);
        dmt_start(&dmt_ltimer->dmts[i]);
    }
    return 0;
}

static void destroy(void *data)
{
    assert(data);

    dmt_ltimer_t *dmt_ltimer = data;

    for (int i = 0; i < NUM_DMTS; i++) {
        if (dmt_ltimer->vaddrs[i]) {
            dmt_stop(&dmt_ltimer->dmts[i]);
            ps_io_unmap(&dmt_ltimer->ops.io_mapper, dmt_ltimer->vaddrs[i], PAGE_SIZE_4K);
        }

        if (dmt_ltimer->callback_datas[i].irq) {
            ps_free(&dmt_ltimer->ops.malloc_ops, sizeof(ps_irq_t), dmt_ltimer->callback_datas[i].irq);
        }

        if (dmt_ltimer->timer_irq_ids[i] > PS_INVALID_IRQ_ID) {
            int error = ps_irq_unregister(&dmt_ltimer->ops.irq_ops, dmt_ltimer->timer_irq_ids[i]);
            ZF_LOGF_IF(error, "Failed to unregister timer IRQ");
        }
    }

    ps_free(&dmt_ltimer->ops.malloc_ops, sizeof(dmt_ltimer), dmt_ltimer);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(dmt_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    dmt_ltimer_t *dmt_ltimer = ltimer->data;
    dmt_ltimer->ops = ops;
    dmt_ltimer->user_callback = callback;
    dmt_ltimer->user_callback_token = callback_token;
    for (int i = 0; i < NUM_DMTS; i++) {
        dmt_ltimer->timer_irq_ids[i] = PS_INVALID_IRQ_ID;
    }

    /* map the frames we need */
    for (int i = 0; i < NUM_DMTS; i++) {
        pmem_region_t region;
        error = get_nth_pmem(ltimer->data, i, &region);
        assert(error == 0);
        dmt_ltimer->vaddrs[i] = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
        if (dmt_ltimer->vaddrs[i] == NULL) {
            error = -1;
            break;
        }

        dmt_config_t config = {
            .vaddr = dmt_ltimer->vaddrs[i],
            .id = DMT_ID + i
        };
        error = dmt_init(&dmt_ltimer->dmts[i], config);
        if (error) {
            break;
        }

        error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t), (void **) &dmt_ltimer->callback_datas[i].irq);
        if (error) {
            break;
        }
        dmt_ltimer->callback_datas[i].ltimer = ltimer;
        dmt_ltimer->callback_datas[i].irq_handler = ltimer_handle_irq;
        error = get_nth_irq(ltimer->data, i, dmt_ltimer->callback_datas[i].irq);
        assert(error == 0);
        dmt_ltimer->timer_irq_ids[i] = ps_irq_register(&ops.irq_ops, *dmt_ltimer->callback_datas[i].irq,
                                                       handle_irq_wrapper, &dmt_ltimer->callback_datas[i]);
        if (dmt_ltimer->timer_irq_ids[i] < 0) {
            error = EIO;
            break;
        }
    }

    /* start the timeout dmt */
    if (!error) {
        error = dmt_start(&dmt_ltimer->dmts[TIMEOUT_DMT]);
    }

    /* start the ticking dmt */
    if (!error) {
        error = dmt_start_ticking_timer(&dmt_ltimer->dmts[TICK_DMT]);
    }

    if (error) {
        destroy(dmt_ltimer);
    }

    return error;
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
