/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

/*
 * Implementation of a logical timer for Microchip Polarfire Icicle platform.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/pmem.h>
#include <platsupport/plat/mstimer.h>
#include <utils/util.h>

#include "../../ltimer.h"

enum {
    TIMEOUT_TIMER,
    TIMESTAMP_TIMER,
    NUM_TIMERS
};

#define TIMEOUT_ADDR_OFFSET   MSTIMER1_OFFSET
#define TIMESTAMP_ADDR_OFFSET MSTIMER2_OFFSET


typedef struct {
    void *base_address;
    mstimer_t timeout;
    mstimer_t timestamp;
    ps_io_ops_t ops;
    timer_callback_data_t callback_data[NUM_TIMERS];
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
} polarfire_timers_t;

static ps_irq_t irqs[] = {
    {
        /* Timeout */
        .type = PS_INTERRUPT,
        .irq.number = MSTIMER_TIMER1INT
    },
    {
        /* Timestamp */
        .type = PS_INTERRUPT,
        .irq.number = MSTIMER_TIMER2INT
    },
};

static pmem_region_t timer_pmem = {
    .type = PMEM_TYPE_DEVICE,
    .base_addr = MSTIMER1_PADDR,
    .length = PAGE_SIZE_4K
};


#define N_IRQS ARRAY_SIZE(irqs)
#define N_PMEMS 1

size_t get_num_irqs(void *data)
{
    return N_IRQS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    if (n >= N_IRQS) {
        ZF_LOGE("IRQ requested out of range");
        return EINVAL;
    }

    *irq = irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return N_PMEMS;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    if (n == 0) {
        *paddr = timer_pmem;
    } else {
        ZF_LOGE("PMEM requested out of range");
        return EINVAL;
    }
    return 0;
}

