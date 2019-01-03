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
#include <platsupport/plat/spt.h>
#include <platsupport/plat/system_timer.h>
#include <platsupport/pmem.h>
#include <utils/util.h>

#define NV_TMR_ID TMR0

enum {
    SP804_TIMER = 0,
    SYSTEM_TIMER = 1,
    NUM_TIMERS = 2,
};

typedef struct {
    spt_t spt;
    void *spt_vaddr;
    system_timer_t system;
    void *system_vaddr;
    ps_io_ops_t ops;
    uint64_t period;
} spt_ltimer_t;

static pmem_region_t pmems[] = {
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = SP804_TIMER_PADDR,
        .length = PAGE_SIZE_4K
    },
    {
        .type = PMEM_TYPE_DEVICE,
        .base_addr = SYSTEM_TIMER_PADDR,
        .length = PAGE_SIZE_4K
    }
};

size_t get_num_irqs(void *data)
{
    return NUM_TIMERS;
}

static int get_nth_irq(void *data, size_t n, ps_irq_t *irq)
{
    assert(n < get_num_irqs(data));
    switch (n) {
        case SP804_TIMER:
            irq->irq.number = SP804_TIMER_IRQ;
            irq->type = PS_INTERRUPT;
            break;
        case SYSTEM_TIMER:
            irq->irq.number = SYSTEM_TIMER_MATCH_IRQ(SYSTEM_TIMER_MATCH);
            irq->type = PS_INTERRUPT;
            break;
    }
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

static int handle_irq(void *data, ps_irq_t *irq)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;

    switch (irq->irq.number) {
        case SP804_TIMER_IRQ:
            spt_handle_irq(&spt_ltimer->spt);
            if (spt_ltimer->period > 0) {
                spt_set_timeout(&spt_ltimer->spt, spt_ltimer->period);
            }
            break;
        case SYSTEM_TIMER_MATCH_IRQ(SYSTEM_TIMER_MATCH):
            system_timer_handle_irq(&spt_ltimer->system);
            break;
        default:
            return EINVAL;
    }
    return 0;
}

static int get_time(void *data, uint64_t *time)
{
    assert(data != NULL);
    assert(time != NULL);

    spt_ltimer_t *spt_ltimer = data;
    *time = system_timer_get_time(&spt_ltimer->system);
    return 0;
}

static int get_resolution(void *data, uint64_t *resolution)
{
    return ENOSYS;
}

static int set_timeout(void *data, uint64_t ns, timeout_type_t type)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_ltimer->period = 0;

    switch (type) {
    case TIMEOUT_ABSOLUTE:
        return system_timer_set_timeout(&spt_ltimer->system, ns);
    case TIMEOUT_PERIODIC:
        spt_ltimer->period = ns;
        /* fall through */
    case TIMEOUT_RELATIVE:
        return spt_set_timeout(&spt_ltimer->spt, ns);
    }

    return EINVAL;
}

static int reset(void *data)
{
    assert(data != NULL);
    spt_ltimer_t *spt_ltimer = data;
    spt_stop(&spt_ltimer->spt);
    spt_start(&spt_ltimer->spt);
    return 0;
}

static void destroy(void *data)
{
    assert(data);
    spt_ltimer_t *spt_ltimer = data;
    if (spt_ltimer->spt_vaddr) {
        spt_stop(&spt_ltimer->spt);
        ps_pmem_unmap(
            &spt_ltimer->ops,
            pmems[SP804_TIMER],
            spt_ltimer->spt_vaddr
        );
    }
    if (spt_ltimer->system_vaddr) {
        ps_pmem_unmap(
            &spt_ltimer->ops,
            pmems[SYSTEM_TIMER],
            spt_ltimer->system_vaddr
        );
    }
    ps_free(&spt_ltimer->ops.malloc_ops, sizeof(spt_ltimer), spt_ltimer);
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

    error = ps_calloc(&ops.malloc_ops, 1, sizeof(spt_ltimer_t), &ltimer->data);
    if (error) {
        return error;
    }
    assert(ltimer->data != NULL);
    spt_ltimer_t *spt_ltimer = ltimer->data;
    spt_ltimer->ops = ops;
    spt_ltimer->spt_vaddr = ps_pmem_map(&ops, pmems[SP804_TIMER], false, PS_MEM_NORMAL);
    if (spt_ltimer->spt_vaddr == NULL) {
        destroy(ltimer->data);
    }
    spt_ltimer->system_vaddr = ps_pmem_map(&ops, pmems[SYSTEM_TIMER], false, PS_MEM_NORMAL);
    if (spt_ltimer->system_vaddr == NULL) {
        destroy(ltimer->data);
    }

    /* setup spt */
    spt_config_t spt_config = {
        .vaddr = spt_ltimer->spt_vaddr,
    };

    spt_init(&spt_ltimer->spt, spt_config);
    spt_start(&spt_ltimer->spt);

    /* Setup system timer */
    system_timer_config_t system_config = {
        .vaddr = spt_ltimer->system_vaddr,
    };

    system_timer_init(&spt_ltimer->system, system_config);

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
