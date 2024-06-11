/*
 * Copyright 2023, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/io.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>
#include <platsupport/pmem.h>
#include <platsupport/timer.h>
#include <utils/io.h>
#include <utils/util.h>

#include "../../ltimer.h"

enum {
    COUNTER_TIMER,
    TIMEOUT_TIMER,
    NUM_TIMERS
};

typedef struct {
    void *vaddr;
    starfive_timer_t timer[NUM_TIMERS];
    irq_id_t irq_id[NUM_TIMERS];
    timer_callback_data_t callback_data[NUM_TIMERS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} starfive_ltimer_t;

/* See timer.h for an explanation of the driver. */

/* Each channel IRQ is edge-triggered. */
static ps_irq_t irqs[] = {
    {
        .type = PS_TRIGGER,
        .trigger.number = STARFIVE_TIMER_CHANNEL_0_IRQ,
        .trigger.trigger = 1
    },
    {
        .type = PS_TRIGGER,
        .trigger.number = STARFIVE_TIMER_CHANNEL_1_IRQ,
        .trigger.trigger = 1
    },
};

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = STARFIVE_TIMER_BASE,
        .length = STARFIVE_TIMER_REGISTER_WINDOW_LEN_IN_BYTES,
    },
};

#define NUM_IRQS ARRAY_SIZE(irqs)
#define NUM_PMEMS ARRAY_SIZE(pmems)

static size_t get_num_irqs(void *data)
{
    return NUM_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < NUM_IRQS);
    assert(irq);

    *irq = irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return NUM_PMEMS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n < NUM_PMEMS);
    assert(paddr);

    *paddr = pmems[n];
    return 0;
}

static int ltimer_handle_irq(void *data, ps_irq_t *irq)
{
    assert(data);
    assert(irq);

    starfive_ltimer_t *timers = (starfive_ltimer_t *)data;
    long irq_number = irq->irq.number;
    ltimer_event_t event;

    if (irq_number == irqs[COUNTER_TIMER].irq.number) {
        starfive_timer_handle_irq(&timers->timer[COUNTER_TIMER]);
        event = LTIMER_OVERFLOW_EVENT;
    } else if (irq_number == irqs[TIMEOUT_TIMER].irq.number) {
        starfive_timer_handle_irq(&timers->timer[TIMEOUT_TIMER]);
        event = LTIMER_TIMEOUT_EVENT;
    } else {
        ZF_LOGE("Invalid IRQ number %ld received.", irq_number);
        return EINVAL;
    }

    if (timers->user_callback) {
        timers->user_callback(timers->user_callback_token, event);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data);
    assert(time);

    starfive_ltimer_t *timers = (starfive_ltimer_t *)data;
    *time = starfive_timer_get_time(&timers->timer[COUNTER_TIMER]);

    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data);
    starfive_ltimer_t *timers = (starfive_ltimer_t *)data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = starfive_timer_get_time(&timers->timer[COUNTER_TIMER]);
        if (time >= ns) {
            ZF_LOGE("Requested time %"PRIu64" earlier than current time %"PRIu64, ns, time);
            return ETIME;
        }
        return starfive_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return starfive_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, false);
    case TIMEOUT_PERIODIC:
        return starfive_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data);
    starfive_ltimer_t *timers = (starfive_ltimer_t *)data;

    starfive_timer_stop(&timers->timer[COUNTER_TIMER]);
    starfive_timer_reset(&timers->timer[COUNTER_TIMER]);
    starfive_timer_start(&timers->timer[COUNTER_TIMER]);

    starfive_timer_stop(&timers->timer[TIMEOUT_TIMER]);
    starfive_timer_reset(&timers->timer[TIMEOUT_TIMER]);

    return 0;
}

static void destroy(void *data)
{
    assert(data);
    starfive_ltimer_t *timers = (starfive_ltimer_t *)data;
    int error;

    starfive_timer_stop(&timers->timer[COUNTER_TIMER]);
    starfive_timer_stop(&timers->timer[TIMEOUT_TIMER]);

    ps_pmem_unmap(&timers->ops, pmems[0], timers->vaddr);

    error = ps_irq_unregister(&timers->ops.irq_ops, timers->irq_id[COUNTER_TIMER]);
    ZF_LOGE_IF(error, "Failed to uregister counter timer IRQ");

    error = ps_irq_unregister(&timers->ops.irq_ops, timers->irq_id[TIMEOUT_TIMER]);
    ZF_LOGE_IF(error, "Failed to uregister timeout timer IRQ");

    error = ps_free(&timers->ops.malloc_ops, sizeof(*timers), timers);
    ZF_LOGE_IF(error, "Failed to free device struct memory");
}

static int register_interrupt(ltimer_t *ltimer,
                              ps_io_ops_t ops,
                              starfive_ltimer_t *timers,
                              int id)
{
    assert(timers);
    assert(id >= 0 && id < NUM_TIMERS);

    timers->callback_data[id].ltimer = ltimer;
    timers->callback_data[id].irq = &irqs[id];
    timers->callback_data[id].irq_handler = ltimer_handle_irq;

    timers->irq_id[id] = ps_irq_register(&ops.irq_ops,
                                         irqs[id],
                                         handle_irq_wrapper,
                                         &timers->callback_data[id]);

    if (timers->irq_id[id] < 0) {
        ZF_LOGE("Unable to register irq %lu", irqs[id].trigger.number);
        return EINVAL;
    }

    return 0;
}

int ltimer_default_init(ltimer_t *ltimer,
                        ps_io_ops_t ops,
                        ltimer_callback_fn_t callback,
                        void *callback_token)
{
    assert(ltimer);

    ltimer->get_num_irqs = get_num_irqs;
    ltimer->get_nth_irq = get_nth_irq;
    ltimer->get_num_pmems = get_num_pmems;
    ltimer->get_nth_pmem = get_nth_pmem;
    ltimer->get_time = get_time;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(starfive_ltimer_t), &ltimer->data);
    if (error) {
        ZF_LOGE("Memory allocation failed with error %d", error);
        return error;
    }

    starfive_ltimer_t *timers = ltimer->data;

    timers->ops = ops;
    timers->user_callback = callback;
    timers->user_callback_token = callback_token;

    error = register_interrupt(ltimer, ops, timers, COUNTER_TIMER);
    if (error) {
        return error;
    }

    error = register_interrupt(ltimer, ops, timers, TIMEOUT_TIMER);
    if (error) {
        return error;
    }

    timers->vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (timers->vaddr == NULL) {
        ZF_LOGE("Unable to map physical memory at 0x%"PRIx64" length 0x%"PRIx64,
                pmems[0].base_addr,
                pmems[0].length);
        destroy(ltimer->data);
        return EINVAL;
    }

    starfive_timer_disable_all_channels(timers->vaddr);

    starfive_timer_init(&timers->timer[COUNTER_TIMER], timers->vaddr, COUNTER_TIMER);
    starfive_timer_start(&timers->timer[COUNTER_TIMER]);

    starfive_timer_init(&timers->timer[TIMEOUT_TIMER], timers->vaddr, TIMEOUT_TIMER);

    return 0;
}