static int ltimer_handle_irq(void *data, ps_irq_t *irq)
{
    /* Ack irq */
    assert(data != NULL);

    polarfire_timers_t *timers = data;
    long irq_number = irq->irq.number;
    ltimer_event_t event;

    if (irq_number == irqs[TIMESTAMP_TIMER].irq.number) {
        mstimer_handle_irq(&timers->timestamp, irq_number);
        event = LTIMER_OVERFLOW_EVENT;
    } else if (irq_number == irqs[TIMEOUT_TIMER].irq.number) {
        mstimer_handle_irq(&timers->timeout, irq_number);
        event = LTIMER_TIMEOUT_EVENT;
    } else {
        ZF_LOGE("Invalid IRQ number: %ld received.\n", irq_number);
        return EINVAL;
    }

    if (timers->user_callback) {
        timers->user_callback(timers->user_callback_token, event);
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{

    assert(data != NULL);
    assert(time != NULL);
    polarfire_timers_t *timers = data;

    *time = mstimer_get_time(&timers->timestamp);

    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{

    assert(data != NULL);
    polarfire_timers_t *timers = data;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = mstimer_get_time(&timers->timestamp);
        if (time >= ns) {
            return ETIME;
        }
        return mstimer_set_timeout(&timers->timeout, ns - time, false);
    }
    case TIMEOUT_RELATIVE:
        return mstimer_set_timeout(&timers->timeout, ns, false);
    case TIMEOUT_PERIODIC:
        return mstimer_set_timeout(&timers->timeout, ns, true);
    }

    return EINVAL;
}

static int reset(void *data)
{

    assert(data != NULL);
    polarfire_timers_t *timers = data;
    mstimer_stop(&timers->timestamp);
    mstimer_reset(&timers->timestamp);
    mstimer_start(&timers->timestamp);
    mstimer_stop(&timers->timeout);
    mstimer_reset(&timers->timeout);

    return 0;
}

static void destroy(void *data)
{

    assert(data);
    polarfire_timers_t *timers = data;
    int error;

    /* Destroy Timeout Timer */
    mstimer_stop(&timers->timeout);

    /* Destroy Timestamp Timer */
    mstimer_stop(&timers->timestamp);

    ps_pmem_unmap(&timers->ops, timer_pmem, timers->base_address);
    error = ps_irq_unregister(&timers->ops.irq_ops, timers->timeout.irq.irq.number);
    ZF_LOGF_IF(error, "Failed to unregister timeout IRQ");
    error = ps_irq_unregister(&timers->ops.irq_ops, timers->timestamp.irq.irq.number);
    ZF_LOGF_IF(error, "Failed to unregister timestamp IRQ");
    ps_free(&timers->ops.malloc_ops, sizeof(timers), timers);

}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{

    assert(ltimer != NULL);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(polarfire_timers_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}


int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{
    irq_id_t irq_id;
    int error;
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    error = create_ltimer_simple(ltimer, ops, sizeof(polarfire_timers_t),
                                 get_time, set_timeout, reset, destroy);
    if (error) {
        ZF_LOGE("Failed to create simple ltimer for polarfire");
        return EINVAL;
    }

    polarfire_timers_t *timers = ltimer->data;
    /* Map the memroy for the timer peripheral */
    timers->base_address = ps_pmem_map(&ops, timer_pmem, false, PS_MEM_NORMAL);
    if (timers->base_address == NULL) {
        ZF_LOGE("Failed to map timer memroy");
        return EINVAL;
    }

    /* Make sure that the 64 bit concat mode is disabled */
    volatile uint32_t *timer_base = timers->base_address;
    timer_base[MS_TIMER64_CONTROL_REG] = 0;
    timer_base[MS_TIMER64_MODE_REG] = 0;

    timers->ops = ops;
    timers->user_callback = callback;
    timers->user_callback_token = callback_token;
    /* Build Configurations */
    mstimer_config_t timeout_config = {
        .base_vaddr = timers->base_address,
        .base_address_offset = TIMEOUT_ADDR_OFFSET,
        .irq = &irqs[TIMEOUT_TIMER],
    };
    mstimer_config_t timestamp_config = {
        .base_vaddr = timers->base_address,
        .base_address_offset = TIMESTAMP_ADDR_OFFSET,
        .irq = &irqs[TIMESTAMP_TIMER],
    };

    /* Register IRQ's */
    timers->callback_data[TIMEOUT_TIMER].ltimer = ltimer;
    timers->callback_data[TIMEOUT_TIMER].irq = &irqs[TIMEOUT_TIMER];
    timers->callback_data[TIMEOUT_TIMER].irq_handler = ltimer_handle_irq;

    irq_id = ps_irq_register(&ops.irq_ops,
                             irqs[TIMEOUT_TIMER],
                             handle_irq_wrapper,
                             &timers->callback_data[TIMEOUT_TIMER]);
    if (irq_id < 0) {
        ZF_LOGE("Failed to register irq %i for MSTimer", irqs[TIMEOUT_TIMER].irq.number);
        return EINVAL;
    }

    timers->callback_data[TIMESTAMP_TIMER].ltimer = ltimer;
    timers->callback_data[TIMESTAMP_TIMER].irq = &irqs[TIMESTAMP_TIMER];
    timers->callback_data[TIMESTAMP_TIMER].irq_handler = ltimer_handle_irq;

    irq_id = ps_irq_register(&ops.irq_ops,
                             irqs[TIMESTAMP_TIMER],
                             handle_irq_wrapper,
                             &timers->callback_data[TIMESTAMP_TIMER]);
    if (irq_id < 0) {
        ZF_LOGE("Failed to register irq %i for MSTimer", irqs[TIMESTAMP_TIMER].irq.number);
        return EINVAL;
    }

    /* Initialize timeout timer */
    error = mstimer_init(&timers->timeout, ops, timeout_config);
    if (error) {
        ZF_LOGE("Failed to initialize Polarfire timeout timer");
        destroy(timers);
    }

    /* Initialize timestamp timer */
    error = mstimer_init(&timers->timestamp, ops, timestamp_config);
    if (error) {
        ZF_LOGE("Failed to initialize Polarfire timestamp timer");
        destroy(timers);
    }

    /* Start timestamp timer */
    error = mstimer_start(&timers->timestamp);
    if (error) {
        ZF_LOGE("Failed to start PolarFire timestamp timer");
        destroy(timers);
    }

    return 0;
}

/* This function is intended to be deleted,
 * this is just left here for now so that stuff can compile */
int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ZF_LOGE("get_(nth/num)_(irqs/pmems) are not valid");
    return EINVAL;
}
