/*
 * Copyright 2019, Data61, CSIRO (ABN 41 687 119 230)
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */
/* Implementation of a logical timer for Ariane SoC platform.
 *
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>
#include <platsupport/plat/apb_timer.h>

#include "../../ltimer.h"

typedef struct {
    apb_timer_t apb_timer;
    void *vaddr;
} apb_timer_ltimer_t;

typedef struct {
    apb_timer_ltimer_t apb_timer_ltimer;
    //ps_irq_t irq;
    ps_io_ops_t ops;
    irq_id_t irq_id;
    ltimer_callback_fn_t user_callback;
    timer_callback_data_t callback_data;
    void *user_callback_token;
    uint64_t period;
} ariane_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = APB_TIMER_PADDR,
        .length = PAGE_SIZE_4K
    }
};

static ps_irq_t irqs[] = {
    {
        .type = PS_INTERRUPT,
        .irq.number = APB_TIMER_IRQ_CMP(0)

    }
};

size_t get_num_irqs(void *data)
{
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(data != NULL);
    assert(irq != NULL);
    assert(n == 0);

    irq->irq.number = APB_TIMER_IRQ_CMP(0);
    irq->type = PS_INTERRUPT;
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

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    ariane_ltimer_t *timer = data;

    *time = apb_timer_get_time(&timer->apb_timer_ltimer.apb_timer);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    ariane_ltimer_t *timer = data;
    timer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = apb_timer_get_time(&timer->apb_timer_ltimer.apb_timer);
        if (time >= ns) {
            return ETIME;
        }
        apb_timer_set_timeout(&timer->apb_timer_ltimer.apb_timer, ns - time);
        return 0;
    }
    case TIMEOUT_PERIODIC: {
        timer->period = ns;
        /* fall through */
    }
    case TIMEOUT_RELATIVE: {
        apb_timer_set_timeout(&timer->apb_timer_ltimer.apb_timer, ns);
        return 0;
    }
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    ariane_ltimer_t *timer = data;
    apb_timer_stop(&timer->apb_timer_ltimer.apb_timer);
    apb_timer_start(&timer->apb_timer_ltimer.apb_timer);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    ariane_ltimer_t *timer = data;
    if (timer->apb_timer_ltimer.vaddr) {
        apb_timer_stop(&timer->apb_timer_ltimer.apb_timer);
        ps_pmem_unmap(&timer->ops, pmems[0], timer->apb_timer_ltimer.vaddr);
    }
    if (timer->irq_id > PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&timer->ops.irq_ops, timer->irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }
    ps_free(&timer->ops.malloc_ops, sizeof(timer), timer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    assert(ltimer != NULL);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(ariane_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

static int init_ltimer(ltimer_t *ltimer)
{
    assert(ltimer != NULL);
    ariane_ltimer_t *timer = ltimer->data;

    /* setup apb_timer */
    apb_timer_config_t config_counter = {
        .vaddr = timer->apb_timer_ltimer.vaddr
    };

    apb_timer_init(&timer->apb_timer_ltimer.apb_timer, config_counter);
    apb_timer_start(&timer->apb_timer_ltimer.apb_timer);
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    ariane_ltimer_t *timer = data;

    timer->apb_timer_ltimer.apb_timer.time_h += timer->apb_timer_ltimer.apb_timer.apb_timer_map->cmp;
    apb_timer_stop(&timer->apb_timer_ltimer.apb_timer);

    if (timer->period > 0) {
        apb_timer_set_timeout(&timer->apb_timer_ltimer.apb_timer, timer->period);
    }

    ltimer_event_t event = LTIMER_TIMEOUT_EVENT;
    if (timer->user_callback) {
        timer->user_callback(timer->user_callback_token, event);
    }
    apb_timer_start(&timer->apb_timer_ltimer.apb_timer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_token)
{

    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    ariane_ltimer_t *timer = ltimer->data;
    timer->ops = ops;
    timer->user_callback = callback;
    timer->user_callback_token = callback_token;
    timer->irq_id = 0;//PS_INVALID_IRQ_ID;

    /* map the registers we need */
    timer->apb_timer_ltimer.vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (timer->apb_timer_ltimer.vaddr == NULL) {
        destroy(ltimer->data);
        return ENOMEM;
    }

    /* register the IRQs we need */
    error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t), (void **) &timer->callback_data.irq);
    if (error) {
        destroy(ltimer->data);
        return error;
    }
    timer->callback_data.ltimer = ltimer;
    timer->callback_data.irq_handler = handle_irq;
    error = get_nth_irq(ltimer->data, 0, timer->callback_data.irq);
    if (error) {
        destroy(ltimer->data);
        return error;
    }
    timer->irq_id = ps_irq_register(&ops.irq_ops, *timer->callback_data.irq, handle_irq_wrapper,
                                    &timer->callback_data);
    if (timer->irq_id < 0) {
        destroy(ltimer->data);
        return EIO;
    }

    error = init_ltimer(ltimer);
    if (error) {
        destroy(ltimer->data);
        return error;
    }
    /* success! */
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
