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

#include <platsupport/ltimer.h>
#include <platsupport/io.h>
#include <utils/io.h>
#include <platsupport/plat/meson_timer.h>

#include "../../ltimer.h"

enum {
    MESON_TIMER = 0,
    NUM_TIMERS = 1
};

typedef struct {
    meson_timer_t meson_timer;
    void *meson_timer_vaddr;
    irq_id_t timer_irq_id;
    timer_callback_data_t callback_data;
    ps_io_ops_t ops;
} odroidc2_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = TIMER_MAP_BASE,
        .length = PAGE_SIZE_4K
    }
};

static ps_irq_t irqs[] = {
    {
        .type = PS_TRIGGER,
        .trigger.number = TIMER_A_IRQ,
        .trigger.trigger = 1 /* edge-triggered */
    }
};

static size_t get_num_irqs(void *data)
{
    return sizeof(irqs) / sizeof(irqs[0]);
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    *irq = irqs[n];
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return sizeof(pmems) / sizeof(pmems[0]);
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    *region = pmems[n];
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    odroidc2_ltimer_t *odroidc2_timer = data;

    if (irq->irq.number != TIMER_A_IRQ) {
        ZF_LOGE("Got IRQ from unkown source?");
        return EINVAL;
    }

    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(time);
    assert(data);

    odroidc2_ltimer_t *odroidc2_timer = data;

    *time = meson_get_time(&odroidc2_timer->meson_timer);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    odroidc2_ltimer_t *odroidc2_timer = data;
    bool periodic = false;
    uint16_t timeout;
    uint64_t current_time;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        current_time = meson_get_time(&odroidc2_timer->meson_timer);
        if (current_time > ns) {
            ZF_LOGE("Timeout in the past: now %lu, timeout %lu", current_time, ns);
            return ETIME;
        } else if ((ns - current_time) / NS_IN_MS > UINT16_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }

        timeout = (ns - current_time) / NS_IN_MS;
        break;
    case TIMEOUT_PERIODIC:
        periodic = true;
    /* Fall through */
    case TIMEOUT_RELATIVE:
        if (ns / NS_IN_MS > UINT16_MAX) {
            ZF_LOGE("Timeout too far in the future");
            return ETIME;
        }

        timeout = ns / NS_IN_MS;
        break;
    }

    /* The timer tick includes 0, i.e. one unit of time greater than the value written */
    timeout = MIN(timeout, (uint16_t)(timeout - 1));
    meson_set_timeout(&odroidc2_timer->meson_timer, timeout, periodic);
    return 0;
}

static int reset(void *data)
{
    assert(data != NULL);
    odroidc2_ltimer_t *odroidc2_timer = data;

    meson_stop_timer(&odroidc2_timer->meson_timer);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    odroidc2_ltimer_t *odroidc2_timer = data;

    meson_stop_timer(&odroidc2_timer->meson_timer);

    if (odroidc2_timer->meson_timer_vaddr) {
        ps_pmem_unmap(&odroidc2_timer->ops, pmems[0], odroidc2_timer->meson_timer_vaddr);
    }

    if (odroidc2_timer->timer_irq_id > PS_INVALID_IRQ_ID) {
        int error = ps_irq_unregister(&odroidc2_timer->ops.irq_ops, odroidc2_timer->timer_irq_id);
        ZF_LOGF_IF(error, "Failed to unregister IRQ");
    }

    ps_free(&odroidc2_timer->ops.malloc_ops, sizeof(odroidc2_ltimer_t), odroidc2_timer);
    return;
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("ltimer cannot be NULL");
        return EINVAL;
    }

    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(odroidc2_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    odroidc2_ltimer_t *odroidc2_timer = ltimer->data;

    odroidc2_timer->ops = ops;
    odroidc2_timer->timer_irq_id = PS_INVALID_IRQ_ID;

    odroidc2_timer->meson_timer_vaddr = ps_pmem_map(&ops, pmems[0], false, PS_MEM_NORMAL);
    if (odroidc2_timer->meson_timer_vaddr == NULL) {
        destroy(ltimer->data);
        return EINVAL;
    }

    odroidc2_timer->callback_data.ltimer = ltimer;
    odroidc2_timer->callback_data.irq = &irqs[0];

    odroidc2_timer->timer_irq_id = ps_irq_register(&ops.irq_ops, irqs[0], handle_irq_wrapper,
                                                   &odroidc2_timer->callback_data);
    if (odroidc2_timer->timer_irq_id < 0) {
        destroy(ltimer->data);
        return EIO;
    }

    meson_timer_config_t meson_config = {
        .vaddr = odroidc2_timer->meson_timer_vaddr
    };
    meson_init(&odroidc2_timer->meson_timer, meson_config);
    return 0;
}

int ltimer_default_describe(ltimer_t *ltimer, ps_io_ops_t ops)
{
    if (ltimer == NULL) {
        ZF_LOGE("Timer is NULL!");
        return EINVAL;
    }

    *ltimer = (ltimer_t) {
        .get_num_irqs = get_num_irqs,
        .get_nth_irq = get_nth_irq,
        .get_num_pmems = get_num_pmems,
        .get_nth_pmem = get_nth_pmem
    };

    return 0;
}
