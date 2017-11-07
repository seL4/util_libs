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
/* Implementation of a logical timer for omap platforms
 *
 * We use two GPTS: one for the time and relative timeouts, the other
 * for absolute timeouts.
 */
#include <platsupport/timer.h>
#include <platsupport/ltimer.h>
#include <platsupport/mach/timer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#define NV_TMR_ID TMR0

typedef struct {
    nv_tmr_t nv_tmr;
    void *vaddr;
    ps_io_ops_t ops;
    uint64_t period;
} nv_tmr_ltimer_t;

static pmem_region_t pmem =
{
    .type = PMEM_TYPE_DEVICE,
    .base_addr = NV_TMR_PADDR,
    .length = PAGE_SIZE_4K
};

size_t get_num_irqs(void *data)
{
    return 1;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n == 0);
    irq->irq.number = nv_tmr_get_irq(NV_TMR_ID);
    irq->type = PS_INTERRUPT;
    return 0;
}

static size_t get_num_pmems(void *data)
{
    return 1;
}

static int get_nth_pmem(void *data, size_t n, pmem_region_t *paddr)
{
    assert(n == 0);
    *paddr = pmem;
    return 0;
}

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    nv_tmr_handle_irq(&nv_tmr_ltimer->nv_tmr);
    if (nv_tmr_ltimer->period > 0) {
        nv_tmr_set_timeout(&nv_tmr_ltimer->nv_tmr, false, nv_tmr_ltimer->period);
    }
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    *time = nv_tmr_get_time(&nv_tmr_ltimer->nv_tmr);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    nv_tmr_ltimer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE: {
        uint64_t time = nv_tmr_get_time(&nv_tmr_ltimer->nv_tmr);
        if (time >= ns) {
            return ETIME;
        }
        return nv_tmr_set_timeout(&nv_tmr_ltimer->nv_tmr, false, ns - time);
    }
    case TIMEOUT_PERIODIC:
        nv_tmr_ltimer->period = ns;
        /* fall through */
    case TIMEOUT_RELATIVE:
        return nv_tmr_set_timeout(&nv_tmr_ltimer->nv_tmr, false, ns);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    nv_tmr_start(&nv_tmr_ltimer->nv_tmr);
    nv_tmr_stop(&nv_tmr_ltimer->nv_tmr);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    nv_tmr_ltimer_t *nv_tmr_ltimer = data;
    if (nv_tmr_ltimer->vaddr) {
        nv_tmr_stop(&nv_tmr_ltimer->nv_tmr);
        nv_tmr_handle_irq(&nv_tmr_ltimer->nv_tmr);
        ps_pmem_unmap(&nv_tmr_ltimer->ops, pmem, nv_tmr_ltimer->vaddr);
    }
    ps_free(&nv_tmr_ltimer->ops.malloc_ops, sizeof(nv_tmr_ltimer), nv_tmr_ltimer);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(nv_tmr_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    nv_tmr_ltimer_t *nv_tmr_ltimer = ltimer->data;
    nv_tmr_ltimer->ops = ops;
    nv_tmr_ltimer->vaddr = ps_pmem_map(&ops, pmem, false, PS_MEM_NORMAL);
    if (nv_tmr_ltimer->vaddr == NULL) {
        destroy(ltimer->data);
    }

    /* setup nv_tmr */
    nv_tmr_config_t config = {
        .vaddr = (uintptr_t) nv_tmr_ltimer->vaddr,
        .id = TMR0
    };

    nv_tmr_init(&nv_tmr_ltimer->nv_tmr, config);
    nv_tmr_start(&nv_tmr_ltimer->nv_tmr);
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
