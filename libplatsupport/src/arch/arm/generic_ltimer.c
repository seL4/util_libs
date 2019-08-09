/*
 * Copyright 2019, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 128 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */
#include <autoconf.h>
#include <stdio.h>
#include <assert.h>

#include <utils/util.h>
#include <utils/time.h>
#include <utils/frequency.h>

#include <platsupport/ltimer.h>
#include <platsupport/arch/generic_timer.h>
#include <platsupport/io.h>
#include <platsupport/plat/timer.h>

#include "../../ltimer.h"

typedef struct {
    uint32_t freq; // frequency of the generic timer
    uint64_t period; // period of a current periodic timeout, in ns
    irq_id_t timer_irq_id;
    timer_callback_data_t callback_data;
    ltimer_callback_fn_t user_callback;
    void *user_callback_token;
    ps_io_ops_t ops;
} generic_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    irq->type = PS_INTERRUPT;
    irq->irq.number = GENERIC_TIMER_PCNT_IRQ;
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 0;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    return -1;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    generic_ltimer_t *ltimer = data;
    uint64_t ticks = generic_timer_get_ticks();
    *time = freq_cycles_and_hz_to_ns(ticks, ltimer->freq);
    return 0;
}

int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    generic_ltimer_t *ltimer = data;
    if (type == TIMEOUT_PERIODIC) {
        ltimer->period = ns;
    } else {
        ltimer->period = 0;
    }

    uint64_t time;
    int error = get_time(data, &time);
    if (type != TIMEOUT_ABSOLUTE) {
        if (error) {
            return error;
        }
        ns += time;
    }

    if (time > ns) {
        return ETIME;
    }
    generic_timer_set_compare(freq_ns_and_hz_to_cycles(ns, ltimer->freq));

    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    if (irq->irq.number != GENERIC_TIMER_PCNT_IRQ) {
        return EINVAL;
    }

    generic_ltimer_t *ltimer = data;
    if (ltimer->period) {
        set_timeout(data, ltimer->period, TIMEOUT_PERIODIC);
    } else {
        generic_timer_set_compare(UINT64_MAX);
    }

    /* Interrupts are only generated for the timeout portion */
    if (ltimer->user_callback) {
        ltimer->user_callback(ltimer->user_callback_token, LTIMER_TIMEOUT_EVENT);
    }
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int reset(void *data)
{
    generic_ltimer_t *generic_ltimer = data;
    generic_ltimer->period = 0;
    generic_timer_set_compare(UINT64_MAX);
    return 0;
}

static void destroy(void *data)
{
    int error;
    generic_ltimer_t *generic_ltimer = data;
    generic_timer_disable();
    if (generic_ltimer->callback_data.irq) {
        ps_free(&generic_ltimer->ops.malloc_ops, sizeof(ps_irq_t), generic_ltimer->callback_data.irq);
    }
    if (generic_ltimer->timer_irq_id > PS_INVALID_IRQ_ID) {
        error = ps_irq_unregister(&ops.irq_ops, generic_ltimer->timer_irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ ID");
    }
    ps_free(&generic_ltimer->ops.malloc_ops, sizeof(generic_ltimer_t), generic_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops, ltimer_callback_fn_t callback, void *callback_data)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    if (!config_set(CONFIG_EXPORT_PCNT_USER)) {
        ZF_LOGE("Generic timer not exported!");
        return ENXIO;
    }

    ltimer_default_describe(ltimer, ops);
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(generic_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    generic_ltimer_t *generic_ltimer = ltimer->data;

    generic_ltimer->ops = ops;
    generic_ltimer->user_callback = callback;
    generic_ltimer->user_callback_token = callback_token;

    generic_ltimer->freq = generic_timer_get_freq();
    if (generic_ltimer->freq == 0) {
        ZF_LOGE("Couldn't read timer frequency");
        return ENXIO;
    }

    generic_timer_set_compare(UINT64_MAX);
    generic_timer_enable();

    /* register the IRQ we need */
    error = ps_calloc(&ops.malloc_ops, 1, sizeof(ps_irq_t), (void **) &generic_timer->callback_data.irq);
    if (error) {
        destroy(ltimer->data);
        return error;
    }
    generic_timer->callback_data.ltimer = ltimer;
    generic_timer->callback_data.irq_handler = handle_irq;
    error = get_nth_irq(ltimer->data, 0, generic_timer->callback_data.irq);
    if (error) {
        destroy(ltimer->data);
        return error;
    }
    generic_timer->timer_irq_id = ps_irq_register(&ops.irq_ops, *generic_timer->callback_data.irq,
                                                  handle_irq_wrapper, &generic_timer->callback_data);
    if (generic_timer->timer_irq_id < 0) {
        destroy(ltimer->data);
        return EIO;
    }

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
