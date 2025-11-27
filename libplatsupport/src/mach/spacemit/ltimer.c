/*
 * Copyright 2025, 10xEngineers
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <platsupport/io.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/timer.h>
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
    spacemit_timer_t timer[NUM_TIMERS];
    irq_id_t irq_id[NUM_TIMERS];
    timer_callback_data_t callback_data[NUM_TIMERS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} spacemit_ltimer_t;

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .trigger.number = SPACEMIT_TIMER0_IRQ,
    },
    {
        .type = PS_INTERRUPT,
        .trigger.number = SPACEMIT_TIMER1_IRQ,
    },
};

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = SPACEMIT_TIMER_BASE,
        .length = SPACEMIT_TIMER_SIZE,
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

    spacemit_ltimer_t *timers = (spacemit_ltimer_t *)data;
    long irq_number = irq->irq.number;
    ltimer_event_t event;

    if (irq_number != irqs[0].irq.number && irq_number != irqs[1].irq.number) {
        ZF_LOGE("Invalid IRQ number %ld received.", irq_number);
        return EINVAL;
    }

    /* Check Timer Status reg */
    uint8_t int_status = timers->timer[COUNTER_TIMER].regs->tsr[COUNTER_TIMER];
    if (int_status & 0x1) {
        /* Ack the Interrupt / clear the Interrupt*/
        timers->timer[COUNTER_TIMER].regs->ticr[COUNTER_TIMER] |= (1u << 0);
        timers->timer[COUNTER_TIMER].value_h += 1;
        event = LTIMER_OVERFLOW_EVENT;
    }

    if (timers->user_callback) {
        timers->user_callback(timers->user_callback_token, event);
    }

    int_status = timers->timer[TIMEOUT_TIMER].regs->tsr[TIMEOUT_TIMER];
    if (int_status & 0x1) {
        /* Ack the Interrupt / clear the Interrupt*/
        timers->timer[TIMEOUT_TIMER].regs->ticr[TIMEOUT_TIMER] |= (1u << 0);
        timers->timer[TIMEOUT_TIMER].value_h += 1;
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

    spacemit_ltimer_t *timers = (spacemit_ltimer_t *)data;
    *time = spacemit_timer_get_time(&timers->timer[COUNTER_TIMER]);

    return 0;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data);
    spacemit_ltimer_t *timers = (spacemit_ltimer_t *)data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = spacemit_timer_get_time(&timers->timer[COUNTER_TIMER]);
        if (time >= ns) {
            ZF_LOGD("Requested time %"PRIu64" earlier than current time %"PRIu64, ns, time);
            return ETIME;
        }
        return spacemit_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return spacemit_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, false);
    case TIMEOUT_PERIODIC:
        return spacemit_timer_set_timeout(&timers->timer[TIMEOUT_TIMER], ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data);
    spacemit_ltimer_t *timers = (spacemit_ltimer_t *)data;

    spacemit_timer_disable(&timers->timer[COUNTER_TIMER]);
    spacemit_timer_reset(&timers->timer[COUNTER_TIMER]);
    spacemit_timer_enable(&timers->timer[COUNTER_TIMER]);

    spacemit_timer_disable(&timers->timer[TIMEOUT_TIMER]);
    spacemit_timer_reset(&timers->timer[TIMEOUT_TIMER]);

    return 0;
}

static void destroy(void *data)
{
    assert(data);
    spacemit_ltimer_t *timers = (spacemit_ltimer_t *)data;
    int error;

    spacemit_timer_disable(&timers->timer[COUNTER_TIMER]);
    spacemit_timer_disable(&timers->timer[TIMEOUT_TIMER]);

    ps_pmem_unmap(&timers->ops, pmems[0], timers->timer[COUNTER_TIMER].vaddr);

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

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(spacemit_ltimer_t), &ltimer->data);
    if (error) {
        ZF_LOGE("Memory allocation failed with error %d", error);
        return error;
    }
    spacemit_ltimer_t *timers = ltimer->data;

    timers->ops = ops;
    timers->user_callback = callback;
    timers->user_callback_token = callback_token;

    timers->vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (timers->vaddr == NULL) {
        ZF_LOGE("Unable to map physical memory at 0x%"PRIx64" length 0x%"PRIx64,
                pmems[0].base_addr,
                pmems[0].length);
        destroy(ltimer->data);
        return EINVAL;
    }

    for (size_t i = 0; i < NUM_TIMERS; i++) {

        timers->callback_data[i].ltimer = ltimer;
        timers->callback_data[i].irq = &irqs[i];
        timers->callback_data[i].irq_handler = ltimer_handle_irq;

        timers->irq_id[i] = ps_irq_register(&ops.irq_ops,
                                            irqs[i],
                                            handle_irq_wrapper,
                                            &timers->callback_data[i]);
        if (timers->irq_id[i] < 0) {
            destroy(ltimer->data);
            return EIO;
        }

        /*Timers share same region*/
        timers->timer[i].vaddr = timers->vaddr;
    }
    spacemit_timer_disable_all(timers->vaddr);

    spacemit_timer_init(&timers->timer[COUNTER_TIMER], timers->timer[COUNTER_TIMER].vaddr, COUNTER_TIMER);
    spacemit_timer_enable(&timers->timer[COUNTER_TIMER]);

    spacemit_timer_init(&timers->timer[TIMEOUT_TIMER], timers->timer[TIMEOUT_TIMER].vaddr, TIMEOUT_TIMER);
    return 0;
}
