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

/* Use ttc0_timer1 for timeouts/sleep */
#define TTC_TIMEOUT   TTC0_TIMER1
/* Use ttc1_timer1 to keep running for timestamp/gettime */
#define TTC_TIMESTAMP TTC1_TIMER1

typedef struct {
    ttc_t ttc_timeout;
    void *vaddr_timeout;

    ttc_t ttc_timestamp;
    void *vaddr_timestamp;

    ps_io_ops_t ops;
    /* ns that have passed on the timestamp counter */
    uint64_t time;
} ttc_ltimer_t;

static size_t get_num_irqs(void *data)
{
    return 2;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));

    irq->type = PS_INTERRUPT;

    switch (n) {
    case 0:
        irq->irq.number = ttc_irq(TTC_TIMEOUT);
        break;
    case 1:
        irq->irq.number = ttc_irq(TTC_TIMESTAMP);
        break;
    default:
        ZF_LOGE("Invalid irq\n");
        return EINVAL;
    }

    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 2;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *region)
{
    assert(n < get_num_pmems(data));
    region->length = PAGE_SIZE_4K;

    switch(n) {
    case 0:
        region->base_addr = ttc_paddr(TTC_TIMEOUT);
        break;
    case 1:
        region->base_addr = ttc_paddr(TTC_TIMESTAMP);
        break;
    default:
        ZF_LOGE("Invalid timer number");
    }
    return 0;
}

/* increment the high bits of the counter if an irq has occured */
static void update_timestamp(ttc_ltimer_t *ttc_ltimer)
{
    if (ttc_handle_irq(&ttc_ltimer->ttc_timestamp)) {
        /* if handle irq returns a high value, we have an overflow irq unhandled */
        ttc_ltimer->time += ttc_ticks_to_ns(&ttc_ltimer->ttc_timestamp, UINT16_MAX);
    }
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    if (irq->irq.number == ttc_irq(TTC_TIMEOUT)) {
        ttc_handle_irq(&ttc_ltimer->ttc_timeout);
    } else if (irq->irq.number == ttc_irq(TTC_TIMESTAMP)) {
        update_timestamp(ttc_ltimer);
    } else {
        return EINVAL;
    }
    return 0;
}

static uint64_t read_time(ttc_ltimer_t *ttc_ltimer)
{
    uint64_t ticks = ttc_get_time(&ttc_ltimer->ttc_timestamp);
    update_timestamp(ttc_ltimer);
    uint64_t ticks2 = ttc_get_time(&ttc_ltimer->ttc_timestamp);
    if (ticks2 < ticks) {
        /* overflow occured  */
        update_timestamp(ttc_ltimer);
        ticks = ticks2;
    }

    return ticks + ttc_ltimer->time;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);
    *time = read_time(data);
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
        uint64_t time = read_time(data);
        if (ns <= time) {
            return ETIME;
        } else {
            ns -= time;
        }
    }

    return ttc_set_timeout(&ttc_ltimer->ttc_timeout, ns, type == TIMEOUT_PERIODIC);
}

static int reset(void *data)
{
    assert(data != NULL);
    ttc_ltimer_t *ttc_ltimer = data;

    /* reset the timers */
    ttc_stop(&ttc_ltimer->ttc_timeout);
    ttc_start(&ttc_ltimer->ttc_timeout);
    ttc_stop(&ttc_ltimer->ttc_timestamp);
    ttc_start(&ttc_ltimer->ttc_timestamp);

    return 0;
}

static void destroy(void *data)
{
    assert(data);

    ttc_ltimer_t *ttc_ltimer = data;
    if (ttc_ltimer->vaddr_timeout) {
        ttc_stop(&ttc_ltimer->ttc_timeout);
        pmem_region_t region;
        UNUSED int error = get_nth_pmem(data, 0, &region);
        assert(!error);
        ps_pmem_unmap(&ttc_ltimer->ops, region, ttc_ltimer->vaddr_timeout);
    }

    if (ttc_ltimer->vaddr_timestamp) {
        ttc_stop(&ttc_ltimer->ttc_timestamp);
        pmem_region_t region;
        UNUSED int error = get_nth_pmem(data, 1, &region);
        assert(!error);
        ps_pmem_unmap(&ttc_ltimer->ops, region, ttc_ltimer->vaddr_timestamp);
    }

    ps_free(&ttc_ltimer->ops.malloc_ops, sizeof(ttc_ltimer), ttc_ltimer);
}

static int create_ltimer(ltimer_t *ltimer, ps_io_ops_t ops)
{
    ltimer->handle_irq = handle_irq;
    ltimer->get_time = get_time;
    ltimer->get_resolution = get_resolution;
    ltimer->set_timeout = set_timeout;
    ltimer->reset = reset;
    ltimer->destroy = destroy;

    int error = ps_calloc(&ops.malloc_ops, 1, sizeof(ttc_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);

    return 0;
}

static int init_ltimer(ltimer_t *ltimer)
{
    int error;

    assert(ltimer != NULL);
    ttc_ltimer_t *ttc_ltimer = ltimer->data;

    ttc_config_t config = {
        .vaddr = ttc_ltimer->vaddr_timeout,
        .id = TTC_TIMEOUT,
    };

    ttc_config_t config_timestamp = {
        .vaddr = ttc_ltimer->vaddr_timestamp,
        .id = TTC_TIMESTAMP,
    };

    error = ttc_init(&ttc_ltimer->ttc_timeout ,config);
    assert(error == 0);
    error = ttc_start(&ttc_ltimer->ttc_timeout);
    assert(error == 0);

    /* set the second ttc to be a timestamp counter */
    error = ttc_init(&ttc_ltimer->ttc_timestamp ,config_timestamp);

    if (!error) {
        ttc_freerun(&ttc_ltimer->ttc_timestamp);
        error = ttc_start(&ttc_ltimer->ttc_timestamp);
    }
}

int ltimer_default_init(ltimer_t *ltimer, ps_io_ops_t ops)
{
    int error = ltimer_default_describe(ltimer, ops);
    if (error) {
        return error;
    }

    error = create_ltimer(ltimer, ops);
    if (error) {
        return error;
    }

    ttc_ltimer_t *ttc_ltimer = ltimer->data;
    ttc_ltimer->ops = ops;

    /* Default timeout timer */
    pmem_region_t region;
    error = get_nth_pmem(ltimer->data, 0, &region);
    assert(error == 0);
    ttc_ltimer->vaddr_timeout = ps_pmem_map(&ops, region, false, PS_MEM_NORMAL);
    if (ttc_ltimer->vaddr_timeout == NULL) {
        error = -1;
    }

    /* Default timestamp timer */
    pmem_region_t region_timestamp;
    error = get_nth_pmem(ltimer->data, 1, &region_timestamp);
    assert(error == 0);
    ttc_ltimer->vaddr_timestamp = ps_pmem_map(&ops, region_timestamp, false, PS_MEM_NORMAL);
    if (ttc_ltimer->vaddr_timestamp == NULL) {
        error = -1;
    }

    error = init_ltimer(ltimer);
    if (error) {
        destroy(ttc_ltimer);
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
