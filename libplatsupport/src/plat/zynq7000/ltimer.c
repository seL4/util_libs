/*
 * Copyright 2017, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(DATA61_BSD)
 */

/* Minimal implementation of a logical timer for zynq
 *
 * Does not implement some functions yet.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/plat/timer.h>

#include <utils/util.h>

#define TTC_ID TTC0_TIMER1

typedef struct {
    ttc_t ttc;
    void *vaddr;
    ps_io_ops_t ops;
} ttc_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));

    irq->type = PS_INTERRUPT;
    irq->irq.number = ttc_irq(TTC_ID);
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 1;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    region->length = PAGE_SIZE_4K;
    region->base_addr = ttc_paddr(TTC_ID);
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;
    ttc_handle_irq(&ttc_ltimer->ttc);
    return EINVAL;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    ttc_t * ttc0_timer1 = (ttc_t *) data;
    *time = ttc_get_time(ttc0_timer1);

    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    if (type == TIMEOUT_ABSOLUTE) {
        return ENOSYS;
    }

    return ttc_set_timeout(&ttc_ltimer->ttc, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    /* reset the timers */
    ttc_stop(&ttc_ltimer->ttc);
    ttc_start(&ttc_ltimer->ttc);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    ttc_ltimer_t *ttc_ltimer = data;
    if (ttc_ltimer->vaddr) {
        ttc_stop(&ttc_ltimer->ttc);
        pmem_region_t region;
        UNUSED int error = get_nth_pmem(data, 0, &region);
        assert(!error);
        ps_pmem_unmap(&ttc_ltimer->ops, region, ttc_ltimer->vaddr);
    }

    ps_free(&ttc_ltimer->ops.malloc_ops, sizeof(ttc_ltimer), ttc_ltimer);
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(ttc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    ttc_ltimer_t *ttc_ltimer = ltimer->data;
    ttc_ltimer->ops = ops;

    pmem_region_t region;
    error = get_nth_pmem(ltimer->data, 0, &region);
    assert(error == 0);
    ttc_ltimer->vaddr = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (ttc_ltimer->vaddr == NULL) {
        error = -1;
    }

    ttc_config_t config = {
        .vaddr = ttc_ltimer->vaddr,
        .id = TTC_ID,
    };

    error = ttc_init(&ttc_ltimer->ttc ,config);

    if (!error) {
        error = ttc_start(&ttc_ltimer->ttc);
    }

    if (error) {
        destroy(ttc_ltimer);
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
