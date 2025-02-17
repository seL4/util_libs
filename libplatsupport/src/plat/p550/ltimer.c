/*
 * Copyright 2025, UNSW
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
    eswin_timer_t timer[NUM_TIMERS];
    irq_id_t irq_id[NUM_TIMERS];
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} eswin_ltimer_t;

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .trigger.number = ESWIN_TIMER_IRQ,
    },
};

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = ESWIN_TIMER_BASE,
        .length = ESWIN_TIMER_SIZE,
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

    eswin_ltimer_t *timers = (eswin_ltimer_t *)data;
    long irq_number = irq->irq.number;
    ltimer_event_t event;

    if (irq_number != irqs[0].irq.number) {
        ZF_LOGE("Invalid IRQ number %ld received.", irq_number);
        return EINVAL;
    }

    if (timers->timer[COUNTER_TIMER].regs->int_status & 0x1) {
        eswin_timer_handle_irq(&timers->timer[COUNTER_TIMER]);
        event = LTIMER_OVERFLOW_EVENT;
    }

    if (timers->user_callback) {
        timers->user_callback(timers->user_callback_token, event);
    }

    if (timers->timer[TIMEOUT_TIMER].regs->int_status & 0x1) {
        eswin_timer_handle_irq(&timers->timer[TIMEOUT_TIMER]);
        event = LTIMER_TIMEOUT_EVENT;
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

    eswin_ltimer_t *timers = (eswin_ltimer_t *)data;
    *time = eswin_timer_get_time(&timers->timer[COUNTER_TIMER]);

    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data);
    eswin_ltimer_t *timers = (eswin_ltimer_t *)data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = eswin_timer_get_time(&timers->timer[COUNTER_TIMER]);
        if (time >= ns) {
            ZF_LOGE("Requested time %"PRIu64" earlier than current time %"PRIu64, ns, time);
            return ETIME;
        }
        return eswin_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return eswin_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, false);
    case TIMEOUT_PERIODIC:
        return eswin_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data);
    eswin_ltimer_t *timers = (eswin_ltimer_t *)data;

    eswin_timer_disable(&timers->timer[COUNTER_TIMER]);
    eswin_timer_reset(&timers->timer[COUNTER_TIMER]);
    eswin_timer_enable(&timers->timer[COUNTER_TIMER]);

    eswin_timer_disable(&timers->timer[TIMEOUT_TIMER]);
    eswin_timer_reset(&timers->timer[TIMEOUT_TIMER]);

    return 0;
}

static void destroy(void *data)
{
    assert(data);
    eswin_ltimer_t *timers = (eswin_ltimer_t *)data;
    int error;

    eswin_timer_disable(&timers->timer[COUNTER_TIMER]);
    eswin_timer_disable(&timers->timer[TIMEOUT_TIMER]);

    ps_pmem_unmap(&timers->ops, pmems[0], timers->vaddr);

    error = ps_irq_unregister(&timers->ops.irq_ops, timers->irq_id[COUNTER_TIMER]);
    ZF_LOGE_IF(error, "Failed to uregister counter timer IRQ");

    error = ps_irq_unregister(&timers->ops.irq_ops, timers->irq_id[TIMEOUT_TIMER]);
    ZF_LOGE_IF(error, "Failed to uregister timeout timer IRQ");

    error = ps_free(&timers->ops.malloc_ops, sizeof(*timers), timers);
    ZF_LOGE_IF(error, "Failed to free device struct memory");
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

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(eswin_ltimer_t), &ltimer->data);
    if (error) {
        ZF_LOGE("Memory allocation failed with error %d", error);
        return error;
    }

    eswin_ltimer_t *timers = ltimer->data;

    timers->ops = ops;
    timers->user_callback = callback;
    timers->user_callback_token = callback_token;

    timers->callback_data.ltimer = ltimer;
    timers->callback_data.irq = &irqs[0];
    timers->callback_data.irq_handler = ltimer_handle_irq;

    irq_id_t irq_id = ps_irq_register(&ops.irq_ops,
                                      irqs[0],
                                      handle_irq_wrapper,
                                      &timers->callback_data);
    assert(irq_id >= 0);

    timers->vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (timers->vaddr == NULL) {
        ZF_LOGE("Unable to map physical memory at 0x%"PRIx64" length 0x%"PRIx64,
                pmems[0].base_addr,
                pmems[0].length);
        destroy(ltimer->data);
        return EINVAL;
    }

    eswin_timer_disable_all(timers->vaddr);

    eswin_timer_init(&timers->timer[COUNTER_TIMER], timers->vaddr, COUNTER_TIMER);
    eswin_timer_enable(&timers->timer[COUNTER_TIMER]);

    eswin_timer_init(&timers->timer[TIMEOUT_TIMER], timers->vaddr, TIMEOUT_TIMER);

    return 0;
}
